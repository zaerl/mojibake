/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#pragma once

#include "utf8.h"
#include "utf16.h"
#include "utf32.h"

static inline bool MJB_USED mjb_decode_step(const char *buffer, size_t size, uint8_t *state,
    size_t *index, mjb_encoding encoding, mjb_codepoint *codepoint) {
    if(encoding == MJB_ENCODING_UTF_8) {
        if(!buffer[*index]) {
            return false;
        }

        *state = mjb_utf8_decode_step(*state, buffer[*index], codepoint);
    } else if(encoding == MJB_ENCODING_UTF_16_BE || encoding == MJB_ENCODING_UTF_16_LE) {
        if(*index + 1 >= size) {
            *state = MJB_UTF_REJECT;
        } else {
            if(!buffer[*index] && !buffer[*index + 1]) {
                return false;
            }

            *state = mjb_utf16_decode_step(*state, buffer[*index], buffer[*index + 1], codepoint,
                encoding == MJB_ENCODING_UTF_16_BE);
            ++*index;
        }
    } else if(encoding == MJB_ENCODING_UTF_32_BE || encoding == MJB_ENCODING_UTF_32_LE) {
        if(*index + 3 >= size) {
            *state = MJB_UTF_REJECT;
        } else {
            if(!buffer[*index] && !buffer[*index + 1] && !buffer[*index + 2] && !buffer[*index + 3]) {
                return false;
            }

            *state = mjb_utf32_decode_step(*state, buffer[*index], buffer[*index + 1], buffer[*index + 2],
                buffer[*index + 3], codepoint, encoding == MJB_ENCODING_UTF_32_BE);
            *index += 3;
        }
    }

    return true;
}
