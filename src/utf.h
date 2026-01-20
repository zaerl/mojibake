/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#pragma once

#include "utf8.h"
#include "utf16.h"
#include "utf32.h"

typedef enum {
    MJB_DECODE_OK,         // Successfully decoded a codepoint
    MJB_DECODE_INCOMPLETE, // Still accumulating bytes for multi-byte sequence
    MJB_DECODE_END,        // Reached end of string
    MJB_DECODE_ERROR       // Invalid sequence (codepoint set to MJB_CODEPOINT_REPLACEMENT "�")
} mjb_decode_result;

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

/**
 * Get the next codepoint from a string.
 */
static inline mjb_decode_result MJB_USED mjb_next_codepoint(const char *buffer, size_t size,
    uint8_t *state, size_t *index, mjb_encoding encoding, mjb_codepoint *codepoint,
    bool *in_error) {
    if(*index >= size) {
        // Check if we have an incomplete sequence at end of buffer
        if(*state != MJB_UTF_ACCEPT && *state != MJB_UTF_REJECT) {
            *codepoint = MJB_CODEPOINT_REPLACEMENT;
            *state = MJB_UTF_ACCEPT;

            return MJB_DECODE_ERROR;
        }

        return MJB_DECODE_END;
    }

    uint8_t prev_state = *state;

    if(!mjb_decode_step(buffer, size, state, index, encoding, codepoint)) {
        // Check if we have an incomplete sequence at end
        if(*state != MJB_UTF_ACCEPT && *state != MJB_UTF_REJECT) {
            *codepoint = MJB_CODEPOINT_REPLACEMENT;
            *state = MJB_UTF_ACCEPT;

            return MJB_DECODE_ERROR;
        }

        return MJB_DECODE_END;
    }

    if(*state == MJB_UTF_REJECT) {
        // Check if the byte is valid standalone (like ASCII) that caused rejection
        // We need to reprocess it after emitting replacement for previous bytes
        bool should_reprocess = (encoding == MJB_ENCODING_UTF_8 && prev_state != MJB_UTF_ACCEPT &&
            *index > 0 && (unsigned char)buffer[*index] < 0x80);

        // Emit MJB_CODEPOINT_REPLACEMENT only once per ill-formed subsequence
        if(!*in_error) {
            *codepoint = MJB_CODEPOINT_REPLACEMENT;
            *in_error = true;
            *state = MJB_UTF_ACCEPT;

            if(should_reprocess) {
                (*index)--;
                *in_error = false;
            }

            return MJB_DECODE_ERROR;
        }

        // Already in error sequence
        if(should_reprocess) {
            // This is a valid standalone byte that broke the sequence. Reprocess it as new
            (*index)--;
            *in_error = false;
            *state = MJB_UTF_ACCEPT;

            return MJB_DECODE_INCOMPLETE;
        }

        *state = MJB_UTF_ACCEPT;

        return MJB_DECODE_INCOMPLETE;
    }

    if(*state == MJB_UTF_ACCEPT) {
        *in_error = false;

        return MJB_DECODE_OK;
    }

    return MJB_DECODE_INCOMPLETE;
}
