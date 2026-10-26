/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <string.h>

#include "mojibake.h"
#include "utf.h"

typedef struct mjb_convert_encoding_context {
    const char *buffer;
    size_t byte_length;
    size_t input_index;
    mjb_encoding input_encoding;
    mjb_malformed_policy malformed_policy;
    mjb_encoding output_encoding;
    mjb_diagnostic *diagnostic;
} mjb_convert_encoding_context;

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
MJB_EXPORT mjb_encoding mjb_detect_encoding(const char *buffer, size_t byte_length) {
    if(buffer == NULL || byte_length == 0 || byte_length == MJB_NUL_TERMINATED) {
        return MJB_ENC_UNKNOWN;
    }

    mjb_encoding bom_encoding = mjb_encoding_from_bom(buffer, byte_length);

    if(bom_encoding != MJB_ENC_UNKNOWN) {
        return bom_encoding;
    }

    // No BOM, let's try UTF-8
    if(mjb_is_utf8(buffer, byte_length)) {
        bom_encoding = MJB_ENC_UTF_8;
    }

    // No BOM, let's try ASCII
    if(mjb_is_ascii(buffer, byte_length)) {
        bom_encoding = (mjb_encoding)(bom_encoding | MJB_ENC_ASCII);
    }

    return bom_encoding;
}

/**
 * Return true if the string is encoded in UTF-8.
 */
MJB_EXPORT bool mjb_is_utf8(const char *buffer, size_t byte_length) {
    mjb_text text = mjb_text_make(buffer, byte_length, MJB_ENC_UTF_8);

    return mjb_validate_encoding(&text, NULL) == MJB_STATUS_OK;
}

/**
 * Return true if the string is encoded in ASCII.
 */
MJB_EXPORT bool mjb_is_ascii(const char *buffer, size_t byte_length) {
    mjb_text text = mjb_text_make(buffer, byte_length, MJB_ENC_ASCII);

    return mjb_validate_encoding(&text, NULL) == MJB_STATUS_OK;
}

/**
 * Return true if the string is encoded in UTF-16BE or UTF-16LE.
 */
MJB_EXPORT bool mjb_is_utf16(const char *buffer, size_t byte_length) {
    mjb_text big_endian = mjb_text_make(buffer, byte_length, MJB_ENC_UTF_16BE);
    mjb_text little_endian = mjb_text_make(buffer, byte_length, MJB_ENC_UTF_16LE);

    return mjb_validate_encoding(&big_endian, NULL) == MJB_STATUS_OK ||
        mjb_validate_encoding(&little_endian, NULL) == MJB_STATUS_OK;
}

/**
 * Return the byte length of a string that ends at its first NUL code unit.
 */
MJB_EXPORT mjb_status mjb_nul_terminated_length(const char *buffer, mjb_encoding encoding,
    size_t *byte_length) {
    if(byte_length == NULL) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    *byte_length = MJB_NUL_TERMINATED;

    if(!mjb_encoding_is_valid_input(encoding)) {
        return MJB_STATUS_INVALID_ENCODING;
    }

    return mjb_scan_nul_terminated_byte_length(buffer, byte_length, encoding);
}

static mjb_status mjb_resolve_decode_input(const char *buffer, size_t byte_length,
    mjb_encoding requested_encoding, size_t *offset, mjb_encoding *resolved_encoding) {
    if(!mjb_encoding_is_valid_input(requested_encoding)) {
        return MJB_STATUS_INVALID_ENCODING;
    }

    if((buffer == NULL && byte_length > 0) || *offset > byte_length) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    size_t data_start = 0;
    *resolved_encoding = mjb_resolve_input_encoding(buffer, byte_length, requested_encoding,
        &data_start);

    if((requested_encoding == MJB_ENC_UTF_16 || requested_encoding == MJB_ENC_UTF_32) &&
        *resolved_encoding == requested_encoding) {
        return MJB_STATUS_INVALID_ENCODING;
    }

    if(*offset < data_start) {
        if(*offset != 0) {
            return MJB_STATUS_INVALID_ARGUMENT;
        }

        *offset = data_start;
    }

    return MJB_STATUS_OK;
}

