/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "mojibake.h"

/**
 * Return UTF-8 length of a string.
 */
MJB_EXPORT size_t mjb_strnlen(const char *buffer, size_t max_length, mjb_encoding encoding) {
    if(buffer == 0 || max_length == 0 || encoding != MJB_ENCODING_UTF_8) {
        return 0;
    }

    uint8_t state = MJB_UTF8_ACCEPT;
    mjb_codepoint codepoint;
    size_t count = 0;
    const char *current = buffer;

    while(*current && (size_t)(current - buffer) < max_length) {
        state = mjb_utf8_decode_step(state, *current, &codepoint);

        if(state == MJB_UTF8_ACCEPT) {
            ++count;
        }

        ++current;
    }

    return count;
}
