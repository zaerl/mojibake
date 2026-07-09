/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#pragma once

#include "utf8.h"
#include "utf16.h"
#include "utf32.h"

/**
 * WARNING: Setting this macro to 1 disables NULL termination checks. This allows processing strings
 * with embedded U+0000 codepoints but removes a **critical safety feature**. Only enable it if you
 * know what you are doing.
 *
 * Default: 0 (NULL codepoints terminate string processing)
 */
#ifndef MJB_DANGEROUSLY_ALLOW_EMBEDDED_NULLS
#define MJB_DANGEROUSLY_ALLOW_EMBEDDED_NULLS 0
#endif

typedef enum {
    MJB_DECODE_OK,         // Successfully decoded a codepoint
    MJB_DECODE_INCOMPLETE, // Still accumulating bytes for multi-byte sequence
    MJB_DECODE_END,        // Reached end of string
    MJB_DECODE_ERROR       // Invalid sequence (codepoint set to MJB_CODEPOINT_REPLACEMENT "�")
} mjb_decode_result;

static inline bool MJB_USED mjb_utf_state_is_incomplete(uint8_t state) {
    return state != MJB_UTF_ACCEPT && state != MJB_UTF_REJECT;
}

static inline bool MJB_USED mjb_starts_with_utf8_bom(const char *buffer, size_t byte_length) {
    return byte_length >= 3 && (uint8_t)buffer[0] == 0xEF && (uint8_t)buffer[1] == 0xBB &&
        (uint8_t)buffer[2] == 0xBF;
}

static inline bool MJB_USED mjb_starts_with_utf16be_bom(const char *buffer, size_t byte_length) {
    return byte_length >= 2 && (uint8_t)buffer[0] == 0xFE && (uint8_t)buffer[1] == 0xFF;
}

static inline bool MJB_USED mjb_starts_with_utf16le_bom(const char *buffer, size_t byte_length) {
    return byte_length >= 2 &&
        (uint8_t)buffer[0] == 0xFF && (uint8_t)buffer[1] == 0xFE;
}

static inline bool MJB_USED mjb_starts_with_utf32be_bom(const char *buffer, size_t byte_length) {
    return byte_length >= 4 && (uint8_t)buffer[0] == 0x00 && (uint8_t)buffer[1] == 0x00 &&
        (uint8_t)buffer[2] == 0xFE && (uint8_t)buffer[3] == 0xFF;
}

static inline bool MJB_USED mjb_starts_with_utf32le_bom(const char *buffer, size_t byte_length) {
    return byte_length >= 4 && (uint8_t)buffer[0] == 0xFF && (uint8_t)buffer[1] == 0xFE &&
        (uint8_t)buffer[2] == 0x00 && (uint8_t)buffer[3] == 0x00;
}

static inline mjb_encoding MJB_USED mjb_resolve_input_encoding(const char *buffer,
    size_t byte_length, mjb_encoding encoding, size_t *index) {
    bool at_start = index != NULL && *index == 0;

    // Consume the BOM at the start of a string and determine the real encoding.
    if(encoding & MJB_ENC_UTF_32) {
        if(mjb_starts_with_utf32be_bom(buffer, byte_length)) {
            if(at_start) {
                *index = 4;
            }

            return MJB_ENC_UTF_32BE;
        }

        if(mjb_starts_with_utf32le_bom(buffer, byte_length)) {
            if(at_start) {
                *index = 4;
            }

            return MJB_ENC_UTF_32LE;
        }

        if((encoding & MJB_ENC_UTF_32BE) && !(encoding & MJB_ENC_UTF_32LE)) {
            return MJB_ENC_UTF_32BE;
        }

        if((encoding & MJB_ENC_UTF_32LE) && !(encoding & MJB_ENC_UTF_32BE)) {
            return MJB_ENC_UTF_32LE;
        }
    }

    // Consume the BOM at the start of a string and determine the real encoding.
    if(encoding & MJB_ENC_UTF_16) {
        if(mjb_starts_with_utf16be_bom(buffer, byte_length)) {
            if(at_start) {
                *index = 2;
            }

            return MJB_ENC_UTF_16BE;
        }

        if(mjb_starts_with_utf16le_bom(buffer, byte_length)) {
            if(at_start) {
                *index = 2;
            }

            return MJB_ENC_UTF_16LE;
        }

        if((encoding & MJB_ENC_UTF_16BE) && !(encoding & MJB_ENC_UTF_16LE)) {
            return MJB_ENC_UTF_16BE;
        }

        if((encoding & MJB_ENC_UTF_16LE) && !(encoding & MJB_ENC_UTF_16BE)) {
            return MJB_ENC_UTF_16LE;
        }
    }

    return encoding;
}