MJB_EXPORT mjb_status mjb_decode_next(const mjb_text *text, mjb_malformed_policy malformed_policy,
    size_t *offset, mjb_codepoint *codepoint, mjb_diagnostic *diagnostic) {
    if(offset == NULL || codepoint == NULL) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    mjb_diagnostic_reset(diagnostic);
    *codepoint = MJB_CODEPOINT_NOT_VALID;

    // A NUL-terminated text would be rescanned on every call, making iteration quadratic.
    if(text == NULL || !mjb_malformed_policy_is_valid(malformed_policy) ||
        text->byte_length == MJB_NUL_TERMINATED) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    const char *buffer = text->buffer;
    size_t byte_length = text->byte_length;
    mjb_encoding encoding = text->encoding;

    mjb_encoding resolved_encoding;
    mjb_status status = mjb_resolve_decode_input(buffer, byte_length, encoding, offset,
        &resolved_encoding);

    if(status != MJB_STATUS_OK) {
        return status;
    }

    for(;;) {
        mjb_diagnostic current;
        mjb_diagnostic_reset(&current);
        status = mjb_decode_next_raw(buffer, byte_length, resolved_encoding, offset, codepoint,
            &current);
        mjb_diagnostic_record(diagnostic, &current);

        if(status != MJB_STATUS_MALFORMED_INPUT) {
            return status;
        }

        if(malformed_policy == MJB_MALFORMED_STOP) {
            return status;
        }

        if(malformed_policy == MJB_MALFORMED_REPLACE) {
            *codepoint = MJB_CODEPOINT_REPLACEMENT;

            return MJB_STATUS_OK;
        }
    }
}

