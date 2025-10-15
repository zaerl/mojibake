/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "test.h"

void *test_plane(void *arg) {
    ATT_ASSERT(mjb_plane_is_valid(MJB_PLANE_SMP), true, "Valid codespace plane");
    ATT_ASSERT(mjb_plane_is_valid(MJB_PLANE_NOT_VALID), false, "Not valid negative codespace plane");
    ATT_ASSERT(mjb_plane_is_valid((mjb_plane)MJB_PLANE_NUM), false, "Not valid exceed codespace plane");
    ATT_ASSERT(mjb_plane_name(MJB_PLANE_BMP, true), "BMP", "Valid codespace plane name abbreviation");
    ATT_ASSERT(mjb_plane_name(MJB_PLANE_BMP, false), "Basic Multilingual Plane", "Valid codespace plane name full");
    ATT_ASSERT(mjb_plane_name(MJB_PLANE_NOT_VALID, false), (const char*)NULL, "Not valid codespace plane low");
    ATT_ASSERT(mjb_plane_name((mjb_plane)MJB_PLANE_NUM, false), (const char*)NULL, "Not valid codespace plane high");
    ATT_ASSERT(mjb_plane_name((mjb_plane)(MJB_PLANE_PUA_A + 1), false), "Unassigned", "Unassigned codespace plane abbreviation");
    ATT_ASSERT(mjb_plane_name((mjb_plane)(MJB_PLANE_PUA_A + 1), true), "Unassigned", "Unassigned codespace plane full");
    ATT_ASSERT(mjb_plane_name(MJB_PLANE_NOT_VALID, true), (const char*)NULL, "Not valid codespace plane abbreviation");
    ATT_ASSERT(mjb_plane_name(MJB_PLANE_NOT_VALID, false), (const char*)NULL, "Not valid codespace plane full");

    ATT_ASSERT((unsigned int)mjb_codepoint_plane(0xFFFD), (unsigned int)MJB_PLANE_BMP, "BMP plane");
    ATT_ASSERT((unsigned int)mjb_codepoint_plane(0xFFFF), (unsigned int)MJB_PLANE_NOT_VALID, "Not valid BMP plane");
    ATT_ASSERT((unsigned int)mjb_codepoint_plane(0xFFFF + 1), (unsigned int)MJB_PLANE_SMP, "SMP plane");

    return NULL;
}
