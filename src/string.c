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

MJB_EXPORT char *mjb_string_output(char *ret, char *input, size_t input_size, size_t *output_index, size_t *output_size) {
    if(!input_size) {
        return NULL;
    }

    if(*output_index + input_size > *output_size) {
        *output_size *= 2;
        ret = mjb_realloc(ret, *output_size);
    }

    memcpy((char*)ret + *output_index, input, input_size);
    *output_index += input_size;

    return ret;
}

/**
 * Return size of a string.
 */
MJB_EXPORT size_t mjb_strnlen(const char *buffer, size_t max_length, mjb_encoding encoding) {
    if(buffer == 0 || max_length == 0) {
        return 0;
    }

    uint8_t state = MJB_UTF8_ACCEPT;
    mjb_codepoint codepoint;
    size_t count = 0;

    if(encoding == MJB_ENCODING_UTF_8) {
        const char *current = buffer;

        while(*current && (size_t)(current - buffer) < max_length) {
            state = mjb_utf8_decode_step(state, *current, &codepoint);

            if(state == MJB_UTF8_ACCEPT) {
                ++count;
            }

            ++current;
        }
    } else if(encoding == MJB_ENCODING_UTF_16_LE || encoding == MJB_ENCODING_UTF_16_BE) {
        state = MJB_UTF16_ACCEPT;
        const uint8_t *bytes = (const uint8_t *)buffer;

        for(size_t i = 0; i < max_length; i += 2) {
            uint16_t unit = encoding == MJB_ENCODING_UTF_16_BE ?
                (bytes[i] << 8) | bytes[i + 1] :
                bytes[i] | (bytes[i + 1] << 8);

            state = mjb_utf16_decode_step(state, unit, &codepoint);

            if(state == MJB_UTF16_ACCEPT) {
                ++count;
            }
        }
    }

    return count;
}

MJB_EXPORT size_t mjb_strncmp(const char *s1, const char *s2, size_t max_length, mjb_encoding encoding) {
    if(s1 == 0 || s2 == 0 || encoding != MJB_ENCODING_UTF_8) {
        // We return 0 to indicate an error. But the behavior is undefined.
        return 0;
    }

    return 0;
}
