/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "mojibake.h"
#include "utf.h"

static bool mjb_codepoint_is_surrogate(mjb_codepoint codepoint) {
    return codepoint >= 0xD800 && codepoint <= 0xDFFF;
}

/**
 * Return the encoding from the BOM (if possible). mjb_resolve_input_encoding() later chooses one
 * concrete endian form and consumes the signature when decoding generic UTF-16/UTF-32 input.
 */
static mjb_encoding mjb_encoding_from_bom(const char *buffer, size_t byte_length) {
    if(byte_length < 2) {
        // BOM are at least 2 characters
        return MJB_ENC_UNKNOWN;
    }

    if(mjb_starts_with_utf8_bom(buffer, byte_length)) {
        return MJB_ENC_UTF_8;
    }

    mjb_encoding bom_encoding = MJB_ENC_UNKNOWN;

    if(mjb_starts_with_utf32be_bom(buffer, byte_length)) {
        bom_encoding = (mjb_encoding)(MJB_ENC_UTF_32 | MJB_ENC_UTF_32BE);
    } else if(mjb_starts_with_utf32le_bom(buffer, byte_length)) {
        // A UTF-32LE BOM also has the UTF-16LE BOM prefix.
        bom_encoding = (mjb_encoding)(MJB_ENC_UTF_32 | MJB_ENC_UTF_32LE | MJB_ENC_UTF_16LE);
    }

    if(mjb_starts_with_utf16be_bom(buffer, byte_length)) {
        bom_encoding = (mjb_encoding)(MJB_ENC_UTF_16 | MJB_ENC_UTF_16BE);
    } else if(mjb_starts_with_utf16le_bom(buffer, byte_length)) {
        bom_encoding = (mjb_encoding)(bom_encoding | MJB_ENC_UTF_16 | MJB_ENC_UTF_16LE);
    }

    return bom_encoding;
}

/**
 * Return the string encoding (the most probable).
 */
MJB_EXPORT mjb_encoding mjb_string_encoding(const char *buffer, size_t byte_length) {
    if(buffer == NULL || byte_length == 0) {
        return MJB_ENC_UNKNOWN;
    }

    mjb_encoding bom_encoding = mjb_encoding_from_bom(buffer, byte_length);

    if(bom_encoding != MJB_ENC_UNKNOWN) {
        return bom_encoding;
    }

    // No BOM, let's try UTF-8
    if(mjb_string_is_utf8(buffer, byte_length)) {
        bom_encoding = (mjb_encoding)(bom_encoding | MJB_ENC_UTF_8);
    }

    // No BOM, let's try ASCII
    if(mjb_string_is_ascii(buffer, byte_length)) {
        bom_encoding = (mjb_encoding)(bom_encoding | MJB_ENC_ASCII);
    }

    return bom_encoding;
}

/**
 * Return true if the string is encoded in UTF-8.
 */
MJB_EXPORT bool mjb_string_is_utf8(const char *buffer, size_t byte_length) {
    if(buffer == NULL || byte_length == 0) {
        return false;
    }

    uint8_t state = MJB_UTF_ACCEPT;
    mjb_codepoint codepoint = MJB_CODEPOINT_NOT_VALID;

    // Loop through the string.
    for(size_t i = 0; i < byte_length; ++i) {
#if !MJB_DANGEROUSLY_ALLOW_EMBEDDED_NULLS
        if(!buffer[i]) {
            break;
        }
#endif

        // Find next codepoint.
        state = mjb_utf8_decode_step(state, buffer[i], &codepoint);

        if(state == MJB_UTF_REJECT) {
            // The string is not well-formed.
            return false;
        }
    }

    return state == MJB_UTF_ACCEPT;
}

/**
 * Return true if the string is encoded in ASCII.
 */
MJB_EXPORT bool mjb_string_is_ascii(const char *buffer, size_t byte_length) {
    if(buffer == NULL || byte_length == 0) {
        return false;
    }

    for(size_t i = 0; i < byte_length; ++i) {
#if !MJB_DANGEROUSLY_ALLOW_EMBEDDED_NULLS
        if(!buffer[i]) {
            break;
        }
#endif

        // Every character must have leading bit at zero.
        if(buffer[i] & 0x80) {
            return false;
        }
    }

    return 1;
}

/**
 * Return true if the string is encoded in UTF-16BE or UTF-16LE.
 */