static mjb_status mjb_decode_previous_raw(const char *buffer, size_t data_start,
    mjb_encoding encoding, size_t *offset, mjb_codepoint *codepoint, mjb_diagnostic *diagnostic) {
    size_t end = *offset;

    if(end <= data_start) {
        return MJB_STATUS_END_OF_INPUT;
    }

    if(encoding == MJB_ENC_ASCII) {
        size_t start = end - 1;
        size_t next = start;
        mjb_status status = mjb_decode_next_raw(buffer, end, encoding, &next, codepoint,
            diagnostic);
        *offset = start;

        return status;
    }

    if(encoding == MJB_ENC_UTF_8) {
        size_t candidate = end - 1;
        size_t continuation_count = 0;

        while(candidate > data_start && (((uint8_t)buffer[candidate] & 0xC0) == 0x80) &&
            continuation_count < 3) {
            --candidate;
            ++continuation_count;
        }

        size_t next = candidate;
        mjb_status status = mjb_decode_next_raw(buffer, end, encoding, &next, codepoint,
            diagnostic);

        if(next == end && (status == MJB_STATUS_OK || status == MJB_STATUS_MALFORMED_INPUT)) {
            *offset = candidate;

            return status;
        }

        candidate = end - 1;
        *offset = candidate;
        *codepoint = MJB_CODEPOINT_REPLACEMENT;
        mjb_set_diagnostic(diagnostic,
            (((uint8_t)buffer[candidate] & 0xC0) == 0x80) ? MJB_TEXT_ERROR_UNEXPECTED_CONTINUATION :
                                                            MJB_TEXT_ERROR_TRUNCATED_SEQUENCE,
            candidate, 1, encoding);

        return MJB_STATUS_MALFORMED_INPUT;
    }

    if(encoding == MJB_ENC_UTF_16BE || encoding == MJB_ENC_UTF_16LE) {
        size_t relative = end - data_start;

        if((relative % 2) != 0) {
            *offset = end - 1;
            *codepoint = MJB_CODEPOINT_REPLACEMENT;
            mjb_set_diagnostic(diagnostic, MJB_TEXT_ERROR_TRUNCATED_CODE_UNIT, end - 1, 1,
                encoding);

            return MJB_STATUS_MALFORMED_INPUT;
        }

        bool big_endian = encoding == MJB_ENC_UTF_16BE;
        size_t start = end - 2;
        uint16_t last = mjb_read_utf16_unit(buffer, start, big_endian);

        if(last >= 0xDC00 && last <= 0xDFFF && start >= data_start + 2) {
            uint16_t first = mjb_read_utf16_unit(buffer, start - 2, big_endian);

            if(first >= 0xD800 && first <= 0xDBFF) {
                *offset = start - 2;
                *codepoint = 0x10000 + (((mjb_codepoint)first - 0xD800) << 10) +
                    ((mjb_codepoint)last - 0xDC00);

                return MJB_STATUS_OK;
            }
        }

        *offset = start;

        if(last >= 0xD800 && last <= 0xDFFF) {
            *codepoint = MJB_CODEPOINT_REPLACEMENT;
            mjb_set_diagnostic(diagnostic, MJB_TEXT_ERROR_UNPAIRED_SURROGATE, start, 2, encoding);

            return MJB_STATUS_MALFORMED_INPUT;
        }

        *codepoint = last;

        return MJB_STATUS_OK;
    }

    if(encoding == MJB_ENC_UTF_32BE || encoding == MJB_ENC_UTF_32LE) {
        size_t relative = end - data_start;
        size_t remainder = relative % 4;

        if(remainder != 0) {
            size_t start = end - remainder;
            *offset = start;
            *codepoint = MJB_CODEPOINT_REPLACEMENT;
            mjb_set_diagnostic(diagnostic, MJB_TEXT_ERROR_TRUNCATED_CODE_UNIT, start, remainder,
                encoding);

            return MJB_STATUS_MALFORMED_INPUT;
        }

        size_t start = end - 4;
        size_t next = start;
        mjb_status status = mjb_decode_next_raw(buffer, end, encoding, &next, codepoint,
            diagnostic);
        *offset = start;

        return status;
    }

    return MJB_STATUS_INVALID_ENCODING;
}

MJB_EXPORT mjb_status mjb_decode_previous(const mjb_text *text,
    mjb_malformed_policy malformed_policy, size_t *offset, mjb_codepoint *codepoint,
    mjb_diagnostic *diagnostic) {
    if(offset == NULL || codepoint == NULL) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    mjb_diagnostic_reset(diagnostic);
    *codepoint = MJB_CODEPOINT_NOT_VALID;

    // A NUL-terminated text would be rescanned on every call, making iteration quadratic.
    if(text == NULL || !mjb_malformed_policy_is_valid(malformed_policy) ||
        text->byte_length == MJB_NUL_TERMINATED) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    const char *buffer = text->buffer;
    size_t byte_length = text->byte_length;
    mjb_encoding encoding = text->encoding;

    if(!mjb_encoding_is_valid_input(encoding)) {
        return MJB_STATUS_INVALID_ENCODING;
    }

    if((buffer == NULL && byte_length > 0) || *offset > byte_length) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    size_t data_start = 0;
    mjb_encoding resolved_encoding = mjb_resolve_input_encoding(buffer, byte_length, encoding,
        &data_start);

    if((encoding == MJB_ENC_UTF_16 || encoding == MJB_ENC_UTF_32) &&
        resolved_encoding == encoding) {
        return MJB_STATUS_INVALID_ENCODING;
    }

    if(*offset < data_start) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    for(;;) {
        mjb_diagnostic current;
        mjb_diagnostic_reset(&current);
        mjb_status status = mjb_decode_previous_raw(buffer, data_start, resolved_encoding, offset,
            codepoint, &current);
        mjb_diagnostic_record(diagnostic, &current);

        if(status != MJB_STATUS_MALFORMED_INPUT) {
            return status;
        }

        if(malformed_policy == MJB_MALFORMED_STOP) {
            return status;
        }

        if(malformed_policy == MJB_MALFORMED_REPLACE) {
            *codepoint = MJB_CODEPOINT_REPLACEMENT;

            return MJB_STATUS_OK;
        }
    }
}

