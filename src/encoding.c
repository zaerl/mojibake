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
        bom_encoding = (mjb_encoding)(bom_encoding | MJB_ENC_UTF_8);
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
    return mjb_string_validate(buffer, byte_length, MJB_ENC_UTF_8, NULL) == MJB_STATUS_OK;
}

/**
 * Return true if the string is encoded in ASCII.
 */
MJB_EXPORT bool mjb_is_ascii(const char *buffer, size_t byte_length) {
    return mjb_string_validate(buffer, byte_length, MJB_ENC_ASCII, NULL) == MJB_STATUS_OK;
}

/**
 * Return true if the string is encoded in UTF-16BE or UTF-16LE.
 */
MJB_EXPORT bool mjb_is_utf16(const char *buffer, size_t byte_length) {
    return mjb_string_validate(buffer, byte_length, MJB_ENC_UTF_16BE, NULL) == MJB_STATUS_OK ||
        mjb_string_validate(buffer, byte_length, MJB_ENC_UTF_16LE, NULL) == MJB_STATUS_OK;
}

static size_t mjb_encoding_code_unit_size(mjb_encoding encoding) {
    if(encoding == MJB_ENC_UTF_16BE || encoding == MJB_ENC_UTF_16LE) {
        return 2;
    }

    if(encoding == MJB_ENC_UTF_32BE || encoding == MJB_ENC_UTF_32LE) {
        return 4;
    }

    return 1;
}

static void mjb_set_diagnostic(mjb_diagnostic *diagnostic, mjb_text_error error, size_t byte_offset,
    size_t byte_length, mjb_encoding encoding) {
    if(diagnostic == NULL) {
        return;
    }

    diagnostic->error = error;
    diagnostic->byte_offset = byte_offset;
    diagnostic->byte_length = byte_length;
    diagnostic->code_unit_offset = byte_offset / mjb_encoding_code_unit_size(encoding);
}

static uint16_t mjb_read_utf16_unit(const char *buffer, size_t offset, bool big_endian) {
    uint16_t first = (uint8_t)buffer[offset];
    uint16_t second = (uint8_t)buffer[offset + 1];

    return big_endian ? (uint16_t)((first << 8) | second) : (uint16_t)(first | (second << 8));
}

