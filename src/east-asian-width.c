/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "mojibake-internal.h"
#include "unicode.h"

extern mojibake mjb_global;

// Return the east asian width of a codepoint
MJB_EXPORT mjb_status mjb_codepoint_east_asian_width(mjb_codepoint codepoint,
    mjb_east_asian_width *width) {
    if(width == NULL || !mjb_codepoint_is_valid(codepoint)) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    int32_t raw = 0;
    bool found = mjb_codepoint_property_int(codepoint, MJB_PR_EAST_ASIAN_WIDTH, &raw) ==
        MJB_STATUS_OK;

    if(!found) {
        // All code points, assigned or unassigned, that are not listed are given the value "N".
        *width = MJB_EAW_NEUTRAL;
    } else {
        *width = (mjb_east_asian_width)raw;
    }

    return MJB_STATUS_OK;
}
