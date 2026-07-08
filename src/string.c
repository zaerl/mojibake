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

// Internal function.
MJB_EXPORT char *mjb_string_output(char *ret, char *input, size_t input_size, size_t *output_index,
    size_t *output_size) {
    if(ret == NULL || (input == NULL && input_size > 0) || output_index == NULL || output_size == NULL) {
        return NULL;
    }

    if(!input_size) {
        return NULL;
    }

    if(*output_index + input_size >= *output_size) {
        size_t required = *output_index + input_size + 1;
        size_t doubled = *output_size * 2;
        size_t new_output_size = (doubled > required) ? doubled : required;
        char *new_ret = (char*)mjb_realloc(ret, new_output_size);

        if(new_ret == NULL) {
            return NULL;
        }

        ret = new_ret;
        *output_size = new_output_size;
    }

    memcpy((char*)ret + *output_index, input, input_size);
    *output_index += input_size;

    // Null-terminate the string. jemalloc reuses freed heap blocks without clearing them.
    ret[*output_index] = '\0';

    return ret;
}

// Internal function.
MJB_EXPORT char *mjb_string_output_codepoint(mjb_codepoint codepoint, char *output,
    size_t *output_index, size_t *output_size, mjb_encoding encoding) {
    // Shortcut for mjb_codepoint_encode + mjb_string_output
    char buffer[5];
    size_t utf_size = mjb_codepoint_encode(codepoint, (char*)buffer, 5, encoding);

    return mjb_string_output(output, buffer, utf_size, output_index, output_size);
}

/**
 * Return size of a string.
 */
MJB_EXPORT size_t mjb_string_length(const char *buffer, size_t max_length, mjb_encoding encoding) {
    if(buffer == NULL || max_length == 0) {
        return 0;
    }

    uint8_t state = MJB_UTF_ACCEPT;
    bool in_error = false;
    mjb_codepoint codepoint;
    size_t count = 0;

    for(size_t i = 0; i < max_length; ) {
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
