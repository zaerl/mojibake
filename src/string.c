/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <string.h>

#include "mojibake-internal.h"
#include "utf.h"

extern mojibake mjb_global;

// Internal function.
MJB_EXPORT char *mjb_string_output(char *ret, char *input, size_t input_size, size_t *output_index,
    size_t *output_size) {
    if(!input_size) {
        return NULL;
    }

    if(*output_index + input_size > *output_size) {
        *output_size *= 2;
        ret = (char*)mjb_realloc(ret, *output_size);
    }

    memcpy((char*)ret + *output_index, input, input_size);
    *output_index += input_size;

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
MJB_EXPORT size_t mjb_strnlen(const char *buffer, size_t max_length, mjb_encoding encoding) {
    if(buffer == 0 || max_length == 0) {
        return 0;
    }

    uint8_t state = MJB_UTF_ACCEPT;
    mjb_codepoint codepoint;
    size_t count = 0;

    for(size_t i = 0; i < max_length; ++i) {
        // Find next codepoint.
        if(!mjb_decode_step(buffer, max_length, &state, &i, encoding, &codepoint)) {
            break;
        }

        if(state == MJB_UTF_ACCEPT) {
            ++count;
        }
    }

    return count;
}

MJB_EXPORT size_t mjb_strncmp(const char *s1, const char *s2, size_t max_length,
    mjb_encoding encoding) {
    if(s1 == 0 || s2 == 0) {
        // We return 0 to indicate an error. But the behavior is undefined.
        return 0;
    }

    return 0;
}
