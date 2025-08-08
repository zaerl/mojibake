/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "test.h"

void *test_plane(void *arg) {
    ATT_ASSERT(mjb_plane_is_valid(1), true, "Valid codespace plane");
    ATT_ASSERT(mjb_plane_is_valid(-1), false, "Not valid negative codespace plane");
    ATT_ASSERT(mjb_plane_is_valid(MJB_PLANE_NUM), false, "Not valid exceed codespace plane");
    ATT_ASSERT(mjb_plane_name(MJB_PLANE_BMP, true), "BMP", "Valid codespace plane name abbreviation");
    ATT_ASSERT(mjb_plane_name(MJB_PLANE_BMP, false), "Basic Multilingual Plane", "Valid codespace plane name full");
    ATT_ASSERT(mjb_plane_name(-1, false), NULL, "Not valid codespace plane low");
    ATT_ASSERT(mjb_plane_name(MJB_PLANE_NUM, false), NULL, "Not valid codespace plane high");
    ATT_ASSERT(mjb_plane_name(MJB_PLANE_PUA_A + 1, false), "Unassigned", "Unassigned codespace plane abbreviation");
    ATT_ASSERT(mjb_plane_name(MJB_PLANE_PUA_A + 1, true), "Unassigned", "Unassigned codespace plane full");
    ATT_ASSERT(mjb_plane_name(MJB_PLANE_NOT_VALID, true), NULL, "Not valid codespace plane abbreviation");
    ATT_ASSERT(mjb_plane_name(MJB_PLANE_NOT_VALID, false), NULL, "Not valid codespace plane full");

    ATT_ASSERT(mjb_codepoint_plane(0xFFFD), MJB_PLANE_BMP, "BMP plane");
    ATT_ASSERT(mjb_codepoint_plane(0xFFFF), MJB_PLANE_NOT_VALID, "Not valid BMP plane");
    ATT_ASSERT(mjb_codepoint_plane(0xFFFF + 1), MJB_PLANE_SMP, "SMP plane");

    return NULL;
}
