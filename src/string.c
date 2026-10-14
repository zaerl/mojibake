/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <string.h>

#include "mojibake-internal.h"
#include "utf.h"

typedef struct mjb_output_copy_context {
    const void *buffer;
    size_t byte_length;
} mjb_output_copy_context;

// Internal function.
char *mjb_string_output(char *ret, const char *input, size_t input_size, size_t *output_index,
    size_t *output_size) {
    if(ret == NULL || (input == NULL && input_size > 0) || output_index == NULL ||
        output_size == NULL) {
        return NULL;
    }

    if(!input_size) {
        return NULL;
    }

    // Leave room for the terminator without allowing either addition below to wrap.
    if(input_size >= SIZE_MAX - *output_index) {
        return NULL;
    }

    if(*output_index + input_size >= *output_size) {
        size_t required = *output_index + input_size + 1;
        size_t doubled = *output_size > SIZE_MAX / 2 ? SIZE_MAX : *output_size * 2;
        size_t new_output_size = (doubled > required) ? doubled : required;
        char *new_ret = (char *)mjb_realloc(ret, new_output_size);

        if(new_ret == NULL) {
            return NULL;
        }

        ret = new_ret;
        *output_size = new_output_size;
    }

    memcpy(ret + *output_index, input, input_size);
    *output_index += input_size;

    // Null-terminate the string. jemalloc reuses freed heap blocks without clearing them.
    ret[*output_index] = '\0';

    return ret;
}

// Internal function.
char *mjb_string_output_codepoint(mjb_codepoint codepoint, char *output, size_t *output_index,
    size_t *output_size, mjb_encoding encoding) {
    // Shortcut for mjb_codepoint_encode + mjb_string_output
    char buffer[5];
    size_t utf_size = mjb_codepoint_encode(codepoint, (char *)buffer, 5, encoding);

    return mjb_string_output(output, buffer, utf_size, output_index, output_size);
}

void mjb_output_init_dynamic(mjb_output *output, char *buffer, size_t capacity) {
    output->buffer = buffer;
    output->size = 0;
    output->capacity = capacity;
    output->mode = MJB_OUTPUT_DYNAMIC;
}

mjb_status mjb_output_write(mjb_output *output, const void *buffer, size_t byte_length) {
    if(output == NULL || (buffer == NULL && byte_length > 0)) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    if(output->size > SIZE_MAX - byte_length) {
        return MJB_STATUS_OVERFLOW;
    }

    if(byte_length == 0) {
        return MJB_STATUS_OK;
    }

    if(output->mode == MJB_OUTPUT_DYNAMIC) {
        char *new_buffer = mjb_string_output(output->buffer, (char *)buffer, byte_length,
            &output->size, &output->capacity);

        if(new_buffer == NULL) {
            return MJB_STATUS_NO_MEMORY;
        }

        output->buffer = new_buffer;

        return MJB_STATUS_OK;
    }

    if(output->mode == MJB_OUTPUT_FIXED) {
        if(output->buffer == NULL || output->size > output->capacity ||
            output->capacity - output->size < byte_length) {
            return MJB_STATUS_OUTPUT_TOO_SMALL;
        }

        memmove(output->buffer + output->size, buffer, byte_length);
    } else if(output->mode == MJB_OUTPUT_BOUNDED) {
        if(output->capacity - output->size < byte_length) {
            // Stop writing at the first chunk that does not fit, so a codepoint is never split
            // and the written bytes stay a prefix of the output. Keep counting the required size.
            output->mode = MJB_OUTPUT_MEASURE;
        } else {
            memmove(output->buffer + output->size, buffer, byte_length);
        }
    }

    output->size += byte_length;

    return MJB_STATUS_OK;
}

mjb_status mjb_output_codepoint(mjb_output *output, mjb_codepoint codepoint,
    mjb_encoding encoding) {
    char buffer[5];
    size_t byte_length = mjb_codepoint_encode(codepoint, buffer, sizeof(buffer), encoding);

    if(byte_length == 0) {
        return MJB_STATUS_UNSUPPORTED;
    }

    return mjb_output_write(output, buffer, byte_length);
}

mjb_status mjb_output_into(void *output, size_t *output_size, mjb_output_writer writer,
    const void *context) {
    if(output_size == NULL || writer == NULL) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    size_t capacity = output == NULL ? 0 : *output_size;
    *output_size = 0;

    // Single pass: write while the output fits and keep measuring after it is full. With a NULL
    // output only measure.
    mjb_output bounded = { (char *)output, 0, capacity,
        output == NULL ? MJB_OUTPUT_MEASURE : MJB_OUTPUT_BOUNDED };
    mjb_status status = writer(&bounded, context);

    if(status != MJB_STATUS_OK) {
        return status;
    }

    *output_size = bounded.size;

    if(output != NULL && bounded.size > capacity) {
        return MJB_STATUS_OUTPUT_TOO_SMALL;
    }

    return MJB_STATUS_OK;
}

static mjb_status mjb_output_copy_writer(mjb_output *output, const void *context_pointer) {
    const mjb_output_copy_context *context = (const mjb_output_copy_context *)context_pointer;

    return mjb_output_write(output, context->buffer, context->byte_length);
}

mjb_status mjb_output_copy_into(const void *buffer, size_t byte_length, void *output,
    size_t *output_size) {
    mjb_output_copy_context context = { buffer, byte_length };

    return mjb_output_into(output, output_size, mjb_output_copy_writer, &context);
}

/**
 * Count the codepoints in a string.
 */
MJB_EXPORT mjb_status mjb_codepoint_count(const mjb_text *text,
    mjb_malformed_policy malformed_policy, size_t *count, mjb_diagnostic *diagnostic) {
    if(count == NULL) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    *count = 0;
    mjb_diagnostic_reset(diagnostic);

    if(text == NULL || !mjb_malformed_policy_is_valid(malformed_policy)) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    const char *buffer = text->buffer;
    size_t byte_length = text->byte_length;
    mjb_encoding encoding = text->encoding;

    if(!mjb_encoding_is_valid_input(encoding)) {
        return MJB_STATUS_INVALID_ENCODING;
    }

    if(byte_length == 0) {
        return MJB_STATUS_OK;
    }

    if(buffer == NULL) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    mjb_status status = mjb_resolve_input_byte_length(buffer, &byte_length, encoding);

    if(status != MJB_STATUS_OK || byte_length == 0) {
        return status;
    }

    mjb_text resolved = mjb_text_make(buffer, byte_length, encoding);
    mjb_codepoint codepoint = 0;
    size_t codepoint_count = 0;
    size_t offset = 0;

    for(;;) {
        mjb_diagnostic current;
        mjb_status decode_status = mjb_decode_next(&resolved, malformed_policy, &offset, &codepoint,
            &current);
        mjb_diagnostic_record(diagnostic, &current);

        if(decode_status == MJB_STATUS_END_OF_INPUT) {
            break;
        }

        if(decode_status != MJB_STATUS_OK) {
            return decode_status;
        }

        ++codepoint_count;
    }

    *count = codepoint_count;

    return MJB_STATUS_OK;
}
