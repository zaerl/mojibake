/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "mojibake-internal.h"
#include "utf.h"

typedef struct mjb_filter_context {
    const char *buffer;
    size_t byte_length;
    mjb_encoding encoding;
    mjb_filter_flags filters;
    mjb_encoding output_encoding;
} mjb_filter_context;

static mjb_status mjb_filter_process(const mjb_filter_context *context, mjb_output *output,
    bool *transformed) {
    uint8_t state = MJB_UTF_ACCEPT;
    mjb_codepoint codepoint = 0;
    mjb_character character;
    bool last_was_whitespace = false;
    size_t combining_mark_count = 0;
    bool any_transformation = false;
    bool in_error = false;

    for(size_t i = 0; i < context->byte_length;) {
        mjb_decode_result decode_status = mjb_next_codepoint(context->buffer,
            context->byte_length, &state, &i, context->encoding, &codepoint, &in_error);

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

        mjb_codepoint original_codepoint = codepoint;

        // Check if current codepoint is whitespace.
        bool is_whitespace = (character.category == MJB_CATEGORY_ZS ||
            character.category == MJB_CATEGORY_ZL || character.category == MJB_CATEGORY_ZP ||
            codepoint == 0x09 || // Tab
            codepoint == 0x0A || // Line feed
            codepoint == 0x0B || // Vertical tab
            codepoint == 0x0C || // Form feed
            codepoint == 0x0D);  // Carriage return
        bool is_combining = mjb_category_is_combining(character.category);

        if(context->filters & MJB_FILTER_CONTROLS) {
            if(character.category == MJB_CATEGORY_CC && codepoint != 0x09 && // Tab
                codepoint != 0x0A &&                                         // Line feed
                codepoint != 0x0B &&                                         // Vertical tab
                codepoint != 0x0C &&                                         // Form feed
                codepoint != 0x0D) {                                         // Carriage return
                any_transformation = true;
                continue;
            }
        }

        if(context->filters & MJB_FILTER_SPACES) {
            if(is_whitespace) {
                // Transform all space characters to ASCII space.
                codepoint = 0x20;

                if(original_codepoint != codepoint) {
                    any_transformation = true;
                }
            }
        }

        if(context->filters & MJB_FILTER_COLLAPSE_SPACES) {
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

        if((context->filters & MJB_FILTER_NUMERIC) &&
            character.decimal != MJB_NUMBER_NOT_VALID) {
            codepoint = 0x30 + character.decimal; // U+0030 DIGIT ZERO

            if(original_codepoint != codepoint) {
                any_transformation = true;
            }
        }

        if(context->filters & MJB_FILTER_LIMIT_COMBINING) {
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

        mjb_status status = mjb_output_codepoint(output, codepoint, context->output_encoding);

        if(status != MJB_STATUS_OK) {
            return status;
        }

        last_was_whitespace = is_whitespace;
    }

    if(mjb_utf_state_is_incomplete(state)) {
        // Incomplete multibyte sequence at end of string
        if(!in_error) {
            mjb_status status = mjb_output_codepoint(output, MJB_CODEPOINT_REPLACEMENT,
                context->output_encoding);

            if(status != MJB_STATUS_OK) {
                return status;
            }

            any_transformation = true;
        }
    }

    if(transformed != NULL) {
        *transformed = any_transformation;
    }

    return MJB_STATUS_OK;
}

static mjb_status mjb_filter_write(mjb_output *output, const void *context) {
    return mjb_filter_process((const mjb_filter_context *)context, output, NULL);
}

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
        mjb_encoding normalize_output_encoding = filters == MJB_FILTER_NORMALIZE ?
            output_encoding : encoding;
        mjb_status status = mjb_normalize(buffer, byte_length, encoding, MJB_NORMALIZATION_NFC,
            normalize_output_encoding, result);

        if(status != MJB_STATUS_OK) {
            return status;
        }

        is_normalized = result->transformed;
    }

    if(filters == MJB_FILTER_NORMALIZE) {
        return MJB_STATUS_OK;
    }

    if(is_normalized) {
        buffer = result->output;
        byte_length = result->output_size;
    }

    char *allocated = (char *)mjb_alloc(byte_length);

    if(allocated == NULL) {
        if(is_normalized) {
            mjb_result_free(result);
        }

        return MJB_STATUS_NO_MEMORY;
    }

    mjb_output output;
    mjb_output_init_dynamic(&output, allocated, byte_length);
    mjb_filter_context context = {
        buffer, byte_length, encoding, filters, output_encoding
    };
    bool transformed = false;
    mjb_status status = mjb_filter_process(&context, &output, &transformed);

    if(status != MJB_STATUS_OK) {
        mjb_free(output.buffer);

        if(is_normalized) {
            mjb_result_free(result);
        }

        // Preserve the allocating API's historical status for unrepresentable output.
        return status == MJB_STATUS_UNSUPPORTED ? MJB_STATUS_NO_MEMORY : status;
    }

    // If no transformation occurred, not normalized and output encoding matches input encoding,
    // return original buffer.
    if(!transformed && !is_normalized && encoding == output_encoding) {
        mjb_free(output.buffer);

        result->output = (char *)buffer;
        result->output_size = byte_length;
        result->transformed = false;

        return MJB_STATUS_OK;
    }

    if(is_normalized) {
        mjb_result_free(result);
    }

    result->output = output.buffer;
    result->output_size = output.size;
    result->transformed = true;

    return MJB_STATUS_OK;
}

MJB_EXPORT mjb_status mjb_filter_into(const char *buffer, size_t byte_length,
    mjb_encoding encoding, mjb_filter_flags filters, mjb_encoding output_encoding, void *output,
    size_t *output_size) {
    if(output_size == NULL) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    if(buffer == NULL && byte_length > 0) {
        *output_size = 0;

        return MJB_STATUS_INVALID_ARGUMENT;
    }

    if(byte_length == 0) {
        return mjb_output_copy_into(buffer, byte_length, output, output_size);
    }

    if(filters == MJB_FILTER_NORMALIZE) {
        return mjb_normalize_into(buffer, byte_length, encoding, MJB_NORMALIZATION_NFC,
            output_encoding, output, output_size);
    }

    mjb_result normalized = { NULL, 0, false };
    bool normalization_requested = (filters & MJB_FILTER_NORMALIZE) != 0;

    if(normalization_requested) {
        mjb_status status = mjb_normalize(buffer, byte_length, encoding, MJB_NORMALIZATION_NFC,
            encoding, &normalized);

        if(status != MJB_STATUS_OK) {
            *output_size = 0;

            return status;
        }

        buffer = normalized.output;
        byte_length = normalized.output_size;
    }

    mjb_filter_context context = {
        buffer, byte_length, encoding, filters, output_encoding
    };
    mjb_status status = mjb_output_into(output, output_size, mjb_filter_write, &context);

    if(normalization_requested) {
        mjb_result_free(&normalized);
    }

    return status;
}
