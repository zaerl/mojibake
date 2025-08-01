/**
 * The Mojibake shell
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../mojibake.h"
#include "../maps.h"
#include "../shell.h"

bool output_next_character(mjb_character *character, mjb_next_character_type type) {
    char buffer_utf8[5];
    unsigned int utf8_length = mjb_codepoint_encode(character->codepoint, buffer_utf8, 5, MJB_ENCODING_UTF_8);

    if(utf8_length == 0) {
        return false;
    }

    bool is_json = cmd_output_mode == OUTPUT_MODE_JSON;

    if(is_json) {
        if(type & MJB_NEXT_CHAR_FIRST) {
            printf("[%s%s{%s", json_nl(), json_i(), json_nl());
        } else {
            printf("%s{%s", json_i(), json_nl());
        }
    } else {
        if(type != MJB_NEXT_CHAR_FIRST) {
            puts("");
        }
    }

    print_value("Codepoint", 1, "U+%04X", (unsigned int)character->codepoint);
    print_value("Name", 1, "%s", character->name);

    if(is_json) {
        printf("%s%s\"character\":%s\"%s", json_i(), json_i(), cmd_json_indent == 0 ? "" : " ", color_green_start());

        if(!print_escaped_character(buffer_utf8)) {
            printf("%s", buffer_utf8);
        }

        printf("%s\",%s", color_reset(), json_nl());
    } else {
        print_value("Character", 1, "%s", buffer_utf8);
    }

    // Hex UTF-8
    if(is_json) {
        printf("%s%s\"hex_utf-8\":%s[%s", json_i(), json_i(), cmd_json_indent == 0 ? "" : " ", color_green_start());
    } else {
        printf("Hex UTF-8: %s", color_green_start());
    }

    for(size_t i = 0; i < utf8_length; ++i) {
        if(is_json) {
            printf("%u%s", (unsigned char)buffer_utf8[i], i == utf8_length - 1 ? "" : ", ");
        } else {
            printf("%02X%s", (unsigned char)buffer_utf8[i], i == utf8_length - 1 ? "" : " ");
        }
    }

    printf("%s%s", color_reset(), is_json ? "]" : "");

    if(is_json) {
        printf(",%s", json_nl());
    } else {
        puts("");
    }

    print_normalization(buffer_utf8, utf8_length, MJB_NORMALIZATION_NFD, "nfd", "NFD", true);
    print_normalization(buffer_utf8, utf8_length, MJB_NORMALIZATION_NFKD, "nfkd", "NFKD", true);

    if(is_json) {
        print_id_name_value("category", character->category, category_name(character->category), true);
    } else {
        print_value("Category", 1, "[%d] %s", character->category, category_name(character->category));
    }

    char *cc_name = ccc_name(character->combining);
    if(is_json) {
        print_id_name_value("combining", character->combining, cc_name, true);
    } else {
        print_value("Combining", 1, "[%d] %s", character->combining, cc_name);
    }

    free(cc_name);

    const char *bi_name = bidi_name(character->bidirectional);

    if(is_json) {
        print_id_name_value("bidirectional", character->bidirectional, bi_name, true);
    } else {
        print_value("Bidirectional", 1, "[%d] %s", character->bidirectional, bi_name);
    }

    mjb_plane plane = mjb_codepoint_plane(character->codepoint);
    const char *plane_name = mjb_plane_name(plane, false);

    if(is_json) {
        print_id_name_value("plane", plane, plane_name, true);
    } else {
        print_value("Plane", 1, "[%d] %s", plane, plane_name);
    }

    mjb_codepoint_block block = {0};

    bool valid_block = mjb_character_block(character->codepoint, &block);

    if(valid_block) {
        if(is_json) {
            print_id_name_value("block", block.id, block.name, true);
        } else {
            print_value("Block", 1, "[%d] %s", block.id, block.name);
        }
    }

    const char *d_name = decomposition_name(character->decomposition);

    if(is_json) {
        print_id_name_value("decomposition", character->decomposition, d_name, true);
    } else {
        print_value("Decomposition", 1, "[%d] %s", character->decomposition, d_name);
    }

    if(character->decimal == MJB_NUMBER_NOT_VALID) {
        print_null_value("Decimal", 1);
    } else {
        print_value("Decimal", 1, "%d", character->decimal);
    }

    if(character->digit == MJB_NUMBER_NOT_VALID) {
        print_null_value("Digit", 1);
    } else {
        print_value("Digit", 1, "%d", character->digit);
    }

    if(character->numeric[0] != '\0') {
        print_value("Numeric", 1, "%s", character->numeric);
    } else {
        print_null_value("Numeric", 1);
    }

    if(is_json) {
        printf("%s%s\"mirrored\":%s%s%s%s,%s", json_i(), json_i(), cmd_json_indent == 0 ? "" : " ",
            color_green_start(), character->mirrored ? "true" : "false", color_reset(), json_nl());
    } else {
        print_value("Mirrored", 1, "%s", character->mirrored ? "Y" : "N");
    }

    if(character->uppercase != 0) {
        print_value("Uppercase", 1, "%04X", character->uppercase);
    } else {
        print_null_value("Uppercase", 1);
    }

    if(character->lowercase != 0) {
        print_value("Lowercase", 1, "%04X", (unsigned int)character->lowercase);
    } else {
        print_null_value("Lowercase", 1);
    }

    if(character->titlecase != 0) {
        print_value("Titlecase", 2, "%04X", (unsigned int)character->titlecase);
    } else {
        print_null_value("Titlecase", 2);
    }

    if(is_json) {
        printf("%s}%s%s", json_i(), !(type & MJB_NEXT_CHAR_LAST) ? "," : "", json_nl());

        if(type & MJB_NEXT_CHAR_LAST) {
            printf("]%s", json_nl());
        }
    }

    return true;
}

int character_command(int argc, char * const argv[], unsigned int flags) {
    mjb_next_character(argv[0], strlen(argv[0]), MJB_ENCODING_UTF_8, output_next_character);

    return 0;
}
