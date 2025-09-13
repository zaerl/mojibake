/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <string.h>

#include "mojibake-internal.h"
#include "utf8.h"
#include "utf16.h"

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
    size_t *output_index, size_t *output_size) {
    // Shortcut for mjb_codepoint_encode + mjb_string_output
    char buffer_utf8[5];
    size_t utf8_size = mjb_codepoint_encode(codepoint, (char*)buffer_utf8, 5, MJB_ENCODING_UTF_8);

    return mjb_string_output(output, buffer_utf8, utf8_size, output_index, output_size);
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

    if(encoding == MJB_ENCODING_UTF_8) {
        for(size_t i = 0; i < max_length && buffer[i]; ++i) {
            // Find next codepoint.
            if(encoding == MJB_ENCODING_UTF_8) {
                state = mjb_utf8_decode_step(state, buffer[i], &codepoint);
            } else {
                state = mjb_utf16_decode_step(state, buffer[i], buffer[i + 1], &codepoint,
                    encoding == MJB_ENCODING_UTF_16_BE);
                ++i;
            }

            if(state == MJB_UTF_ACCEPT) {
                ++count;
            }
        }
    } else if(encoding == MJB_ENCODING_UTF_16_LE || encoding == MJB_ENCODING_UTF_16_BE) {
        state = MJB_UTF_ACCEPT;

        for(size_t i = 0; i < max_length; i += 2) {
            state = mjb_utf16_decode_step(state, buffer[i], buffer[i + 1], &codepoint,
                encoding == MJB_ENCODING_UTF_16_BE);

            if(state == MJB_UTF_ACCEPT) {
                ++count;
            }
        }
    }

    return count;
}

MJB_EXPORT size_t mjb_strncmp(const char *s1, const char *s2, size_t max_length,
    mjb_encoding encoding) {
    if(s1 == 0 || s2 == 0 || encoding != MJB_ENCODING_UTF_8) {
        // We return 0 to indicate an error. But the behavior is undefined.
        return 0;
    }

    return 0;
}
