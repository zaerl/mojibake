/**
 * The Mojibake shell
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../src/mojibake.h"
#include "../maps.h"
#include "../shell.h"

static bool parse_character(const char *input, mjb_character *character) {
    mjb_codepoint value = 0;

    if(!parse_codepoint(input, &value)) {
        return false;
    }

    return mjb_codepoint_character(character, value);
}

int character_command(int argc, char * const argv[]) {
    mjb_character character = {0};

    if(!parse_character(argv[0], &character)) {
        fprintf(stderr, cmd_verbose ? "Invalid\n" : "N\n");

        return 1;
    }

    char buffer_utf8[5];
    mjb_codepoint_encode(character.codepoint, buffer_utf8, 5, MJB_ENCODING_UTF_8);
    size_t utf8_length = strnlen(buffer_utf8, 5);

    bool is_json = cmd_output_mode == OUTPUT_MODE_JSON;

    if(is_json) {
        puts("{");
    }

    print_value("Codepoint", "U+%04X", (unsigned int)character.codepoint);
    print_value("Name", "%s", character.name);

    if(is_json) {
        printf("  \"character\": \"%s", color_green_start());

        if(!print_escaped_character(buffer_utf8)) {
            printf("%s", buffer_utf8);
        }

        printf("%s\"\n", color_reset());
    } else {
        print_value("Character", "%s", buffer_utf8);
    }

    // Hex UTF-8
    if(is_json) {
        printf("  \"hex_utf-8\": [%s", color_green_start());
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

    printf("%s%s\n", color_reset(), is_json ? "]," : "");

    print_normalization(buffer_utf8, utf8_length, MJB_NORMALIZATION_NFD, "nfd", "NFD");
    print_normalization(buffer_utf8, utf8_length, MJB_NORMALIZATION_NFKD, "nfkd", "NFKD");

    if(is_json) {
        print_id_name_value("category", character.category, category_name(character.category));
    } else {
        print_value("Category", "[%d] %s", character.category, category_name(character.category));
    }

    char *cc_name = ccc_name(character.combining);
    if(is_json) {
        print_id_name_value("combining", character.combining, cc_name);
    } else {
        print_value("Combining", "[%d] %s", character.combining, cc_name);
    }

    free(cc_name);

    const char *bi_name = bidi_name(character.bidirectional);

    if(is_json) {
        print_id_name_value("bidirectional", character.bidirectional, bi_name);
    } else {
        print_value("Bidirectional", "[%d] %s", character.bidirectional, bi_name);
    }

    mjb_plane plane = mjb_codepoint_plane(character.codepoint);
    const char *plane_name = mjb_plane_name(plane, false);

    if(is_json) {
        print_id_name_value("plane", plane, plane_name);
    } else {
        print_value("Plane", "[%d] %s", plane, plane_name);
    }

    mjb_codepoint_block block = {0};

    bool valid_block = mjb_character_block(character.codepoint, &block);

    if(valid_block) {
        if(is_json) {
            print_id_name_value("block", block.id, block.name);
        } else {
            print_value("Block", "[%d] %s", block.id, block.name);
        }
    }

    const char *d_name = decomposition_name(character.decomposition);

    if(is_json) {
        print_id_name_value("decomposition", character.decomposition, d_name);
    } else {
        print_value("Decomposition", "[%d] %s", character.decomposition, d_name);
    }

    if(character.decimal == MJB_NUMBER_NOT_VALID) {
        print_null_value("Decimal");
    } else {
        print_value("Decimal", "%d", character.decimal);
    }

    if(character.digit == MJB_NUMBER_NOT_VALID) {
        print_null_value("Digit");
    } else {
        print_value("Digit", "%d", character.digit);
    }

    if(character.numeric[0] != '\0') {
        print_value("Numeric", "%s", character.numeric);
    } else {
        print_null_value("Numeric");
    }

    if(is_json  ) {
        printf("  \"mirrored\": %s%s%s,\n", color_green_start(), character.mirrored ? "true" : "false", color_reset());
    } else {
        print_value("Mirrored", "%s", character.mirrored ? "Y" : "N");
    }

    if(character.uppercase != 0) {
        print_value("Uppercase", "%04X", character.uppercase);
    } else {
        print_null_value("Uppercase");
    }

    if(character.lowercase != 0) {
        print_value("Lowercase", "%04X", (unsigned int)character.lowercase);
    } else {
        print_null_value("Lowercase");
    }

    if(character.titlecase != 0) {
        print_value("Titlecase", "%04X", (unsigned int)character.titlecase);
    } else {
        print_null_value("Titlecase");
    }

    if(is_json) {
        puts("}");
    }

    return 0;
}