MJB_EXPORT mjb_status mjb_validate_encoding(const mjb_text *text, mjb_diagnostic *diagnostic) {
    mjb_diagnostic_reset(diagnostic);

    if(text == NULL) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    const char *buffer = text->buffer;
    size_t byte_length = text->byte_length;
    mjb_encoding encoding = text->encoding;

    if(!mjb_encoding_is_valid_input(encoding)) {
        return MJB_STATUS_INVALID_ENCODING;
    }

    mjb_status status = mjb_resolve_input_byte_length(buffer, &byte_length, encoding);

    if(status != MJB_STATUS_OK) {
        return status;
    }

    if(buffer == NULL && byte_length > 0) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    size_t offset = 0;
    mjb_encoding resolved_encoding = mjb_resolve_input_encoding(buffer, byte_length, encoding,
        &offset);

    if((encoding == MJB_ENC_UTF_16 || encoding == MJB_ENC_UTF_32) &&
        resolved_encoding == encoding) {
        return MJB_STATUS_INVALID_ENCODING;
    }

    mjb_codepoint codepoint;

    for(;;) {
        status = mjb_decode_next_raw(buffer, byte_length, resolved_encoding, &offset, &codepoint,
            diagnostic);

        if(status == MJB_STATUS_END_OF_INPUT) {
            return MJB_STATUS_OK;
        }

        if(status != MJB_STATUS_OK) {
            return status;
        }
    }
}

