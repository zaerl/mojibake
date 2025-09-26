/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <string.h>

#include "mojibake.h"
#include "utf.h"

#define MJB_ENCODING_UTF_8_BOM "\xEF\xBB\xBF"
#define MJB_ENCODING_UTF_16_BE_BOM "\xFE\xFF"
#define MJB_ENCODING_UTF_16_LE_BOM "\xFF\xFE"
#define MJB_ENCODING_UTF_32_BE_BOM "\x00\x00\xFE\xFF"
#define MJB_ENCODING_UTF_32_LE_BOM "\xFF\xFE\x00\x00"

/**
 * Return the encoding from the BOM (if possible).
 */
static mjb_encoding mjb_encoding_from_bom(const char *buffer, size_t size) {
    if(size < 2) {
        // BOM are at least 2 characters
        return MJB_ENCODING_UNKNOWN;
    }

    if(size >= 3) {
        if(memcmp(buffer, MJB_ENCODING_UTF_8_BOM, 3) == 0) {
            return MJB_ENCODING_UTF_8;
        }
    }

    mjb_encoding bom_encoding = MJB_ENCODING_UNKNOWN;

    if(size >= 4) {
        if(memcmp(buffer, MJB_ENCODING_UTF_32_BE_BOM, 4) == 0) {
            bom_encoding = (mjb_encoding)(MJB_ENCODING_UTF_32 | MJB_ENCODING_UTF_32_BE);
        } else if(memcmp(buffer, MJB_ENCODING_UTF_32_LE_BOM, 4) == 0) {
            // A UTF-32-LE document is also valid UTF-16-LE
            bom_encoding = (mjb_encoding)(MJB_ENCODING_UTF_32 | MJB_ENCODING_UTF_32_LE | MJB_ENCODING_UTF_16_LE);
        }
    }

    if(size >= 2) {
        if(memcmp(buffer, MJB_ENCODING_UTF_16_BE_BOM, 2) == 0) {
            bom_encoding = (mjb_encoding)(MJB_ENCODING_UTF_16 | MJB_ENCODING_UTF_16_BE);
        } else if(memcmp(buffer, MJB_ENCODING_UTF_16_LE_BOM, 2) == 0) {
            bom_encoding = (mjb_encoding)(bom_encoding | MJB_ENCODING_UTF_16 | MJB_ENCODING_UTF_16_LE);
        }
    }

    return bom_encoding;
}

/**
 * Return the string encoding (the most probable).
 */
MJB_EXPORT mjb_encoding mjb_string_encoding(const char *buffer, size_t size) {
    if(buffer == 0 || size == 0) {
        return MJB_ENCODING_UNKNOWN;
    }

    mjb_encoding bom_encoding = mjb_encoding_from_bom(buffer, size);

    if(bom_encoding != MJB_ENCODING_UNKNOWN) {
        return bom_encoding;
    }

    // No BOM, let's try UTF-8
    if(mjb_string_is_utf8(buffer, size)) {
        bom_encoding = (mjb_encoding)(bom_encoding | MJB_ENCODING_UTF_8);
    }

    // No BOM, let's try ASCII
    if(mjb_string_is_ascii(buffer, size)) {
        bom_encoding = (mjb_encoding)(bom_encoding | MJB_ENCODING_ASCII);
    }

    return bom_encoding;
}

/**
 * Return true if the string is encoded in UTF-8.
 */
