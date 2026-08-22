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
    mjb_malformed_policy malformed_policy;
    mjb_filter_flags filters;
    mjb_encoding output_encoding;
    mjb_diagnostic *diagnostic;
} mjb_filter_context;

static mjb_status mjb_filter_process(const mjb_filter_context *context, mjb_output *output,
    bool *transformed) {
    mjb_codepoint codepoint = 0;
    mjb_character character;
    bool last_was_whitespace = false;
    size_t combining_mark_count = 0;
    bool any_transformation = false;

    for(size_t offset = 0;;) {
        mjb_diagnostic current;
        mjb_status decode_status = mjb_decode_next(context->buffer, context->byte_length,
            context->encoding, context->malformed_policy, &offset, &codepoint, &current);

        if(current.error != MJB_TEXT_ERROR_NONE) {
            any_transformation = true;
            mjb_diagnostic_record(context->diagnostic, &current);
        }

        if(decode_status == MJB_STATUS_END_OF_INPUT) {
            break;
        }

        if(decode_status != MJB_STATUS_OK) {
            return decode_status;
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

        if((context->filters & MJB_FILTER_NUMERIC) && character.decimal != MJB_NUMBER_NOT_VALID) {
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

    if(transformed != NULL) {
        *transformed = any_transformation;
    }

    return MJB_STATUS_OK;
}

static mjb_status mjb_filter_write(mjb_output *output, const void *context) {
    return mjb_filter_process((const mjb_filter_context *)context, output, NULL);
}

MJB_EXPORT mjb_status mjb_filter(const char *buffer, size_t byte_length, mjb_encoding encoding,
    mjb_malformed_policy malformed_policy, mjb_filter_flags filters, mjb_encoding output_encoding,
    mjb_result *result, mjb_diagnostic *diagnostic) {
    if(result == NULL || (buffer == NULL && byte_length > 0) ||
        !mjb_malformed_policy_is_valid(malformed_policy)) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    mjb_diagnostic_reset(diagnostic);

    if(!mjb_encoding_is_valid_input(encoding) || !mjb_encoding_is_valid_output(output_encoding)) {
        return MJB_STATUS_INVALID_ENCODING;
    }

    mjb_status status = mjb_resolve_input_byte_length(buffer, &byte_length, encoding);

    if(status != MJB_STATUS_OK) {
        return status;
    }

    if(byte_length == 0) {
        result->output = (char *)buffer;
        result->output_size = byte_length;
        result->transformed = false;

        return MJB_STATUS_OK;
    }

    bool is_normalized = false;

    if(filters & MJB_FILTER_NORMALIZE) {
        mjb_encoding normalize_output_encoding = filters == MJB_FILTER_NORMALIZE ? output_encoding :
                                                                                   encoding;
        status = mjb_normalize(buffer, byte_length, encoding, malformed_policy,
            MJB_NORMALIZATION_NFC, normalize_output_encoding, result, diagnostic);

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
    mjb_filter_context context = { buffer, byte_length, encoding, malformed_policy, filters,
        output_encoding, diagnostic };
    bool transformed = false;
    status = mjb_filter_process(&context, &output, &transformed);

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

MJB_EXPORT mjb_status mjb_filter_into(const char *buffer, size_t byte_length, mjb_encoding encoding,
    mjb_malformed_policy malformed_policy, mjb_filter_flags filters, mjb_encoding output_encoding,
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

    if(!mjb_encoding_is_valid_input(encoding) || !mjb_encoding_is_valid_output(output_encoding)) {
        *output_size = 0;

        return MJB_STATUS_INVALID_ENCODING;
    }

    mjb_status status = mjb_resolve_input_byte_length(buffer, &byte_length, encoding);

    if(status != MJB_STATUS_OK) {
        *output_size = 0;

        return status;
    }

    if(byte_length == 0) {
        return mjb_output_copy_into(buffer, byte_length, output, output_size);
    }

    bool normalization_requested = (filters & MJB_FILTER_NORMALIZE) != 0;

    if(filters == MJB_FILTER_NORMALIZE) {
        status = mjb_normalize_into(buffer, byte_length, encoding, malformed_policy,
            MJB_NORMALIZATION_NFC, output_encoding, output, output_size, diagnostic);

        return status;
    }

    mjb_result normalized = { NULL, 0, false };

    if(normalization_requested) {
        status = mjb_normalize(buffer, byte_length, encoding, malformed_policy,
            MJB_NORMALIZATION_NFC, encoding, &normalized, diagnostic);

        if(status != MJB_STATUS_OK) {
            *output_size = 0;

            return status;
        }

        buffer = normalized.output;
        byte_length = normalized.output_size;
    }

    mjb_filter_context context = { buffer, byte_length, encoding, malformed_policy, filters,
        output_encoding, diagnostic };
    status = mjb_output_into(output, output_size, mjb_filter_write, &context);

    if(normalization_requested) {
        mjb_result_free(&normalized);
    }

    return status;
}
