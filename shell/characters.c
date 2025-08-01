/**
 * The Mojibake shell
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>

#include "characters.h"
#include "shell.h"

bool next_character(mjb_character *character, mjb_next_character_type type) {
    printf("%sU+%04X%s%s", color_green_start(), (unsigned int)character->codepoint, color_reset(),
        (type & MJB_NEXT_CHAR_LAST) ? "" : " ");

    return true;
}

bool next_array_character(mjb_character *character, mjb_next_character_type type) {
    printf("%u%s", character->codepoint, (type & MJB_NEXT_CHAR_LAST) ? "" : ", ");

    return true;
}

bool next_string_character(mjb_character *character, mjb_next_character_type type) {
    char buffer_utf8[5];
    size_t size = mjb_codepoint_encode(character->codepoint, buffer_utf8, 5, MJB_ENCODING_UTF_8);

    if(!size) {
        return false;
    }

    printf("%s", buffer_utf8);

    return true;
}

bool next_escaped_character(mjb_character *character, mjb_next_character_type type) {
    char buffer_utf8[5];
    size_t size = mjb_codepoint_encode(character->codepoint, buffer_utf8, 5, MJB_ENCODING_UTF_8);

    if(!size) {
        return false;
    }

    if(!print_escaped_character(buffer_utf8)) {
        printf("%s", buffer_utf8);
    }

    return true;
}