MJB_EXPORT bool mjb_string_is_utf16(const char *buffer, size_t byte_length) {
    if(buffer == NULL || byte_length < 2 || (byte_length % 2) != 0) {
        return false;
    }

    // Try UTF-16BE first
    uint8_t state_be = MJB_UTF_ACCEPT;
    mjb_codepoint codepoint = MJB_CODEPOINT_NOT_VALID;
    bool be_valid = true;

    for(size_t i = 0; i < byte_length; i += 2) {
        state_be = mjb_utf16_decode_step(state_be, buffer[i], buffer[i + 1], &codepoint, true);

        if(state_be == MJB_UTF_REJECT) {
            be_valid = false;  // Error in UTF-16BE
            break;
        }
    }

    if(be_valid && state_be == MJB_UTF_ACCEPT) {
        return true;  // Valid UTF-16BE
    }

    // Try UTF-16LE
    uint8_t state_le = MJB_UTF_ACCEPT;
    bool le_valid = true;

    for(size_t i = 0; i < byte_length; i += 2) {
        state_le = mjb_utf16_decode_step(state_le, buffer[i], buffer[i + 1], &codepoint, false);

        if(state_le == MJB_UTF_REJECT) {
            le_valid = false;  // Error in UTF-16LE
            break;
        }
    }

    return le_valid && state_le == MJB_UTF_ACCEPT;  // Valid UTF-16LE
}

MJB_EXPORT unsigned int mjb_codepoint_encode(mjb_codepoint codepoint, char *buffer, size_t byte_length,
    mjb_encoding encoding) {
    if(buffer == NULL || byte_length < 2) {
        return 0;
    }

    if(mjb_codepoint_is_surrogate(codepoint)) {
        return 0;
    }

    if(encoding == MJB_ENC_ASCII) {
        if(codepoint <= 0x7F) {
            buffer[0] = (char)codepoint;
            buffer[1] = '\0';

            return 1;
        }
    } else if(encoding == MJB_ENC_UTF_8) {
        if(codepoint <= 0x7F) {
            // 0b0x|xx|xx|xx, 1 byte sequence (ASCII)
            buffer[0] = (char)codepoint;
            buffer[1] = '\0';

            return 1;
        } else if(codepoint <= 0x7FF) {
            if(byte_length < 3) {
                return 0;
            }

            // 0b11|0x|xx|xx: 2 bytes sequence
            buffer[0] = (char)(0xC0 | ((codepoint >> 6) & 0x1F));
            buffer[1] = (char)(0x80 | (codepoint & 0x3F));
            buffer[2] = '\0';

            return 2;
        } else if(codepoint <= 0xFFFF) {
            if(byte_length < 4) {
                return 0;
            }

            // 0b11|10|xx|xx: 3 bytes sequence
            buffer[0] = (char)(0xE0 | ((codepoint >> 12) & 0x0F));
            buffer[1] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
            buffer[2] = (char)(0x80 | (codepoint & 0x3F));
            buffer[3] = '\0';

            return 3;
        } else if(codepoint <= 0x10FFFF) {
            if(byte_length < 5) {
                return 0;
            }

            // 0b11|11|0x|xx: 4 bytes sequence
            buffer[0] = (char)(0xF0 | ((codepoint >> 18) & 0x07));
            buffer[1] = (char)(0x80 | ((codepoint >> 12) & 0x3F));
            buffer[2] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
            buffer[3] = (char)(0x80 | (codepoint & 0x3F));
            buffer[4] = '\0';

            return 4;
        }
    } else if(((encoding & MJB_ENC_UTF_16LE) || (encoding & MJB_ENC_UTF_16BE)) &&
        !((encoding & MJB_ENC_UTF_32LE) || (encoding & MJB_ENC_UTF_32BE))) {
        if(byte_length < 3) {
            return 0;
        }

        if(codepoint <= 0xFFFF) {
            // Basic Multilingual Plane - single 16-bit code unit
            if(encoding & MJB_ENC_UTF_16LE) {
                buffer[0] = (char)(codepoint & 0xFF);
                buffer[1] = (char)((codepoint >> 8) & 0xFF);
            } else {
                buffer[0] = (char)((codepoint >> 8) & 0xFF);
                buffer[1] = (char)(codepoint & 0xFF);
            }

            buffer[2] = '\0';

            return 2;
        } else if(codepoint <= 0x10FFFF) {
            // Supplementary Planes - surrogate pair
            if(byte_length < 5) {
                return 0;
            }

            // Convert to surrogate pair
            uint32_t adjusted = codepoint - 0x10000;
            uint16_t high = 0xD800 | ((adjusted >> 10) & 0x3FF);
            uint16_t low = 0xDC00 | (adjusted & 0x3FF);

            if(encoding & MJB_ENC_UTF_16LE) {
                buffer[0] = (char)(high & 0xFF);
                buffer[1] = (char)((high >> 8) & 0xFF);
                buffer[2] = (char)(low & 0xFF);
                buffer[3] = (char)((low >> 8) & 0xFF);
            } else {
                buffer[0] = (char)((high >> 8) & 0xFF);
                buffer[1] = (char)(high & 0xFF);
                buffer[2] = (char)((low >> 8) & 0xFF);
                buffer[3] = (char)(low & 0xFF);
            }

            buffer[4] = '\0';

            return 4;
        }
    } else if((encoding & MJB_ENC_UTF_32LE) || (encoding & MJB_ENC_UTF_32BE)) {
        if(byte_length < 5) {
            return 0;
        }

        if(codepoint <= 0x10FFFF) {
            // UTF-32 uses a single 32-bit code unit for each codepoint
            if(encoding & MJB_ENC_UTF_32LE) {
                // Little endian: least significant byte first
                buffer[0] = (char)(codepoint & 0xFF);
                buffer[1] = (char)((codepoint >> 8) & 0xFF);
                buffer[2] = (char)((codepoint >> 16) & 0xFF);
                buffer[3] = (char)((codepoint >> 24) & 0xFF);
            } else {
                // Big endian: most significant byte first
                buffer[0] = (char)((codepoint >> 24) & 0xFF);
                buffer[1] = (char)((codepoint >> 16) & 0xFF);
                buffer[2] = (char)((codepoint >> 8) & 0xFF);
                buffer[3] = (char)(codepoint & 0xFF);
            }

            buffer[4] = '\0';

            return 4;
        }
    }

    return 0;
}

