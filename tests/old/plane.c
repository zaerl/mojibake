/**
 * The mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <string.h>
#include "test.h"

MJB_EXPORT void mjb_plane_is_valid_test(void) {
    bool validity = mjb_plane_is_valid(1);
    mjb_assert("Valid codespace plane", validity);

    validity = mjb_plane_is_valid(-1);
    mjb_assert("Not valid negative codespace plane", !validity);

    validity = mjb_plane_is_valid(MJB_PLANE_NUM);
    mjb_assert("Not valid exceed codespace plane", !validity);
}

MJB_EXPORT void mjb_plane_name_test(void) {
    bool validity = strcmp(mjb_plane_name(0, true), "BMP") == 0;
    mjb_assert("Valid codespace plane name abbreviation", validity);

    validity = strcmp(mjb_plane_name(0, false), "Basic Multilingual Plane") == 0;
    mjb_assert("Valid codespace plane name full", validity);

    validity = mjb_plane_name(-1, false) == NULL;
    mjb_assert("Not valid codespace plane low", validity);

    validity = mjb_plane_name(MJB_PLANE_NUM, false) == NULL;
    mjb_assert("Not valid codespace plane high", validity);

    validity = strcmp(mjb_plane_name(4, false), "Unassigned") == 0;
    mjb_assert("Unassigned codespace plane abbreviation", validity);

    validity = strcmp(mjb_plane_name(4, true), "Unassigned") == 0;
    mjb_assert("Unassigned codespace plane full", validity);
}