MJB_EXPORT bool mjb_string_is_utf8(const char *buffer, size_t size) {
    if(buffer == 0 || size == 0) {
        return false;
    }

    uint8_t state = MJB_UTF_ACCEPT;
    mjb_codepoint codepoint = MJB_CODEPOINT_NOT_VALID;

    // Loop through the string.
    for(size_t i = 0; i < size && buffer[i]; ++i) {
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
MJB_EXPORT bool mjb_string_is_ascii(const char *buffer, size_t size) {
    if(buffer == 0 || size == 0) {
        return false;
    }

    for(size_t i = 0; i < size && buffer[i]; ++i) {
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
MJB_EXPORT bool mjb_string_is_utf16(const char *buffer, size_t size) {
    if(buffer == 0 || size < 2 || (size % 2) != 0) {
        return false;
    }

    // Try UTF-16BE first
    uint8_t state_be = MJB_UTF_ACCEPT;
    mjb_codepoint codepoint = MJB_CODEPOINT_NOT_VALID;
    bool be_valid = true;

    for(size_t i = 0; i < size; i += 2) {
        state_be = mjb_utf16_decode_step(state_be, buffer[i], buffer[i + 1], &codepoint, true);

        if(state_be > MJB_UTF_REJECT) {
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

    for(size_t i = 0; i < size; i += 2) {
        state_le = mjb_utf16_decode_step(state_le, buffer[i], buffer[i + 1], &codepoint, false);

        if(state_le > MJB_UTF_REJECT) {
            le_valid = false;  // Error in UTF-16LE
            break;
        }
    }

    return le_valid && state_le == MJB_UTF_ACCEPT;  // Valid UTF-16LE
}

MJB_EXPORT unsigned int mjb_codepoint_encode(mjb_codepoint codepoint, char *buffer, size_t size,
    mjb_encoding encoding) {
    if(buffer == NULL || size < 2) {
        return 0;
    }

    if(encoding == MJB_ENCODING_UTF_8) {
        if(codepoint <= 0x7F) {
            // 0b0x|xx|xx|xx, 1 byte sequence (ASCII)
            buffer[0] = (char)codepoint;
            buffer[1] = '\0';

            return 1;
        } else if(codepoint <= 0x7FF) {
            if(size < 3) {
                return 0;
            }

            // 0b11|0x|xx|xx: 2 bytes sequence
            buffer[0] = (char)(0xC0 | ((codepoint >> 6) & 0x1F));
            buffer[1] = (char)(0x80 | (codepoint & 0x3F));
            buffer[2] = '\0';

            return 2;
        } else if(codepoint <= 0xFFFF) {
            if(size < 4) {
                return 0;
            }

            // 0b11|10|xx|xx: 3 bytes sequence
            buffer[0] = (char)(0xE0 | ((codepoint >> 12) & 0x0F));
            buffer[1] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
            buffer[2] = (char)(0x80 | (codepoint & 0x3F));
            buffer[3] = '\0';

            return 3;
        } else if(codepoint <= 0x10FFFF) {
            if(size < 5) {
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
    } else if((encoding & MJB_ENCODING_UTF_16_LE) || (encoding & MJB_ENCODING_UTF_16_BE)) {
        if(size < 3) {
            return 0;
        }

        if(codepoint <= 0xFFFF) {
            // Basic Multilingual Plane - single 16-bit code unit
            if(encoding & MJB_ENCODING_UTF_16_LE) {
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
            if(size < 5) {
                return 0;
            }

            // Convert to surrogate pair
            uint32_t adjusted = codepoint - 0x10000;
            uint16_t high = 0xD800 | ((adjusted >> 10) & 0x3FF);
            uint16_t low = 0xDC00 | (adjusted & 0x3FF);

            if(encoding & MJB_ENCODING_UTF_16_LE) {
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
    } else if((encoding & MJB_ENCODING_UTF_32_LE) || (encoding & MJB_ENCODING_UTF_32_BE)) {
        if(size < 5) {
            return 0;
        }

        if(codepoint <= 0x10FFFF && !(codepoint >= 0xD800 && codepoint <= 0xDFFF)) {
            // UTF-32 uses a single 32-bit code unit for each codepoint
            if(encoding & MJB_ENCODING_UTF_32_LE) {
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

MJB_EXPORT bool mjb_string_convert_encoding(const char *buffer, size_t size, mjb_encoding encoding,
    mjb_encoding output_encoding, mjb_result *result) {
    if(size == 0 || encoding == output_encoding) {
        result->output = (char*)buffer;
        result->output_size = 0;
        result->transformed = false;

        return true;
    }

    uint8_t state = MJB_UTF_ACCEPT;
    mjb_codepoint codepoint = 0;
    result->output = (char*)mjb_alloc(size);
    result->output_size = size;
    result->transformed = true;
    size_t output_index = 0;

    for(size_t i = 0; i < size; ++i) {
        if(!mjb_decode_step(buffer, size, &state, &i, encoding, &codepoint)) {
            break;
        }

        bool replaced = false;

        if(state == MJB_UTF_REJECT) {
            codepoint = MJB_CODEPOINT_REPLACEMENT;
            replaced = true;
        }

        if(state == MJB_UTF_ACCEPT || replaced) {
            char *new_output = mjb_string_output_codepoint(codepoint, result->output, &output_index,
                &result->output_size, output_encoding);

            if(new_output != NULL) {
                result->output = new_output;
            } else {
                // TODO: check if this is the correct behavior
                return false;
            }
        }
    }

    result->output_size = output_index;

    return true;
}