static inline bool MJB_USED mjb_decode_step(const char *buffer, size_t byte_length, uint8_t *state,
    size_t *index, mjb_encoding encoding, mjb_codepoint *codepoint) {
    if(encoding == MJB_ENC_UTF_8 || encoding == MJB_ENC_ASCII) {
#if !MJB_DANGEROUSLY_ALLOW_EMBEDDED_NULLS
        if(!buffer[*index]) {
            return false;
        }
#endif

        *state = mjb_utf8_decode_step(*state, buffer[*index], codepoint);
        ++*index;  // Increment by 1 byte
    } else if(encoding == MJB_ENC_UTF_16BE || encoding == MJB_ENC_UTF_16LE) {
        if(*index + 1 >= byte_length) {
            // Truncated trailing unit: consume it, or decoding would never terminate.
            *state = MJB_UTF_REJECT;
            *index = byte_length;
        } else {
#if !MJB_DANGEROUSLY_ALLOW_EMBEDDED_NULLS
            if(!buffer[*index] && !buffer[*index + 1]) {
                return false;
            }
#endif

            *state = mjb_utf16_decode_step(*state, buffer[*index], buffer[*index + 1], codepoint,
                encoding == MJB_ENC_UTF_16BE);
            *index += 2;  // Increment by 2 bytes (full code unit)
        }
    } else if(encoding == MJB_ENC_UTF_32BE || encoding == MJB_ENC_UTF_32LE) {
        if(*index + 3 >= byte_length) {
            // Truncated trailing unit: consume it, or decoding would never terminate.
            *state = MJB_UTF_REJECT;
            *index = byte_length;
        } else {
#if !MJB_DANGEROUSLY_ALLOW_EMBEDDED_NULLS
            if(!buffer[*index] && !buffer[*index + 1] && !buffer[*index + 2] && !buffer[*index + 3]) {
                return false;
            }
#endif

            *state = mjb_utf32_decode_step(*state, buffer[*index], buffer[*index + 1], buffer[*index + 2],
                buffer[*index + 3], codepoint, encoding == MJB_ENC_UTF_32BE);
            *index += 4;  // Increment by 4 bytes (full code unit)
        }
    } else {
        *codepoint = MJB_CODEPOINT_REPLACEMENT;
        *state = MJB_UTF_ACCEPT;
        ++*index;
    }

    return true;
}

/**
 * Get the next codepoint from a string.
 */
