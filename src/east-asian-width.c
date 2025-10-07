/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "mojibake-internal.h"
#include "east-asian-width.h"
#include "unicode.h"

extern mojibake mjb_global;

// Return the east asian width of a codepoint
MJB_EXPORT bool mjb_codepoint_east_asian_width(mjb_codepoint codepoint, mjb_east_asian_width *width) {
    if(!mjb_initialize()) {
        return false;
    }

    if(!mjb_codepoint_is_valid(codepoint)) {
        return false;
    }

    sqlite3_reset(mjb_global.stmt_east_asian_width);

    int rc = sqlite3_bind_int(mjb_global.stmt_east_asian_width, 1, codepoint);

    if(rc != SQLITE_OK) {
        return false;
    }

    if(sqlite3_step(mjb_global.stmt_east_asian_width) == SQLITE_ROW) {
        // Listed codepoint
        *width = (mjb_east_asian_width)sqlite3_column_int(mjb_global.stmt_east_asian_width, 0);
        return true;
    }

    // The unassigned code points in the following blocks default to "W":
    // CJK Unified Ideographs Extension A
    // CJK Unified Ideographs
    // CJK Compatibility Ideographs
    if(
        (codepoint >= MJB_CJK_EXTENSION_A_START && codepoint <= MJB_CJK_EXTENSION_A_END) ||
        (codepoint >= MJB_CJK_IDEOGRAPH_START && codepoint <= MJB_CJK_IDEOGRAPH_END) ||
        (codepoint >= MJB_CJK_COMPATIBILITY_IDEOGRAPH_START &&
            codepoint <= MJB_CJK_COMPATIBILITY_IDEOGRAPH_END)
    ) {
        *width = MJB_EAW_WIDE;
        return true;
    }

    // All undesignated code points in Planes 2 and 3, whether inside or outside of allocated
    // blocks, default to "W"
    if(
        (codepoint >= MJB_CJK_EXTENSION_B_START && codepoint <= 0x2FFFD) || // Plane 2
        (codepoint >= 0x30000 && codepoint <= 0x3FFFD) // Plane 3
    ) {
        *width = MJB_EAW_WIDE;
        return true;
    }

    if(
        (codepoint >= MJB_PRIVATE_USE_START && codepoint <= 0xFFFFD) || // Plane 15 Private Use
        (codepoint >= 0x100000 && codepoint <= MJB_PRIVATE_USE_END) // Plane 16 Private Use
    ) {
        *width = MJB_EAW_AMBIGUOUS;
        return true;
    }

    // All code points, assigned or unassigned, that are not listed are given the value "N".
    *width = MJB_EAW_NEUTRAL;

    return true;
}
