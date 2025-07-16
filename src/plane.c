/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "mojibake.h"

MJB_EXPORT mjb_plane mjb_codepoint_plane(mjb_codepoint codepoint) {
    if(!mjb_codepoint_is_valid(codepoint)) {
        return MJB_PLANE_NOT_VALID;
    }

    return codepoint >> 16;
}

// Return true if the plane is valid
MJB_EXPORT bool mjb_plane_is_valid(mjb_plane plane) {
    return plane >= 0 && plane < MJB_PLANE_NUM;
}

// Return the name of a plane, NULL if the place specified is not valid
MJB_EXPORT const char *mjb_plane_name(mjb_plane plane, bool abbreviation) {
    if(plane == MJB_PLANE_NOT_VALID) {
        return NULL;
    }

    if(!mjb_plane_is_valid(plane)) {
        return NULL;
    }

    switch(plane) {
        case MJB_PLANE_BMP:
            return abbreviation ? "BMP" : "Basic Multilingual Plane";

        case MJB_PLANE_SMP:
            return abbreviation ? "SMP" : "Supplementary Multilingual Plane";

        case MJB_PLANE_SIP:
            return abbreviation ? "SIP" : "Supplementary Ideographic Plane";

        case MJB_PLANE_TIP:
            return abbreviation ? "TIP" : "Tertiary Ideographic Plane";

        case MJB_PLANE_SSP:
            return abbreviation ? "SSP" : "Supplementary Special-purpose Plane";

        case MJB_PLANE_PUA_A:
            return abbreviation ? "PUA-A" : "Supplementary Private Use Area-A";

        case MJB_PLANE_PUA_B:
            return abbreviation ? "PUA-B" : "Supplementary Private Use Area-B";

        case MJB_PLANE_NOT_VALID:
            return "Not valid";
    }

    return "Unassigned";
}
