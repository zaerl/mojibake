/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#pragma once

#include "mojibake-internal.h"

static inline bool MJB_USED mjb_malformed_policy_is_valid(mjb_malformed_policy policy) {
    return policy == MJB_MALFORMED_STOP || policy == MJB_MALFORMED_REPLACE ||
        policy == MJB_MALFORMED_SKIP;
}

static inline void MJB_USED mjb_diagnostic_reset(mjb_diagnostic *diagnostic) {
    if(diagnostic == NULL) {
        return;
    }

    diagnostic->error = MJB_TEXT_ERROR_NONE;
    diagnostic->byte_offset = 0;
    diagnostic->byte_length = 0;
    diagnostic->code_unit_offset = 0;
}

static inline void MJB_USED mjb_diagnostic_record(mjb_diagnostic *destination,
    const mjb_diagnostic *source) {
    if(destination != NULL && source != NULL && destination->error == MJB_TEXT_ERROR_NONE &&
        source->error != MJB_TEXT_ERROR_NONE) {
        *destination = *source;
    }
}

static inline void MJB_USED mjb_diagnose_malformed_input(const char *buffer, size_t byte_length,
    mjb_encoding encoding, mjb_status status, mjb_diagnostic *diagnostic) {
    if(status == MJB_STATUS_MALFORMED_INPUT && diagnostic != NULL &&
        diagnostic->error == MJB_TEXT_ERROR_NONE) {
        mjb_text text = mjb_text_make(buffer, byte_length, encoding);
        mjb_status validation_status = mjb_validate_encoding(&text, diagnostic);
        (void)validation_status;
    }
}

/**
 * Repair input only after a strict traversal has found malformed text. Valid input therefore pays
 * no separate validation cost.
 */
static inline mjb_status MJB_USED mjb_repair_text_input(const char **buffer, size_t *byte_length,
    mjb_encoding *encoding, mjb_malformed_policy malformed_policy, mjb_diagnostic *diagnostic,
    mjb_result *sanitized) {
    if(malformed_policy == MJB_MALFORMED_STOP) {
        mjb_diagnose_malformed_input(*buffer, *byte_length, *encoding, MJB_STATUS_MALFORMED_INPUT,
            diagnostic);

        return MJB_STATUS_MALFORMED_INPUT;
    }

    mjb_text text = mjb_text_make(*buffer, *byte_length, *encoding);
    mjb_status status = mjb_convert_encoding(&text, malformed_policy, MJB_ENC_UTF_8, sanitized,
        diagnostic);

    if(status == MJB_STATUS_OK) {
        *buffer = sanitized->output;
        *byte_length = sanitized->output_size;
        *encoding = MJB_ENC_UTF_8;
    }

    return status;
}

/**
 * Validate one input of a function that failed with MJB_STATUS_MALFORMED_INPUT. A well-formed text
 * is used as-is. A malformed one records its diagnostic, then stops or, with MJB_MALFORMED_REPLACE
 * and MJB_MALFORMED_SKIP, is repaired into a UTF-8 copy owned by `sanitized`.
 */
static inline mjb_status MJB_USED mjb_prepare_text_input(const mjb_text *text,
    mjb_malformed_policy malformed_policy, mjb_diagnostic *diagnostic, mjb_text *prepared,
    mjb_result *sanitized) {
    *prepared = *text;

    mjb_diagnostic validity;
    mjb_status status = mjb_validate_encoding(text, &validity);

    if(status != MJB_STATUS_MALFORMED_INPUT) {
        return status;
    }

    mjb_diagnostic_record(diagnostic, &validity);

    if(malformed_policy == MJB_MALFORMED_STOP) {
        return MJB_STATUS_MALFORMED_INPUT;
    }

    status = mjb_convert_encoding(text, malformed_policy, MJB_ENC_UTF_8, sanitized, NULL);

    if(status == MJB_STATUS_OK) {
        *prepared = mjb_text_utf8(sanitized->output, sanitized->output_size);
    }

    return status;
}

static inline bool MJB_USED mjb_starts_with_utf8_bom(const char *buffer, size_t byte_length) {
    return byte_length >= 3 && (uint8_t)buffer[0] == 0xEF && (uint8_t)buffer[1] == 0xBB &&
        (uint8_t)buffer[2] == 0xBF;
}

static inline bool MJB_USED mjb_starts_with_utf16be_bom(const char *buffer, size_t byte_length) {
    return byte_length >= 2 && (uint8_t)buffer[0] == 0xFE && (uint8_t)buffer[1] == 0xFF;
}

static inline bool MJB_USED mjb_starts_with_utf16le_bom(const char *buffer, size_t byte_length) {
    return byte_length >= 2 && (uint8_t)buffer[0] == 0xFF && (uint8_t)buffer[1] == 0xFE;
}

