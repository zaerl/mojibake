/**
 * The Mojibake shell
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../mojibake.h"
#include "../shell.h"
#include "../maps.h"

bool output_next_character(mjb_character *character, mjb_next_character_type type) {
    char buffer_utf8[5];
    unsigned int utf8_length = mjb_codepoint_encode(character->codepoint, buffer_utf8, 5, MJB_ENCODING_UTF_8);

    if(utf8_length == 0) {
        return false;
    }

    bool is_json = cmd_output_mode == OUTPUT_MODE_JSON;

    if(is_json) {
        if(type & MJB_NEXT_CHAR_FIRST) {
            printf("[%s%s{%s", mjbsh_json_nl(), mjbsh_json_i(), mjbsh_json_nl());
        } else {
            printf("%s{%s", mjbsh_json_i(), mjbsh_json_nl());
        }
    } else {
        if(!(type & MJB_NEXT_CHAR_FIRST)) {
            puts("");
        }
    }

    mjbsh_value("Codepoint", 1, "U+%04X", (unsigned int)character->codepoint);
    mjbsh_value("Name", 1, "%s", character->name);

    if(is_json) {
        printf("%s%s\"character\":%s\"%s", mjbsh_json_i(), mjbsh_json_i(), cmd_json_indent == 0 ? "" : " ", mjbsh_green());

        if(!mjbsh_print_escaped_character(buffer_utf8)) {
            printf("%s", buffer_utf8);
        }

        printf("%s\",%s", mjbsh_reset(), mjbsh_json_nl());
    } else {
        mjbsh_value("Character", 1, "%s", buffer_utf8);
    }

    // Hex UTF-8
    if(is_json) {
        printf("%s%s\"hex_utf-8\":%s[%s", mjbsh_json_i(), mjbsh_json_i(), cmd_json_indent == 0 ? "" : " ", mjbsh_green());
    } else {
        printf("Hex UTF-8: %s", mjbsh_green());
    }

    for(size_t i = 0; i < utf8_length; ++i) {
        if(is_json) {
            printf("%u%s", (unsigned char)buffer_utf8[i], i == utf8_length - 1 ? "" : ", ");
        } else {
            printf("%02X%s", (unsigned char)buffer_utf8[i], i == utf8_length - 1 ? "" : " ");
        }
    }

    printf("%s%s", mjbsh_reset(), is_json ? "]" : "");

    if(is_json) {
        printf(",%s", mjbsh_json_nl());
    }

    if(cmd_verbose > 0) {
        mjb_encoding other_encodings[] = {
            MJB_ENCODING_UTF_16_BE,
            MJB_ENCODING_UTF_16_LE,
            MJB_ENCODING_UTF_32_BE,
            MJB_ENCODING_UTF_32_LE
        };

        const char *other_encodings_names[] = {
            "16BE",
            "16LE",
            "32BE",
            "32LE"
        };

        const char *other_encodings_labels[] = {
            "16be",
            "16le",
            "32be",
            "32le"
        };

        for(size_t i = 0; i < 4; ++i) {
            char buffer[5];
            unsigned int length = mjb_codepoint_encode(character->codepoint, buffer, 5, other_encodings[i]);

            if(is_json) {
                printf(
                    "%s%s\"hex_utf-%s\":%s[%s",
                    mjbsh_json_i(), mjbsh_json_i(),
                    other_encodings_labels[i],
                    cmd_json_indent == 0 ? "" : " ", mjbsh_green());
            } else {
                printf("\nHex UTF-%s: %s", other_encodings_names[i], mjbsh_green());
            }

            for(size_t i = 0; i < length; ++i) {
                if(is_json) {
                    printf("%u%s", (unsigned char)buffer[i], i == length - 1 ? "" : ", ");
                } else {
                    printf("%02X%s", (unsigned char)buffer[i], i == length - 1 ? "" : " ");
                }
            }

            printf("%s%s", mjbsh_reset(), is_json ? "]" : "");

            if(is_json) {
                printf(",%s", mjbsh_json_nl());
            }
        }
    }

    if(!is_json) {
        puts("");
    }

    if(cmd_verbose > 0) {
        mjbsh_normalization(buffer_utf8, utf8_length, MJB_NORMALIZATION_NFD, "nfd", "NFD", 1);
        mjbsh_normalization(buffer_utf8, utf8_length, MJB_NORMALIZATION_NFC, "nfc", "NFC", 1);
        mjbsh_normalization(buffer_utf8, utf8_length, MJB_NORMALIZATION_NFKD, "nfkd", "NFKD", 1);
        mjbsh_normalization(buffer_utf8, utf8_length, MJB_NORMALIZATION_NFKD, "nfkc", "NFKC", 1);

        // Need to flush stdout here to ensure the normalization is printed before the next character
        fflush(stdout);
    }

    if(is_json) {
        mjbsh_id_name("category", character->category, mjbsh_category_name(character->category),
            cmd_verbose == 0 ? 0 : 1);
    } else {
        mjbsh_value("Category", cmd_verbose == 0 ? 0 : 1, "[%d] %s", character->category,
            mjbsh_category_name(character->category));
    }

    if(cmd_verbose > 0) {
        char *cc_name = mjbsh_ccc_name(character->combining);

        if(is_json) {
            mjbsh_id_name("combining", character->combining, cc_name, 1);
        } else {
            mjbsh_value("Combining", 1, "[%d] %s", character->combining, cc_name);
        }

        free(cc_name);

        const char *bi_name = mjbsh_bidi_name((mjb_bidi_class)character->bidirectional);

        if(is_json) {
            mjbsh_id_name("bidirectional", character->bidirectional, bi_name, 1);
        } else {
            mjbsh_value("Bidirectional", 1, "[%d] %s", character->bidirectional, bi_name);
        }

        mjb_plane plane = mjb_codepoint_plane(character->codepoint);
        const char *plane_name = mjb_plane_name(plane, false);

        if(is_json) {
            mjbsh_id_name("plane", plane, plane_name, 1);
        } else {
            mjbsh_value("Plane", 1, "[%d] %s", plane, plane_name);
        }

        mjb_block_info block;
        bool valid_block = mjb_codepoint_block(character->codepoint, &block);

        // Need to flush stdout here to ensure the block is printed before the next character
        fflush(stdout);

        if(valid_block) {
            if(is_json) {
                mjbsh_id_name("block", block.id, block.name, 1);
            } else {
                mjbsh_value("Block", 1, "[%d] %s", block.id, block.name);
            }
        }

        const char *d_name = mjbsh_decomposition_name(character->decomposition);

        if(is_json) {
            mjbsh_id_name("decomposition", character->decomposition, d_name, 1);
        } else {
            mjbsh_value("Decomposition", 1, "[%d] %s", character->decomposition, d_name);
        }
    }

    if(cmd_verbose > 0) {
        if(character->decimal == MJB_NUMBER_NOT_VALID) {
            mjbsh_null("Decimal", 1);
        } else {
            mjbsh_numeric("Decimal", 1, character->decimal);
        }

        if(character->digit == MJB_NUMBER_NOT_VALID) {
            mjbsh_null("Digit", 1);
        } else {
            mjbsh_numeric("Digit", 1, character->digit);
        }

        if(character->numeric[0] != '\0') {
            mjbsh_value("Numeric", 1, "%s", character->numeric);
        } else {
            mjbsh_null("Numeric", 1);
        }

        mjbsh_bool("Mirrored", 1, character->mirrored);

        if(character->uppercase != 0) {
            mjbsh_codepoint("Uppercase", 1, character->uppercase);
        } else {
            mjbsh_null("Uppercase", 1);
        }

        if(character->lowercase != 0) {
            mjbsh_codepoint("Lowercase", 1, (unsigned int)character->lowercase);
        } else {
            mjbsh_null("Lowercase", 1);
        }

        if(character->titlecase != 0) {
            mjbsh_codepoint("Titlecase", cmd_verbose == 1 ? 0 : 1, character->titlecase);
        } else {
            mjbsh_null("Titlecase", cmd_verbose == 1 ? 0 : 1);
        }
    }

    if(cmd_verbose > 1) {
        mjb_east_asian_width east_asian_width;
        bool eaw_valid = mjb_codepoint_east_asian_width(character->codepoint, &east_asian_width);

        if(eaw_valid) {
            if(is_json) {
                mjbsh_id_name("east_asian_width", east_asian_width,
                    mjbsh_east_asian_width_name(east_asian_width), 1);
            } else {
                mjbsh_value("East Asian Width", 1, "[%d] %s", east_asian_width,
                    mjbsh_east_asian_width_name(east_asian_width));
            }
        } else {
            mjbsh_null("East Asian Width", 1);
        }

        mjb_emoji_properties emoji_properties;
        bool emoji_valid = mjb_codepoint_emoji(character->codepoint, &emoji_properties);

        if(emoji_valid) {
            mjbsh_bool("Emoji", 1, emoji_properties.emoji);
            mjbsh_bool(is_json ? "emoji_presentation" : "Emoji Presentation", 1, emoji_properties.presentation);
            mjbsh_bool(is_json ? "emoji_modifier" : "Emoji Modifier", 1, emoji_properties.modifier);
            mjbsh_bool(is_json ? "emoji_modifier_base" : "Emoji Modifier Base", 1, emoji_properties.modifier_base);
            mjbsh_bool(is_json ? "emoji_component" : "Emoji Component", 1, emoji_properties.component);
            mjbsh_bool(is_json ? "extended_pictographic" : "Extended Pictographic", 0, emoji_properties.extended_pictographic);
        } else {
            mjbsh_null("Emoji", 1);
            mjbsh_null(is_json ? "emoji_presentation" : "Emoji Presentation", 1);
            mjbsh_null(is_json ? "emoji_modifier" : "Emoji Modifier", 1);
            mjbsh_null(is_json ? "emoji_modifier_base" : "Emoji Modifier Base", 1);
            mjbsh_null(is_json ? "emoji_component" : "Emoji Component", 1);
            mjbsh_null(is_json ? "extended_pictographic" : "Extended Pictographic", 0);
        }
    }

    if(cmd_verbose >= 2) {
        uint8_t properties[MJB_PR_BUFFER_SIZE];
        bool ret = mjb_codepoint_properties(character->codepoint, properties);

        if(ret) {
            for(size_t i = 0; i < MJB_PR_COUNT; ++i) {
                if(mjbsh_property_is_bool(i)) {
                    if(properties[i]) {
                        mjbsh_bool(mjbsh_property_name(i), 1, true);
                    }
                } else {
                    if(properties[i] != 0) {
                        mjbsh_value(mjbsh_property_name(i), 1, "%d", properties[i]);
                    }
                }
            }
        }
    }

    if(is_json) {
        printf("%s}%s%s", mjbsh_json_i(), (type & MJB_NEXT_CHAR_LAST) ? "" : ",", mjbsh_json_nl());

        if(type & MJB_NEXT_CHAR_LAST) {
            printf("]%s", mjbsh_json_nl());
        }
    }

    return true;
}

int mjbsh_character_command(int argc, char * const argv[], unsigned int flags) {
    mjb_next_character(argv[0], strlen(argv[0]), MJB_ENCODING_UTF_8, output_next_character);

    return 0;
}