MJB_EXPORT unsigned int mjb_codepoint_encode(mjb_codepoint codepoint, char *buffer,
    size_t byte_length, mjb_encoding encoding) {
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

static mjb_status mjb_convert_encoding_write(mjb_output *output, const void *context_pointer) {
    const mjb_convert_encoding_context *context = (const mjb_convert_encoding_context *)
        context_pointer;
    mjb_text input = mjb_text_make(context->buffer, context->byte_length, context->input_encoding);
    mjb_codepoint codepoint = 0;

    for(size_t offset = context->input_index;;) {
        mjb_diagnostic current;
        mjb_status decode_status = mjb_decode_next(&input, context->malformed_policy, &offset,
            &codepoint, &current);
        mjb_diagnostic_record(context->diagnostic, &current);

        if(decode_status == MJB_STATUS_END_OF_INPUT) {
            break;
        }

        if(decode_status != MJB_STATUS_OK) {
            return decode_status;
        }

        mjb_status status = mjb_output_codepoint(output, codepoint, context->output_encoding);

        if(status != MJB_STATUS_OK) {
            return status;
        }
    }

    return MJB_STATUS_OK;
}

MJB_EXPORT mjb_status mjb_convert_encoding(const mjb_text *text,
    mjb_malformed_policy malformed_policy, mjb_encoding output_encoding, mjb_result *result,
    mjb_diagnostic *diagnostic) {
    if(result == NULL || text == NULL || (text->buffer == NULL && text->byte_length > 0) ||
        !mjb_malformed_policy_is_valid(malformed_policy)) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    const char *buffer = text->buffer;
    size_t byte_length = text->byte_length;
    mjb_encoding encoding = text->encoding;

    mjb_diagnostic_reset(diagnostic);

    if(!mjb_encoding_is_valid_output(output_encoding)) {
        return MJB_STATUS_INVALID_ENCODING;
    }

    mjb_status status = mjb_resolve_input_byte_length(buffer, &byte_length, encoding);

    if(status != MJB_STATUS_OK) {
        return status;
    }

    mjb_result_borrow(result, NULL, 0);

    size_t input_index = 0;
    mjb_encoding input_encoding = mjb_resolve_input_encoding(buffer, byte_length, encoding,
        &input_index);

    if(input_encoding == MJB_ENC_UTF_16 || input_encoding == MJB_ENC_UTF_32) {
        return MJB_STATUS_INVALID_ENCODING;
    }

    if(byte_length == 0 || encoding == output_encoding) {
        mjb_diagnostic validity;
        mjb_text resolved = mjb_text_make(buffer, byte_length, encoding);
        status = mjb_validate_encoding(&resolved, &validity);
        mjb_diagnostic_record(diagnostic, &validity);

        if(status == MJB_STATUS_OK) {
            mjb_result_borrow(result, buffer, byte_length);

            return MJB_STATUS_OK;
        }

        if(status != MJB_STATUS_MALFORMED_INPUT || malformed_policy == MJB_MALFORMED_STOP) {
            return status;
        }
    }

    char *allocated = (char *)mjb_alloc(byte_length);

    if(allocated == NULL) {
        return MJB_STATUS_NO_MEMORY;
    }

    mjb_output output;
    mjb_output_init_dynamic(&output, allocated, byte_length);
    mjb_convert_encoding_context context = { buffer, byte_length, input_index, input_encoding,
        malformed_policy, output_encoding, diagnostic };
    status = mjb_convert_encoding_write(&output, &context);

    if(status != MJB_STATUS_OK) {
        mjb_free(output.buffer, output.capacity);

        return status;
    }

    mjb_result_own(result, output.buffer, output.size, output.capacity);

    return MJB_STATUS_OK;
}

MJB_EXPORT mjb_status mjb_convert_encoding_into(const mjb_text *text,
    mjb_malformed_policy malformed_policy, mjb_buffer *output, mjb_diagnostic *diagnostic) {
    if(output == NULL) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    void *data = output->data;
    size_t *output_size = &output->size;
    mjb_encoding output_encoding = output->encoding;

    mjb_diagnostic_reset(diagnostic);

    if(!mjb_malformed_policy_is_valid(malformed_policy)) {
        *output_size = 0;

        return MJB_STATUS_INVALID_ARGUMENT;
    }

    if(text == NULL || (text->buffer == NULL && text->byte_length > 0)) {
        *output_size = 0;

        return MJB_STATUS_INVALID_ARGUMENT;
    }

    const char *buffer = text->buffer;
    size_t byte_length = text->byte_length;
    mjb_encoding encoding = text->encoding;

    if(!mjb_encoding_is_valid_output(output_encoding)) {
        *output_size = 0;

        return MJB_STATUS_INVALID_ENCODING;
    }

    mjb_status status = mjb_resolve_input_byte_length(buffer, &byte_length, encoding);

    if(status != MJB_STATUS_OK) {
        *output_size = 0;

        return status;
    }

    size_t input_index = 0;
    mjb_encoding input_encoding = mjb_resolve_input_encoding(buffer, byte_length, encoding,
        &input_index);

    if(input_encoding == MJB_ENC_UTF_16 || input_encoding == MJB_ENC_UTF_32) {
        *output_size = 0;

        return MJB_STATUS_INVALID_ENCODING;
    }

    if(byte_length == 0 || encoding == output_encoding) {
        mjb_diagnostic validity;
        mjb_text resolved = mjb_text_make(buffer, byte_length, encoding);
        status = mjb_validate_encoding(&resolved, &validity);
        mjb_diagnostic_record(diagnostic, &validity);

        if(status == MJB_STATUS_OK) {
            return mjb_output_copy_into(buffer, byte_length, data, output_size);
        }

        if(status != MJB_STATUS_MALFORMED_INPUT || malformed_policy == MJB_MALFORMED_STOP) {
            *output_size = 0;

            return status;
        }
    }

    mjb_convert_encoding_context context = { buffer, byte_length, input_index, input_encoding,
        malformed_policy, output_encoding, diagnostic };

    return mjb_output_into(data, output_size, mjb_convert_encoding_write, &context);
}