static inline bool MJB_USED mjb_starts_with_utf32be_bom(const char *buffer, size_t byte_length) {
    return byte_length >= 4 && (uint8_t)buffer[0] == 0x00 && (uint8_t)buffer[1] == 0x00 &&
        (uint8_t)buffer[2] == 0xFE && (uint8_t)buffer[3] == 0xFF;
}

static inline bool MJB_USED mjb_starts_with_utf32le_bom(const char *buffer, size_t byte_length) {
    return byte_length >= 4 && (uint8_t)buffer[0] == 0xFF && (uint8_t)buffer[1] == 0xFE &&
        (uint8_t)buffer[2] == 0x00 && (uint8_t)buffer[3] == 0x00;
}

static inline mjb_encoding MJB_USED mjb_resolve_input_encoding(const char *buffer,
    size_t byte_length, mjb_encoding encoding, size_t *index) {
    bool at_start = index != NULL && *index == 0;

    // Consume the BOM at the start of a string and determine the real encoding.
    if(encoding & MJB_ENC_UTF_32) {
        if(mjb_starts_with_utf32be_bom(buffer, byte_length)) {
            if(at_start) {
                *index = 4;
            }

            return MJB_ENC_UTF_32BE;
        }

        if(mjb_starts_with_utf32le_bom(buffer, byte_length)) {
            if(at_start) {
                *index = 4;
            }

            return MJB_ENC_UTF_32LE;
        }

        if((encoding & MJB_ENC_UTF_32BE) && !(encoding & MJB_ENC_UTF_32LE)) {
            return MJB_ENC_UTF_32BE;
        }

        if((encoding & MJB_ENC_UTF_32LE) && !(encoding & MJB_ENC_UTF_32BE)) {
            return MJB_ENC_UTF_32LE;
        }
    }

    // Consume the BOM at the start of a string and determine the real encoding.
    if(encoding & MJB_ENC_UTF_16) {
        if(mjb_starts_with_utf16be_bom(buffer, byte_length)) {
            if(at_start) {
                *index = 2;
            }

            return MJB_ENC_UTF_16BE;
        }

        if(mjb_starts_with_utf16le_bom(buffer, byte_length)) {
            if(at_start) {
                *index = 2;
            }

            return MJB_ENC_UTF_16LE;
        }

        if((encoding & MJB_ENC_UTF_16BE) && !(encoding & MJB_ENC_UTF_16LE)) {
            return MJB_ENC_UTF_16BE;
        }

        if((encoding & MJB_ENC_UTF_16LE) && !(encoding & MJB_ENC_UTF_16BE)) {
            return MJB_ENC_UTF_16LE;
        }
    }

    return encoding;
}

static inline mjb_status MJB_USED mjb_check_input_encoding_byte_order(const char *buffer,
    size_t byte_length, mjb_encoding encoding) {
    size_t index = 0;
    mjb_encoding resolved = mjb_resolve_input_encoding(buffer, byte_length, encoding, &index);

    if((encoding == MJB_ENC_UTF_16 || encoding == MJB_ENC_UTF_32) && resolved == encoding) {
        return MJB_STATUS_INVALID_ENCODING;
    }

    return MJB_STATUS_OK;
}

static inline bool MJB_USED mjb_codepoint_is_surrogate(mjb_codepoint codepoint) {
    return codepoint >= 0xD800 && codepoint <= 0xDFFF;
}

static inline size_t MJB_USED mjb_encoding_code_unit_size(mjb_encoding encoding) {
    if(encoding == MJB_ENC_UTF_16BE || encoding == MJB_ENC_UTF_16LE) {
        return 2;
    }

    if(encoding == MJB_ENC_UTF_32BE || encoding == MJB_ENC_UTF_32LE) {
        return 4;
    }

    return 1;
}

static inline void MJB_USED mjb_set_diagnostic(mjb_diagnostic *diagnostic, mjb_text_error error,
    size_t byte_offset, size_t byte_length, mjb_encoding encoding) {
    if(diagnostic == NULL) {
        return;
    }

    diagnostic->error = error;
    diagnostic->byte_offset = byte_offset;
    diagnostic->byte_length = byte_length;
    diagnostic->code_unit_offset = byte_offset / mjb_encoding_code_unit_size(encoding);
}

static inline uint16_t MJB_USED mjb_read_utf16_unit(const char *buffer, size_t offset,
    bool big_endian) {
    uint16_t first = (uint8_t)buffer[offset];
    uint16_t second = (uint8_t)buffer[offset + 1];

    return big_endian ? (uint16_t)((first << 8) | second) : (uint16_t)(first | (second << 8));
}