MJB_EXPORT mjb_status mjb_string_convert_encoding(const char *buffer, size_t byte_length,
    mjb_encoding encoding, mjb_encoding output_encoding, mjb_result *result) {
    if(result == NULL || (buffer == NULL && byte_length > 0)) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    result->output = NULL;
    result->output_size = 0;
    result->transformed = false;

    if(byte_length == 0 || encoding == output_encoding) {
        result->output = (char*)buffer;
        result->output_size = byte_length;
        result->transformed = false;

        return MJB_STATUS_OK;
    }

    size_t input_index = 0;
    mjb_encoding input_encoding = mjb_resolve_input_encoding(buffer, byte_length, encoding,
        &input_index);

    if(input_encoding == MJB_ENC_UTF_16 || input_encoding == MJB_ENC_UTF_32 ||
        output_encoding == MJB_ENC_UTF_16 || output_encoding == MJB_ENC_UTF_32) {
        return MJB_STATUS_INVALID_ENCODING;
    }

    uint8_t state = MJB_UTF_ACCEPT;
    mjb_codepoint codepoint = 0;
    result->output = (char*)mjb_alloc(byte_length);

    if(result->output == NULL) {
        result->output_size = 0;
        result->transformed = false;

        return MJB_STATUS_NO_MEMORY;
    }

    result->output_size = byte_length;
    result->transformed = true;
    size_t output_index = 0;
    bool in_error = false;

    for(size_t i = input_index; i < byte_length;) {
        mjb_decode_result decode_status = mjb_next_codepoint(buffer, byte_length, &state, &i,
            input_encoding, &codepoint, &in_error);

        if(decode_status == MJB_DECODE_END) {
            break;
        }

        if(decode_status == MJB_DECODE_INCOMPLETE) {
            continue;
        }

        char output_buffer[5];
        size_t output_size = mjb_codepoint_encode(codepoint, output_buffer,
            sizeof(output_buffer), output_encoding);

        if(output_size == 0) {
            mjb_result_free(result);

            return MJB_STATUS_UNSUPPORTED;
        }

        char *new_output = mjb_string_output(result->output, output_buffer, output_size,
            &output_index, &result->output_size);

        if(new_output != NULL) {
            result->output = new_output;
        } else {
            mjb_result_free(result);

            return MJB_STATUS_NO_MEMORY;
        }
    }

    result->output_size = output_index;

    return MJB_STATUS_OK;
}