static uint32_t mjb_read_utf32_unit(const char *buffer, size_t offset, bool big_endian) {
    uint32_t b0 = (uint8_t)buffer[offset];
    uint32_t b1 = (uint8_t)buffer[offset + 1];
    uint32_t b2 = (uint8_t)buffer[offset + 2];
    uint32_t b3 = (uint8_t)buffer[offset + 3];

    if(big_endian) {
        return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
    }

    return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

static mjb_status mjb_decode_next_raw(const char *buffer, size_t byte_length, mjb_encoding encoding,
    size_t *offset, mjb_codepoint *codepoint, mjb_diagnostic *diagnostic) {
    size_t start = *offset;

    if(start >= byte_length) {
        return MJB_STATUS_END_OF_INPUT;
    }

    if(encoding == MJB_ENC_ASCII) {
        uint8_t byte = (uint8_t)buffer[start];
        *offset = start + 1;

        if(byte <= 0x7F) {
            *codepoint = byte;

            return MJB_STATUS_OK;
        }

        *codepoint = MJB_CODEPOINT_REPLACEMENT;
        mjb_set_diagnostic(diagnostic, MJB_TEXT_ERROR_NON_ASCII, start, 1, encoding);

        return MJB_STATUS_MALFORMED_INPUT;
    }

    if(encoding == MJB_ENC_UTF_8) {
        uint8_t first = (uint8_t)buffer[start];

        if(first <= 0x7F) {
            *offset = start + 1;
            *codepoint = first;

            return MJB_STATUS_OK;
        }

        if(first >= 0x80 && first <= 0xBF) {
            *offset = start + 1;
            *codepoint = MJB_CODEPOINT_REPLACEMENT;
            mjb_set_diagnostic(diagnostic, MJB_TEXT_ERROR_UNEXPECTED_CONTINUATION, start, 1,
                encoding);

            return MJB_STATUS_MALFORMED_INPUT;
        }

        if(first == 0xC0 || first == 0xC1) {
            *offset = start + 1;
            *codepoint = MJB_CODEPOINT_REPLACEMENT;
            mjb_set_diagnostic(diagnostic, MJB_TEXT_ERROR_OVERLONG_SEQUENCE, start, 1, encoding);

            return MJB_STATUS_MALFORMED_INPUT;
        }

        size_t required;

        if(first <= 0xDF) {
            required = 2;
        } else if(first <= 0xEF) {
            required = 3;
        } else if(first <= 0xF4) {
            required = 4;
        } else {
            *offset = start + 1;
            *codepoint = MJB_CODEPOINT_REPLACEMENT;
            mjb_set_diagnostic(diagnostic,
                first <= 0xF7 ? MJB_TEXT_ERROR_OUT_OF_RANGE : MJB_TEXT_ERROR_INVALID_LEADING_BYTE,
                start, 1, encoding);

            return MJB_STATUS_MALFORMED_INPUT;
        }

        size_t remaining = byte_length - start;

        if(remaining < 2) {
            *offset = byte_length;
            *codepoint = MJB_CODEPOINT_REPLACEMENT;
            mjb_set_diagnostic(diagnostic, MJB_TEXT_ERROR_TRUNCATED_SEQUENCE, start, remaining,
                encoding);

            return MJB_STATUS_MALFORMED_INPUT;
        }

        uint8_t second = (uint8_t)buffer[start + 1];

        if((second & 0xC0) != 0x80) {
            *offset = start + 1;
            *codepoint = MJB_CODEPOINT_REPLACEMENT;
            mjb_set_diagnostic(diagnostic, MJB_TEXT_ERROR_MISSING_CONTINUATION, start, 1, encoding);

            return MJB_STATUS_MALFORMED_INPUT;
        }

        mjb_text_error constrained_error = MJB_TEXT_ERROR_NONE;

        if((first == 0xE0 && second < 0xA0) || (first == 0xF0 && second < 0x90)) {
            constrained_error = MJB_TEXT_ERROR_OVERLONG_SEQUENCE;
        } else if(first == 0xED && second >= 0xA0) {
            constrained_error = MJB_TEXT_ERROR_SURROGATE;
        } else if(first == 0xF4 && second >= 0x90) {
            constrained_error = MJB_TEXT_ERROR_OUT_OF_RANGE;
        }

        if(constrained_error != MJB_TEXT_ERROR_NONE) {
            // Only the lead byte is a maximal subpart when the second byte violates the
            // well-formed UTF-8 range for that lead byte.
            *offset = start + 1;
            *codepoint = MJB_CODEPOINT_REPLACEMENT;
            mjb_set_diagnostic(diagnostic, constrained_error, start, 1, encoding);

            return MJB_STATUS_MALFORMED_INPUT;
        }

        mjb_codepoint decoded;

        if(required == 2) {
            decoded = (mjb_codepoint)(first & 0x1F);
        } else if(required == 3) {
            decoded = (mjb_codepoint)(first & 0x0F);
        } else {
            decoded = (mjb_codepoint)(first & 0x07);
        }

        decoded = (decoded << 6) | ((mjb_codepoint)second & 0x3F);

        for(size_t i = 2; i < required; ++i) {
            if(i >= remaining) {
                *offset = byte_length;
                *codepoint = MJB_CODEPOINT_REPLACEMENT;
                mjb_set_diagnostic(diagnostic, MJB_TEXT_ERROR_TRUNCATED_SEQUENCE, start, remaining,
                    encoding);

                return MJB_STATUS_MALFORMED_INPUT;
            }

            uint8_t continuation = (uint8_t)buffer[start + i];

            if((continuation & 0xC0) != 0x80) {
                *offset = start + i;
                *codepoint = MJB_CODEPOINT_REPLACEMENT;
                mjb_set_diagnostic(diagnostic, MJB_TEXT_ERROR_MISSING_CONTINUATION, start, i,
                    encoding);

                return MJB_STATUS_MALFORMED_INPUT;
            }

            decoded = (decoded << 6) | ((mjb_codepoint)continuation & 0x3F);
        }

        *codepoint = decoded;
        *offset = start + required;

        return MJB_STATUS_OK;
    }

    if(encoding == MJB_ENC_UTF_16BE || encoding == MJB_ENC_UTF_16LE) {
        size_t remaining = byte_length - start;

        if(remaining < 2) {
            *offset = byte_length;
            *codepoint = MJB_CODEPOINT_REPLACEMENT;
            mjb_set_diagnostic(diagnostic, MJB_TEXT_ERROR_TRUNCATED_CODE_UNIT, start, remaining,
                encoding);

            return MJB_STATUS_MALFORMED_INPUT;
        }

        bool big_endian = encoding == MJB_ENC_UTF_16BE;
        uint16_t first = mjb_read_utf16_unit(buffer, start, big_endian);

        if(first >= 0xD800 && first <= 0xDBFF) {
            if(remaining < 4) {
                *offset = start + 2;
                *codepoint = MJB_CODEPOINT_REPLACEMENT;
                mjb_set_diagnostic(diagnostic, MJB_TEXT_ERROR_UNPAIRED_SURROGATE, start, 2,
                    encoding);

                return MJB_STATUS_MALFORMED_INPUT;
            }

            uint16_t second = mjb_read_utf16_unit(buffer, start + 2, big_endian);

            if(second < 0xDC00 || second > 0xDFFF) {
                *offset = start + 2;
                *codepoint = MJB_CODEPOINT_REPLACEMENT;
                mjb_set_diagnostic(diagnostic, MJB_TEXT_ERROR_UNPAIRED_SURROGATE, start, 2,
                    encoding);

                return MJB_STATUS_MALFORMED_INPUT;
            }

            *codepoint = 0x10000 + (((mjb_codepoint)first - 0xD800) << 10) +
                ((mjb_codepoint)second - 0xDC00);
            *offset = start + 4;

            return MJB_STATUS_OK;
        }

        *offset = start + 2;

        if(first >= 0xDC00 && first <= 0xDFFF) {
            *codepoint = MJB_CODEPOINT_REPLACEMENT;
            mjb_set_diagnostic(diagnostic, MJB_TEXT_ERROR_UNPAIRED_SURROGATE, start, 2, encoding);

            return MJB_STATUS_MALFORMED_INPUT;
        }

        *codepoint = first;

        return MJB_STATUS_OK;
    }

    if(encoding == MJB_ENC_UTF_32BE || encoding == MJB_ENC_UTF_32LE) {
        size_t remaining = byte_length - start;

        if(remaining < 4) {
            *offset = byte_length;
            *codepoint = MJB_CODEPOINT_REPLACEMENT;
            mjb_set_diagnostic(diagnostic, MJB_TEXT_ERROR_TRUNCATED_CODE_UNIT, start, remaining,
                encoding);

            return MJB_STATUS_MALFORMED_INPUT;
        }

        mjb_codepoint decoded = mjb_read_utf32_unit(buffer, start, encoding == MJB_ENC_UTF_32BE);
        *offset = start + 4;

        if(mjb_codepoint_is_surrogate(decoded)) {
            *codepoint = MJB_CODEPOINT_REPLACEMENT;
            mjb_set_diagnostic(diagnostic, MJB_TEXT_ERROR_SURROGATE, start, 4, encoding);

            return MJB_STATUS_MALFORMED_INPUT;
        }

        if(decoded > MJB_CODEPOINT_MAX) {
            *codepoint = MJB_CODEPOINT_REPLACEMENT;
            mjb_set_diagnostic(diagnostic, MJB_TEXT_ERROR_OUT_OF_RANGE, start, 4, encoding);

            return MJB_STATUS_MALFORMED_INPUT;
        }

        *codepoint = decoded;

        return MJB_STATUS_OK;
    }

    return MJB_STATUS_INVALID_ENCODING;
}

static mjb_status mjb_resolve_decode_input(const char *buffer, size_t *byte_length,
    mjb_encoding requested_encoding, size_t *offset, mjb_encoding *resolved_encoding) {
    if(!mjb_encoding_is_valid_input(requested_encoding)) {
        return MJB_STATUS_INVALID_ENCODING;
    }

    mjb_status status = mjb_resolve_input_byte_length(buffer, byte_length, requested_encoding);

    if(status != MJB_STATUS_OK) {
        return status;
    }

    if((buffer == NULL && *byte_length > 0) || *offset > *byte_length) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    size_t data_start = 0;
    *resolved_encoding = mjb_resolve_input_encoding(buffer, *byte_length, requested_encoding,
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

MJB_EXPORT mjb_status mjb_decode_next(const char *buffer, size_t byte_length, mjb_encoding encoding,
    mjb_malformed_policy malformed_policy, size_t *offset, mjb_codepoint *codepoint,
    mjb_diagnostic *diagnostic) {
    if(offset == NULL || codepoint == NULL) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    mjb_diagnostic_reset(diagnostic);
    *codepoint = MJB_CODEPOINT_NOT_VALID;

    if(!mjb_malformed_policy_is_valid(malformed_policy)) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    mjb_encoding resolved_encoding;
    mjb_status status = mjb_resolve_decode_input(buffer, &byte_length, encoding, offset,
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

MJB_EXPORT mjb_status mjb_decode_previous(const char *buffer, size_t byte_length,
    mjb_encoding encoding, mjb_malformed_policy malformed_policy, size_t *offset,
    mjb_codepoint *codepoint, mjb_diagnostic *diagnostic) {
    if(offset == NULL || codepoint == NULL) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    mjb_diagnostic_reset(diagnostic);
    *codepoint = MJB_CODEPOINT_NOT_VALID;

    if(!mjb_malformed_policy_is_valid(malformed_policy)) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    if(!mjb_encoding_is_valid_input(encoding)) {
        return MJB_STATUS_INVALID_ENCODING;
    }

    mjb_status status = mjb_resolve_input_byte_length(buffer, &byte_length, encoding);

    if(status != MJB_STATUS_OK) {
        return status;
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
        status = mjb_decode_previous_raw(buffer, data_start, resolved_encoding, offset, codepoint,
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

MJB_EXPORT mjb_status mjb_string_validate(const char *buffer, size_t byte_length,
    mjb_encoding encoding, mjb_diagnostic *diagnostic) {
    mjb_diagnostic_reset(diagnostic);

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
    mjb_codepoint codepoint = 0;

    for(size_t offset = context->input_index;;) {
        mjb_diagnostic current;
        mjb_status decode_status = mjb_decode_next(context->buffer, context->byte_length,
            context->input_encoding, context->malformed_policy, &offset, &codepoint, &current);
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

MJB_EXPORT mjb_status mjb_convert_encoding(const char *buffer, size_t byte_length,
    mjb_encoding encoding, mjb_malformed_policy malformed_policy, mjb_encoding output_encoding,
    mjb_result *result, mjb_diagnostic *diagnostic) {
    if(result == NULL || (buffer == NULL && byte_length > 0) ||
        !mjb_malformed_policy_is_valid(malformed_policy)) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    mjb_diagnostic_reset(diagnostic);

    if(!mjb_encoding_is_valid_output(output_encoding)) {
        return MJB_STATUS_INVALID_ENCODING;
    }

    mjb_status status = mjb_resolve_input_byte_length(buffer, &byte_length, encoding);

    if(status != MJB_STATUS_OK) {
        return status;
    }

    result->output = NULL;
    result->output_size = 0;
    result->transformed = false;

    size_t input_index = 0;
    mjb_encoding input_encoding = mjb_resolve_input_encoding(buffer, byte_length, encoding,
        &input_index);

    if(input_encoding == MJB_ENC_UTF_16 || input_encoding == MJB_ENC_UTF_32) {
        return MJB_STATUS_INVALID_ENCODING;
    }

    if(byte_length == 0 || encoding == output_encoding) {
        mjb_diagnostic validity;
        status = mjb_string_validate(buffer, byte_length, encoding, &validity);
        mjb_diagnostic_record(diagnostic, &validity);

        if(status == MJB_STATUS_OK) {
            result->output = (char *)buffer;
            result->output_size = byte_length;

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
        mjb_free(output.buffer);

        return status;
    }

    result->output = output.buffer;
    result->output_size = output.size;
    result->transformed = true;

    return MJB_STATUS_OK;
}

MJB_EXPORT mjb_status mjb_convert_encoding_into(const char *buffer, size_t byte_length,
    mjb_encoding encoding, mjb_malformed_policy malformed_policy, mjb_encoding output_encoding,
    void *output, size_t *output_size, mjb_diagnostic *diagnostic) {
    if(output_size == NULL) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    mjb_diagnostic_reset(diagnostic);

    if(!mjb_malformed_policy_is_valid(malformed_policy)) {
        *output_size = 0;

        return MJB_STATUS_INVALID_ARGUMENT;
    }

    if(buffer == NULL && byte_length > 0) {
        *output_size = 0;

        return MJB_STATUS_INVALID_ARGUMENT;
    }

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
        status = mjb_string_validate(buffer, byte_length, encoding, &validity);
        mjb_diagnostic_record(diagnostic, &validity);

        if(status == MJB_STATUS_OK) {
            return mjb_output_copy_into(buffer, byte_length, output, output_size);
        }

        if(status != MJB_STATUS_MALFORMED_INPUT || malformed_policy == MJB_MALFORMED_STOP) {
            *output_size = 0;

            return status;
        }
    }

    mjb_convert_encoding_context context = { buffer, byte_length, input_index, input_encoding,
        malformed_policy, output_encoding, diagnostic };

    return mjb_output_into(output, output_size, mjb_convert_encoding_write, &context);
}
