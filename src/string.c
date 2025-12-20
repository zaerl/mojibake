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
    if(buffer == NULL || max_length == 0) {
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

MJB_PURE int mjb_string_compare(const char *s1, size_t s1_length, mjb_encoding s1_encoding,
    const char *s2, size_t s2_length, mjb_encoding s2_encoding) {
    uint8_t state_1 = MJB_UTF_ACCEPT;
    uint8_t state_2 = MJB_UTF_ACCEPT;
    mjb_codepoint codepoint_1;
    mjb_codepoint codepoint_2;
    size_t i = 0;
    size_t j = 0;

    for(i = 0; i < s1_length; ++i) {
        // Find next codepoint.
        if(!mjb_decode_step(s1, s1_length, &state_1, &i, s1_encoding, &codepoint_1)) {
            break;
        }

        if(state_1 == MJB_UTF_REJECT) {
            return -1;
        }

        if(state_1 == MJB_UTF_ACCEPT) {
            for(; j < s2_length; ++j) {
                if(!mjb_decode_step(s2, s2_length, &state_2, &j, s2_encoding, &codepoint_2)) {
                    break;
                }

                if(state_2 == MJB_UTF_REJECT) {
                    return 1;
                }

                if(state_2 == MJB_UTF_ACCEPT) {
                    if(codepoint_1 < codepoint_2) {
                        return -1;
                    } else if(codepoint_1 > codepoint_2) {
                        return 1;
                    }

                    if(s2_encoding == MJB_ENCODING_UTF_16_BE || s2_encoding == MJB_ENCODING_UTF_16_LE) {
                        j += 2;
                    } else if(s2_encoding == MJB_ENCODING_UTF_32_BE || s2_encoding == MJB_ENCODING_UTF_32_LE) {
                        j += 4;
                    } else {
                        ++j;
                    }

                    break;
                }
            }
        }
    }

    if(j < s2_length) {
        return -1;
    } else if(i < s1_length) {
        return 1;
    }

    return 0;
}
