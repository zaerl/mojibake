/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "mojibake-internal.h"
#include "unicode.h"
#include "utf.h"

extern mojibake mjb_global;

// Return the east asian width of a codepoint
MJB_EXPORT bool mjb_codepoint_east_asian_width(mjb_codepoint codepoint, mjb_east_asian_width *width) {
    if(!mjb_codepoint_is_valid(codepoint)) {
        return false;
    }

    bool found = mjb_codepoint_has_property(codepoint, MJB_PR_EAST_ASIAN_WIDTH, (uint8_t*)width);

    if(!found) {
        // All code points, assigned or unassigned, that are not listed are given the value "N".
        *width = MJB_EAW_NEUTRAL;
    }

    return true;
}

/**
 * Return size of a string.
 */
MJB_EXPORT bool mjb_display_width(const char *buffer, size_t size, mjb_encoding encoding,
    size_t *width) {
    if(size == 0) {
        *width = 0;

        return true;
    }

    *width = 0;
    uint8_t state = MJB_UTF_ACCEPT;
    bool in_error = false;
    mjb_codepoint codepoint;

    for(size_t i = 0; i < size;) {
        mjb_decode_result decode_status = mjb_next_codepoint(buffer, size, &state, &i,
            encoding, &codepoint, &in_error);

        if(decode_status == MJB_DECODE_END) {
            break;
        }

        if(decode_status == MJB_DECODE_INCOMPLETE) {
            continue;
        }

        mjb_east_asian_width eaw = MJB_EAW_NOT_SET;

        if(mjb_codepoint_east_asian_width(codepoint, &eaw)) {
            if(eaw == MJB_EAW_NEUTRAL || eaw == MJB_EAW_NARROW || eaw == MJB_EAW_HALF_WIDTH) {
                *width += 1;
            } else if(eaw == MJB_EAW_FULL_WIDTH || eaw == MJB_EAW_WIDE) {
                *width += 2;
            } else if(eaw == MJB_EAW_AMBIGUOUS) {
                *width += 2; // TODO: Handle ambiguous width
            }
        }
    }

    return true;
}
