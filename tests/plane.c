/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "../src/mojibake.h"
#include "test.h"

void *test_plane(void *arg) {
    ATT_ASSERT(mjb_string_encoding(0, 10), MJB_ENCODING_UNKNOWN, "Void string")
    ATT_ASSERT(mjb_plane_is_valid(1), true, "Valid codespace plane");
    ATT_ASSERT(mjb_plane_is_valid(-1), false, "Not valid negative codespace plane");
    ATT_ASSERT(mjb_plane_is_valid(MJB_PLANE_NUM), false, "Not valid exceed codespace plane");
    ATT_ASSERT(mjb_plane_name(0, true), "BMP", "Valid codespace plane name abbreviation");
    ATT_ASSERT(mjb_plane_name(0, false), "Basic Multilingual Plane", "Valid codespace plane name full");
    ATT_ASSERT(mjb_plane_name(-1, false), NULL, "Not valid codespace plane low");
    ATT_ASSERT(mjb_plane_name(MJB_PLANE_NUM, false), NULL, "Not valid codespace plane high");
    ATT_ASSERT(mjb_plane_name(4, false), "Unassigned", "Unassigned codespace plane abbreviation");
    ATT_ASSERT(mjb_plane_name(4, true), "Unassigned", "Unassigned codespace plane full");

    return NULL;
}