static inline uint32_t MJB_USED mjb_read_utf32_unit(const char *buffer, size_t offset,
    bool big_endian) {
    uint32_t b0 = (uint8_t)buffer[offset];
    uint32_t b1 = (uint8_t)buffer[offset + 1];
    uint32_t b2 = (uint8_t)buffer[offset + 2];
    uint32_t b3 = (uint8_t)buffer[offset + 3];

    if(big_endian) {
        return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
    }

    return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

/**
 * Decode the scalar value at |*offset| and advance past it. |buffer| and |byte_length| are already
 * resolved: no MJB_NUL_TERMINATED, and |encoding| is a concrete byte order (see
 * mjb_decode_begin()). A malformed subsequence returns MJB_STATUS_MALFORMED_INPUT, sets
 * |*codepoint| to U+FFFD, and advances past one maximal subpart. Returns MJB_STATUS_END_OF_INPUT at
 * the end and MJB_STATUS_INVALID_ENCODING, without advancing, for an unsupported encoding.
 */
static inline mjb_status MJB_USED mjb_decode_next_raw(const char *buffer, size_t byte_length,
    mjb_encoding encoding, size_t *offset, mjb_codepoint *codepoint, mjb_diagnostic *diagnostic) {
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

        // 0x80..0xBF: a continuation byte cannot start a sequence.
        if(first <= 0xBF) {
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

/**
 * Prepare a buffer for mjb_decode_next_raw(). Pick the concrete byte order of generic UTF-16 and
 * UTF-32 input and, when |index| is 0, skip its byte order mark. Return false when a non-empty
 * generic input has no byte order mark: its byte order is unknown, so the input is malformed.
 */
static inline bool MJB_USED mjb_decode_begin(const char *buffer, size_t byte_length,
    mjb_encoding *encoding, size_t *index) {
    mjb_encoding requested = *encoding;
    *encoding = mjb_resolve_input_encoding(buffer, byte_length, requested, index);

    return byte_length == 0 ||
        !((requested == MJB_ENC_UTF_16 || requested == MJB_ENC_UTF_32) && *encoding == requested);
}

static inline bool MJB_USED mjb_encoding_is_valid_input(mjb_encoding encoding) {
    return encoding == MJB_ENC_ASCII || encoding == MJB_ENC_UTF_8 || encoding == MJB_ENC_UTF_16 ||
        encoding == MJB_ENC_UTF_16BE || encoding == MJB_ENC_UTF_16LE ||
        encoding == MJB_ENC_UTF_32 || encoding == MJB_ENC_UTF_32BE || encoding == MJB_ENC_UTF_32LE;
}

static inline bool MJB_USED mjb_encoding_is_valid_output(mjb_encoding encoding) {
    return encoding == MJB_ENC_ASCII || encoding == MJB_ENC_UTF_8 || encoding == MJB_ENC_UTF_16BE ||
        encoding == MJB_ENC_UTF_16LE || encoding == MJB_ENC_UTF_32BE ||
        encoding == MJB_ENC_UTF_32LE;
}

/**
 * Scan for the encoding-aware terminator of a NUL-terminated string. Kept out of
 * mjb_resolve_input_byte_length so that its unbounded loop does not stop static analyzers from
 * following the common explicit-length path.
 */
static inline mjb_status MJB_USED mjb_scan_nul_terminated_byte_length(const char *buffer,
    size_t *byte_length, mjb_encoding encoding) {
    if(buffer == NULL) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    size_t code_unit_size;

    if(encoding == MJB_ENC_ASCII || encoding == MJB_ENC_UTF_8) {
        code_unit_size = 1;
    } else if(encoding == MJB_ENC_UTF_16 || encoding == MJB_ENC_UTF_16BE ||
        encoding == MJB_ENC_UTF_16LE) {
        code_unit_size = 2;
    } else if(encoding == MJB_ENC_UTF_32 || encoding == MJB_ENC_UTF_32BE ||
        encoding == MJB_ENC_UTF_32LE) {
        code_unit_size = 4;
    } else {
        return MJB_STATUS_INVALID_ENCODING;
    }

    size_t index = 0;

    for(;;) {
        bool terminated = true;

        for(size_t i = 0; i < code_unit_size; ++i) {
            if(buffer[index + i] != '\0') {
                terminated = false;
                break;
            }
        }

        if(terminated) {
            *byte_length = index;

            return MJB_STATUS_OK;
        }

        index += code_unit_size;
    }
}

/**
 * Resolve MJB_NUL_TERMINATED to an encoding-aware payload byte length. The terminator itself is not
 * included. Explicit byte lengths are left unchanged and may contain U+0000 codepoints.
 */
static inline mjb_status MJB_USED mjb_resolve_input_byte_length(const char *buffer,
    size_t *byte_length, mjb_encoding encoding) {
    if(byte_length == NULL) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    if(*byte_length != MJB_NUL_TERMINATED) {
        return MJB_STATUS_OK;
    }

    return mjb_scan_nul_terminated_byte_length(buffer, byte_length, encoding);
}
