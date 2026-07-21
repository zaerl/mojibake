/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "mojibake-internal.h"
#include "utf.h"

extern mojibake mjb_global;

MJB_EXPORT mjb_status mjb_filter(const char *buffer, size_t byte_length, mjb_encoding encoding,
    mjb_filter_flags filters, mjb_encoding output_encoding, mjb_result *result) {
    if(result == NULL || (buffer == NULL && byte_length > 0)) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    if(byte_length == 0) {
        result->output = (char *)buffer;
        result->output_size = byte_length;
        result->transformed = false;

        return MJB_STATUS_OK;
    }

    bool is_normalized = false;

    if(filters & MJB_FILTER_NORMALIZE) {
        // Use the output encoding if only the normalization filter is applied.
        // clang-format off
        mjb_encoding normalize_output_encoding = filters == MJB_FILTER_NORMALIZE ? output_encoding :
            encoding;
        // clang-format on

        mjb_status status = mjb_normalize(buffer, byte_length, encoding, MJB_NORMALIZATION_NFC,
            normalize_output_encoding, result);

        if(status != MJB_STATUS_OK) {
            return status;
        }

        is_normalized = result->transformed;
    }

    // Only normalize, no filtering.
    if(filters == MJB_FILTER_NORMALIZE) {
        return MJB_STATUS_OK;
    }

    if(is_normalized) {
        buffer = result->output;
        byte_length = result->output_size;
    }

    uint8_t state = MJB_UTF_ACCEPT;
    mjb_codepoint codepoint = 0;
    mjb_codepoint original_codepoint = 0;
    mjb_character character;

    char *output = (char *)mjb_alloc(byte_length);
    size_t output_size = byte_length;
    size_t output_index = 0;
    bool last_was_whitespace = false;
    size_t combining_mark_count = 0;
    bool any_transformation = false;

    if(output == NULL) {
        if(is_normalized) {
            mjb_free(result->output);
        }

        return MJB_STATUS_NO_MEMORY;
    }

    bool in_error = false;

    for(size_t i = 0; i < byte_length;) {
        mjb_decode_result decode_status = mjb_next_codepoint(buffer, byte_length, &state, &i,
            encoding, &codepoint, &in_error);

        if(decode_status == MJB_DECODE_END) {
            break;
        }

        if(decode_status == MJB_DECODE_INCOMPLETE) {
            continue;
        }

        if(decode_status == MJB_DECODE_ERROR) {
            any_transformation = true;
        }

        // Get current character.
        if(mjb_codepoint_info(codepoint, &character) != MJB_STATUS_OK) {
            continue;
        }

        original_codepoint = codepoint;

        // Check if current codepoint is whitespace.
        bool is_whitespace = (character.category == MJB_CATEGORY_ZS ||
            character.category == MJB_CATEGORY_ZL || character.category == MJB_CATEGORY_ZP ||
            codepoint == 0x09 || // Tab
            codepoint == 0x0A || // Line feed
            codepoint == 0x0B || // Vertical tab
            codepoint == 0x0C || // Form feed
            codepoint == 0x0D);  // Carriage return
        bool is_combining = mjb_category_is_combining(character.category);

        if(filters & MJB_FILTER_CONTROLS) {
            if(character.category == MJB_CATEGORY_CC && codepoint != 0x09 && // Tab
                codepoint != 0x0A &&                                         // Line feed
                codepoint != 0x0B &&                                         // Vertical tab
                codepoint != 0x0C &&                                         // Form feed
                codepoint != 0x0D) {                                         // Carriage return
                any_transformation = true;
                continue;
            }
        }

        if(filters & MJB_FILTER_SPACES) {
            if(is_whitespace) {
                // Transform all space characters to ASCII space.
                codepoint = 0x20;

                if(original_codepoint != codepoint) {
                    any_transformation = true;
                }
            }
        }

        if(filters & MJB_FILTER_COLLAPSE_SPACES) {
            if(is_whitespace) {
                // Skip consecutive whitespace.
                if(last_was_whitespace) {
                    any_transformation = true;
                    continue;
                }

                // Convert all whitespace to ASCII space.
                if(codepoint != 0x20) {
                    any_transformation = true;
                }

                codepoint = 0x20;
            }
        }

        if((filters & MJB_FILTER_NUMERIC) && character.decimal != MJB_NUMBER_NOT_VALID) {
            codepoint = 0x30 + character.decimal; // U+0030 DIGIT ZERO

            if(original_codepoint != codepoint) {
                any_transformation = true;
            }
        }

        if(filters & MJB_FILTER_LIMIT_COMBINING) {
            if(is_combining) {
                if(combining_mark_count >= MJB_FILTER_MAX_COMBINING_MARKS) {
                    any_transformation = true;
                    continue;
                }

                ++combining_mark_count;
            } else {
                combining_mark_count = 0;
            }
        }

        char *new_output = mjb_string_output_codepoint(codepoint, output, &output_index,
            &output_size, output_encoding);

        if(new_output != NULL) {
            output = new_output;
        } else {
            mjb_free(output);

            if(is_normalized) {
                mjb_free(result->output);
            }

            return MJB_STATUS_NO_MEMORY;
        }

        last_was_whitespace = is_whitespace;
    }

    if(mjb_utf_state_is_incomplete(state)) {
        // Incomplete multibyte sequence at end of string
        if(!in_error) {
            char *new_output = mjb_string_output_codepoint(MJB_CODEPOINT_REPLACEMENT, output,
                &output_index, &output_size, output_encoding);

            if(new_output != NULL) {
                output = new_output;
            } else {
                mjb_free(output);

                if(is_normalized) {
                    mjb_free(result->output);
                }

                return MJB_STATUS_NO_MEMORY;
            }

            any_transformation = true;
        }
    }

    // If no transformation occurred, not normalized and output encoding matches input encoding,
    // return original buffer.
    if(!any_transformation && !is_normalized && encoding == output_encoding) {
        mjb_free(output);

        result->output = (char *)buffer;
        result->output_size = byte_length;
        result->transformed = false;

        return MJB_STATUS_OK;
    }

    if(is_normalized) {
        mjb_result_free(result);
    }

    result->output = output;
    result->output_size = output_index;
    result->transformed = true;

    return MJB_STATUS_OK;
}
