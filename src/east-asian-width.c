/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "mojibake-internal.h"
#include "unicode.h"

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