static inline mjb_decode_result MJB_USED mjb_next_codepoint(const char *buffer, size_t byte_length,
    uint8_t *state, size_t *index, mjb_encoding encoding, mjb_codepoint *codepoint,
    bool *in_error) {
    mjb_encoding requested_encoding = encoding;
    encoding = mjb_resolve_input_encoding(buffer, byte_length, encoding, index);

    if(*index >= byte_length) {
        // Check if we have an incomplete sequence at end of buffer
        if(mjb_utf_state_is_incomplete(*state)) {
            *codepoint = MJB_CODEPOINT_REPLACEMENT;
            *state = MJB_UTF_ACCEPT;

            return MJB_DECODE_ERROR;
        }

        return MJB_DECODE_END;
    }

    if((requested_encoding == MJB_ENC_UTF_16 || requested_encoding == MJB_ENC_UTF_32) &&
        encoding == requested_encoding) {
        *codepoint = MJB_CODEPOINT_REPLACEMENT;
        *state = MJB_UTF_ACCEPT;
        *index = byte_length;
        *in_error = true;

        return MJB_DECODE_ERROR;
    }

    uint8_t prev_state = *state;

    if(!mjb_decode_step(buffer, byte_length, state, index, encoding, codepoint)) {
        // Check if we have an incomplete sequence at end
        if(mjb_utf_state_is_incomplete(*state)) {
            *codepoint = MJB_CODEPOINT_REPLACEMENT;
            *state = MJB_UTF_ACCEPT;

            return MJB_DECODE_ERROR;
        }

        return MJB_DECODE_END;
    }

    if(*state == MJB_UTF_REJECT) {
        // Check if the byte is valid standalone (like ASCII) that caused rejection
        // We need to reprocess it after emitting replacement for previous bytes
        // Note: index has been incremented, so the byte just processed is at [*index - 1]
        bool should_reprocess = (encoding == MJB_ENC_UTF_8 && prev_state != MJB_UTF_ACCEPT &&
            *index > 0 && (unsigned char)buffer[*index - 1] < 0x80);

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

/**
 * Return the number of encoded bytes a codepoint occupies in the given encoding.
 * UTF-8:  1–4 bytes depending on codepoint value (ASCII shares the UTF-8 decode path).
 * UTF-16: 2 bytes for BMP (U+0000–U+FFFF), 4 bytes for supplementary planes.
 * UTF-32: always 4 bytes.
 */
static inline size_t MJB_USED mjb_codepoint_encoded_bytes(mjb_codepoint cp, mjb_encoding encoding) {
    if(encoding == MJB_ENC_UTF_8 || encoding == MJB_ENC_ASCII) {
        if(cp < 0x80) {
            return 1;
        }

        if(cp < 0x800) {
            return 2;
        }

        if(cp < 0x10000) {
            return 3;
        }

        return 4;
    }

    if((encoding & MJB_ENC_UTF_32) || (encoding & MJB_ENC_UTF_32BE) ||
        (encoding & MJB_ENC_UTF_32LE)) {
        return 4;
    }

    if((encoding & MJB_ENC_UTF_16) || (encoding & MJB_ENC_UTF_16BE) ||
        (encoding & MJB_ENC_UTF_16LE)) {
        return cp >= 0x10000 ? 4 : 2;
    }

    return 4;
}

/**
 * Byte offset at which the codepoint most recently decoded into |index| begins. Guards against
 * underflow when a replacement codepoint reports more encoded bytes than were consumed from a
 * malformed sequence.
 */
static inline size_t MJB_USED mjb_cluster_start(size_t index, size_t size, mjb_codepoint cp,
    mjb_encoding encoding) {
    if(index > size) {
        return size;
    }

    size_t encoded = mjb_codepoint_encoded_bytes(cp, encoding);

    return encoded > index ? 0 : index - encoded;
}

static inline void MJB_USED mjb_mark_decode_terminated(uint8_t *state, size_t *index,
    mjb_codepoint *current_codepoint, mjb_encoding encoding) {
    if(*current_codepoint == MJB_CODEPOINT_NOT_VALID) {
        *current_codepoint = 0;
    }

    *index += mjb_codepoint_encoded_bytes(*current_codepoint, encoding);
    *state = MJB_UTF_TERMINATED;
}

static inline size_t MJB_USED mjb_boundary_position(size_t index, size_t size, mjb_codepoint cp,
    mjb_encoding encoding, bool terminated) {
    if(terminated) {
        size_t encoded = mjb_codepoint_encoded_bytes(cp, encoding);

        return encoded > index ? 0 : index - encoded;
    }

    return mjb_cluster_start(index, size, cp, encoding);
}

static inline size_t MJB_USED mjb_monotonic_boundary_position(size_t index, size_t size,
    mjb_codepoint cp, mjb_encoding encoding, bool terminated, size_t previous) {
    size_t position = mjb_boundary_position(index, size, cp, encoding, terminated);

    return position < previous ? previous : position;
}
