/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <string.h>

#include "mojibake-internal.h"
#include "utf.h"

extern mojibake mjb_global;

typedef struct mjb_output_copy_context {
    const void *buffer;
    size_t byte_length;
} mjb_output_copy_context;

// Internal function.
char *mjb_string_output(char *ret, char *input, size_t input_size, size_t *output_index,
    size_t *output_size) {
    if(ret == NULL || (input == NULL && input_size > 0) || output_index == NULL ||
        output_size == NULL) {
        return NULL;
    }

    if(!input_size) {
        return NULL;
    }

    if(*output_index + input_size >= *output_size) {
        size_t required = *output_index + input_size + 1;
        size_t doubled = *output_size * 2;
        size_t new_output_size = (doubled > required) ? doubled : required;
        char *new_ret = (char *)mjb_realloc(ret, new_output_size);

        if(new_ret == NULL) {
            return NULL;
        }

        ret = new_ret;
        *output_size = new_output_size;
    }

    memcpy((char *)ret + *output_index, input, input_size);
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

    mjb_output measured = { NULL, 0, 0, MJB_OUTPUT_MEASURE };
    mjb_status status = writer(&measured, context);

    if(status != MJB_STATUS_OK) {
        return status;
    }

    *output_size = measured.size;

    if(output == NULL) {
        return MJB_STATUS_OK;
    }

    if(capacity < measured.size) {
        return MJB_STATUS_OUTPUT_TOO_SMALL;
    }

    mjb_output fixed = { (char *)output, 0, capacity, MJB_OUTPUT_FIXED };
    status = writer(&fixed, context);

    if(status != MJB_STATUS_OK) {
        *output_size = 0;

        return status;
    }

    *output_size = fixed.size;

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
 * Return size of a string.
 */
MJB_EXPORT size_t mjb_count_codepoints(const char *buffer, size_t max_length,
    mjb_encoding encoding) {
    if(buffer == NULL || max_length == 0) {
        return 0;
    }

    if(mjb_resolve_input_byte_length(buffer, &max_length, encoding) != MJB_STATUS_OK) {
        return 0;
    }

    uint8_t state = MJB_UTF_ACCEPT;
    bool in_error = false;
    mjb_codepoint codepoint;
    size_t count = 0;

    for(size_t i = 0; i < max_length;) {
        mjb_decode_result decode_status = mjb_next_codepoint(buffer, max_length, &state, &i,
            encoding, &codepoint, &in_error);

        if(decode_status == MJB_DECODE_END) {
            break;
        }

        if(decode_status == MJB_DECODE_OK || decode_status == MJB_DECODE_ERROR) {
            ++count;
        }
    }

    return count;
}
