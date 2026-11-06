/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "test.h"

ATT_TEST(plane) {
    ATT_ASSERT(mjb_plane_is_valid(MJB_PLANE_SMP), true, "Valid codespace plane");
    ATT_ASSERT(mjb_plane_is_valid(MJB_PLANE_NOT_VALID), false, "Not valid negative codespace plane");
    ATT_ASSERT(mjb_plane_is_valid((mjb_plane)MJB_PLANE_NUM), false,
        "Not valid exceed codespace plane");
    ATT_ASSERT(mjb_plane_name(MJB_PLANE_BMP, true), "BMP",
        "Valid codespace plane name abbreviation");
    ATT_ASSERT(mjb_plane_name(MJB_PLANE_BMP, false), "Basic Multilingual Plane",
        "Valid codespace plane name full");
    ATT_ASSERT(mjb_plane_name(MJB_PLANE_NOT_VALID, false), (const char *)NULL,
        "Not valid codespace plane low");
    ATT_ASSERT(mjb_plane_name((mjb_plane)MJB_PLANE_NUM, false), (const char *)NULL,
        "Not valid codespace plane high");
    ATT_ASSERT(mjb_plane_name((mjb_plane)4, false), "Unassigned", "Unassigned plane 4 full");
    ATT_ASSERT(mjb_plane_name((mjb_plane)4, true), "Unassigned", "Unassigned plane 4 abbreviation");
    ATT_ASSERT(mjb_plane_name((mjb_plane)13, false), "Unassigned", "Unassigned plane 13 full");
    ATT_ASSERT(mjb_plane_name((mjb_plane)13, true), "Unassigned",
        "Unassigned plane 13 abbreviation");
    ATT_ASSERT((int)MJB_PLANE_SSP, 14, "SSP is plane 14");
    ATT_ASSERT((int)MJB_PLANE_PUA_A, 15, "PUA-A is plane 15");
    ATT_ASSERT((int)MJB_PLANE_PUA_B, 16, "PUA-B is plane 16");
    ATT_ASSERT(mjb_plane_name(MJB_PLANE_SSP, false), "Supplementary Special-purpose Plane",
        "SSP plane name full");
    ATT_ASSERT(mjb_plane_name(MJB_PLANE_SSP, true), "SSP", "SSP plane name abbreviation");
    ATT_ASSERT(mjb_plane_name(MJB_PLANE_PUA_A, false), "Supplementary Private Use Area-A",
        "PUA-A plane name full");
    ATT_ASSERT(mjb_plane_name(MJB_PLANE_PUA_A, true), "PUA-A", "PUA-A plane name abbreviation");
    ATT_ASSERT(mjb_plane_name(MJB_PLANE_NOT_VALID, true), (const char *)NULL,
        "Not valid codespace plane abbreviation");
    ATT_ASSERT(mjb_plane_name(MJB_PLANE_NOT_VALID, false), (const char *)NULL,
        "Not valid codespace plane full");

    ATT_ASSERT((unsigned int)mjb_codepoint_plane(0xFFFD), (unsigned int)MJB_PLANE_BMP, "BMP plane");
    ATT_ASSERT((unsigned int)mjb_codepoint_plane(0xFFFF), (unsigned int)MJB_PLANE_NOT_VALID,
        "Not valid BMP plane");
    ATT_ASSERT((unsigned int)mjb_codepoint_plane(0xFFFF + 1), (unsigned int)MJB_PLANE_SMP,
        "SMP plane");
    ATT_ASSERT((unsigned int)mjb_codepoint_plane(0xE0001), (unsigned int)MJB_PLANE_SSP,
        "U+E0001 LANGUAGE TAG is in the SSP");
    ATT_ASSERT(mjb_plane_name(mjb_codepoint_plane(0xE0001), false),
        "Supplementary Special-purpose Plane", "U+E0001 plane name");
    ATT_ASSERT((unsigned int)mjb_codepoint_plane(0xF0000), (unsigned int)MJB_PLANE_PUA_A,
        "U+F0000 is in PUA-A");
    ATT_ASSERT((unsigned int)mjb_codepoint_plane(0x100000), (unsigned int)MJB_PLANE_PUA_B,
        "U+100000 is in PUA-B");
}
