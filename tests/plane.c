#include <string.h>

#include "test.h"

MB_EXPORT void mb_plane_is_valid_test() {
    bool validity = mb_plane_is_valid(1);
    mb_assert("Valid codespace plane", validity);

    validity = mb_plane_is_valid(-1);
    mb_assert("Not valid negative codespace plane", !validity);

    validity = mb_plane_is_valid(MB_PLANE_NUM);
    mb_assert("Not valid exceed codespace plane", !validity);
}

MB_EXPORT void mb_plane_name_test() {
    bool validity = strcmp(mb_plane_name(0, true), "BMP") == 0;
    mb_assert("Valid codespace plane name abbreviation", validity);

    validity = strcmp(mb_plane_name(0, false), "Basic Multilingual Plane") == 0;
    mb_assert("Valid codespace plane name full", validity);

    validity = mb_plane_name(-1, false) == NULL;
    mb_assert("Not valid codespace plane low", validity);

    validity = mb_plane_name(MB_PLANE_NUM, false) == NULL;
    mb_assert("Not valid codespace plane high", validity);

    validity = strcmp(mb_plane_name(4, false), "Unassigned") == 0;
    mb_assert("Unassigned codespace plane abbreviation", validity);

    validity = strcmp(mb_plane_name(4, true), "Unassigned") == 0;
    mb_assert("Unassigned codespace plane full", validity);
}
