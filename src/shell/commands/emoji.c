/**
 * The Mojibake shell
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "../shell.h"

static unsigned int mjbsh_emoji_json_character_count = 0;

static const char *mjbsh_emoji_sequence_type_name(mjb_emoji_sequence_type type) {
    switch(type) {
        case MJB_EMOJI_SEQUENCE_NONE:
            return "None";
        case MJB_EMOJI_SEQUENCE_BASIC:
            return "Basic";
        case MJB_EMOJI_SEQUENCE_KEYCAP:
            return "Keycap";
        case MJB_EMOJI_SEQUENCE_FLAG:
            return "Flag";
        case MJB_EMOJI_SEQUENCE_TAG:
            return "Tag";
        case MJB_EMOJI_SEQUENCE_MODIFIER:
            return "Modifier";
        case MJB_EMOJI_SEQUENCE_ZWJ:
            return "ZWJ";
        case MJB_EMOJI_SEQUENCE_TEXT_VARIATION:
            return "Text variation";
        case MJB_EMOJI_SEQUENCE_EMOJI_VARIATION:
            return "Emoji variation";
    }

    return "Unknown";
}

static const char *mjbsh_emoji_qualification_name(mjb_emoji_qualification qualification) {
    switch(qualification) {
        case MJB_EMOJI_QUALIFICATION_NONE:
            return "None";
        case MJB_EMOJI_QUALIFICATION_COMPONENT:
            return "Component";
        case MJB_EMOJI_QUALIFICATION_FULLY_QUALIFIED:
            return "Fully-qualified";
        case MJB_EMOJI_QUALIFICATION_MINIMALLY_QUALIFIED:
            return "Minimally-qualified";
        case MJB_EMOJI_QUALIFICATION_UNQUALIFIED:
            return "Unqualified";
    }

    return "Unknown";
}

static const char *mjbsh_emoji_json_space(void) {
    return cmd_json_indent == 0 ? "" : " ";
}

static void mjbsh_emoji_json_indent(unsigned int level) {
    for(unsigned int i = 0; i < level; ++i) {
        printf("%s", mjbsh_ji());
    }
}

static void mjbsh_emoji_json_string(const char *value) {
    if(value == NULL) {
        value = "";
    }

    putchar('"');

    for(const unsigned char *p = (const unsigned char *)value; *p != '\0'; ++p) {
        char c = (char)*p;

        if(!mjbsh_print_escaped_character(&c)) {
            putchar(c);
        }
    }

    putchar('"');
}

static void mjbsh_emoji_json_string_field(const char *name, const char *value, unsigned int level,
    bool comma) {
    mjbsh_emoji_json_indent(level);
    printf("\"%s\":%s", name, mjbsh_emoji_json_space());
    mjbsh_emoji_json_string(value);
    printf("%s%s", comma ? "," : "", mjbsh_jnl());
}

static void mjbsh_emoji_json_bool_field(const char *name, bool value, unsigned int level,
    bool comma) {
    mjbsh_emoji_json_indent(level);
    printf("\"%s\":%s%s%s", name, mjbsh_emoji_json_space(), value ? "true" : "false",
        comma ? "," : "");
    printf("%s", mjbsh_jnl());
}

static void mjbsh_emoji_json_size_field(const char *name, size_t value, unsigned int level,
    bool comma) {
    mjbsh_emoji_json_indent(level);
    printf("\"%s\":%s%zu%s", name, mjbsh_emoji_json_space(), value, comma ? "," : "");
    printf("%s", mjbsh_jnl());
}

static void mjbsh_emoji_json_id_field(const char *name, unsigned int code, const char *value,
    unsigned int level, bool comma) {
    mjbsh_emoji_json_indent(level);
    printf("\"%s\":%s{%s", name, mjbsh_emoji_json_space(), mjbsh_jnl());

    mjbsh_emoji_json_size_field("code", code, level + 1, true);
    mjbsh_emoji_json_string_field("value", value, level + 1, false);

    mjbsh_emoji_json_indent(level);
    printf("}%s%s", comma ? "," : "", mjbsh_jnl());
}

static bool mjbsh_emoji_next_character(mjb_character *character, mjb_character_position type) {
    char buffer_utf8[5];
    unsigned int utf8_length = mjb_codepoint_encode(character->codepoint, buffer_utf8,
        sizeof(buffer_utf8), MJB_ENC_UTF_8);
    mjb_emoji_properties emoji;
    bool emoji_valid = mjb_codepoint_emoji_properties(character->codepoint, &emoji) ==
        MJB_STATUS_OK;

    if(utf8_length == 0) {
        return false;
    }

    if(cmd_output_mode == OUTPUT_MODE_JSON) {
        char codepoint[16];

        snprintf(codepoint, sizeof(codepoint), "U+%04X", (unsigned int)character->codepoint);
        if(type & MJB_POSITION_FIRST) {
            printf("%s", mjbsh_jnl());
        } else {
            printf(",%s", mjbsh_jnl());
        }
        mjbsh_emoji_json_indent(2);
        printf("{%s", mjbsh_jnl());

        mjbsh_emoji_json_string_field("codepoint", codepoint, 3, true);
        mjbsh_emoji_json_string_field("name", character->name, 3, true);
        mjbsh_emoji_json_string_field("character", buffer_utf8, 3, true);
        mjbsh_emoji_json_bool_field("emoji_data", emoji_valid, 3, true);
        mjbsh_emoji_json_bool_field("emoji", emoji_valid && emoji.emoji, 3, true);
        mjbsh_emoji_json_bool_field("emoji_presentation", emoji_valid && emoji.presentation, 3,
            true);
        mjbsh_emoji_json_bool_field("emoji_modifier", emoji_valid && emoji.modifier, 3, true);
        mjbsh_emoji_json_bool_field("emoji_modifier_base", emoji_valid && emoji.modifier_base, 3,
            true);
        mjbsh_emoji_json_bool_field("emoji_component", emoji_valid && emoji.component, 3, true);
        mjbsh_emoji_json_bool_field("extended_pictographic",
            emoji_valid && emoji.extended_pictographic, 3, false);

        mjbsh_emoji_json_indent(2);
        printf("}");

        if(type & MJB_POSITION_LAST) {
            printf("%s", mjbsh_jnl());
        }

        ++mjbsh_emoji_json_character_count;

        return true;
    }

    if(type & MJB_POSITION_FIRST) {
        puts("");
        puts("Characters:");
    } else {
        puts("");
    }

    mjbsh_value("Codepoint", 1, "U+%04X", (unsigned int)character->codepoint);
    mjbsh_value("Name", 1, "%s", character->name);
    mjbsh_value("Character", 1, "%s", buffer_utf8);
    mjbsh_bool("Emoji Data", 1, emoji_valid);
    mjbsh_bool("Emoji", 1, emoji_valid && emoji.emoji);
    mjbsh_bool("Emoji Presentation", 1, emoji_valid && emoji.presentation);
    mjbsh_bool("Emoji Modifier", 1, emoji_valid && emoji.modifier);
    mjbsh_bool("Emoji Modifier Base", 1, emoji_valid && emoji.modifier_base);
    mjbsh_bool("Emoji Component", 1, emoji_valid && emoji.component);
    mjbsh_bool("Extended Pictographic", 1, emoji_valid && emoji.extended_pictographic);

    return true;
}

static bool mjbsh_emoji_input_from_codepoints(int argc, char *const argv[], char **buffer,
    size_t *size) {
    size_t buffer_size = ((size_t)argc * 4) + 1;
    size_t index = 0;
    char *codepoints = (char *)malloc(buffer_size);

    if(codepoints == NULL) {
        return false;
    }

    for(int i = 0; i < argc; ++i) {
        mjb_codepoint codepoint = 0;

        if(!mjbsh_parse_codepoint(argv[i], &codepoint)) {
            free(codepoints);

            return false;
        }

        unsigned int written = mjb_codepoint_encode(codepoint, codepoints + index,
            buffer_size - index, MJB_ENC_UTF_8);

        if(written == 0) {
            free(codepoints);

            return false;
        }

        index += written;
    }

    codepoints[index] = '\0';
    *buffer = codepoints;
    *size = index;

    return true;
}

static void mjbsh_emoji_print_json(const char *buffer, size_t byte_length, bool is_emoji_sequence,
    bool is_rgi, const mjb_emoji_sequence *emoji) {
    printf("{%s", mjbsh_jnl());

    mjbsh_emoji_json_string_field("input", buffer, 1, true);
    mjbsh_emoji_json_bool_field("emoji_sequence", is_emoji_sequence, 1, true);
    mjbsh_emoji_json_bool_field("rgi_emoji", is_rgi, 1, true);
    mjbsh_emoji_json_id_field("sequence_type", (unsigned int)emoji->type,
        mjbsh_emoji_sequence_type_name(emoji->type), 1, true);
    mjbsh_emoji_json_id_field("qualification", (unsigned int)emoji->qualification,
        mjbsh_emoji_qualification_name(emoji->qualification), 1, true);
    mjbsh_emoji_json_size_field("sequence_codepoints", emoji->codepoint_count, 1, true);

    mjbsh_emoji_json_indent(1);
    printf("\"characters\":%s[", mjbsh_emoji_json_space());

    mjbsh_emoji_json_character_count = 0;

    if(mjb_string_each_character(buffer, byte_length, MJB_ENC_UTF_8, mjbsh_emoji_next_character) !=
        MJB_STATUS_OK) {
        printf("]%s", mjbsh_jnl());
        printf("}%s", mjbsh_jnl());

        return;
    }

    if(mjbsh_emoji_json_character_count > 0) {
        mjbsh_emoji_json_indent(1);
    }

    printf("]%s", mjbsh_jnl());
    printf("}%s", mjbsh_jnl());
}

static void mjbsh_emoji_print_plain(const char *buffer, size_t byte_length, bool is_emoji_sequence,
    bool is_rgi, const mjb_emoji_sequence *emoji) {
    mjbsh_value("Input", 1, "%s", buffer);
    mjbsh_bool("Emoji Sequence", 1, is_emoji_sequence);
    mjbsh_bool("RGI Emoji", 1, is_rgi);
    mjbsh_id_name("Sequence Type", (unsigned int)emoji->type,
        mjbsh_emoji_sequence_type_name(emoji->type), 1);
    mjbsh_id_name("Qualification", (unsigned int)emoji->qualification,
        mjbsh_emoji_qualification_name(emoji->qualification), 1);
    mjbsh_numeric("Sequence Codepoints", 1, (unsigned int)emoji->codepoint_count);

    if(mjb_string_each_character(buffer, byte_length, MJB_ENC_UTF_8, mjbsh_emoji_next_character) !=
        MJB_STATUS_OK) {
        return;
    }
}

int mjbsh_emoji_command(int argc, char *const argv[], unsigned int flags) {
    char *buffer = argv[0];
    size_t size = strlen(buffer);
    bool should_free = false;

    (void)flags;

    if(cmd_interpret_mode == INTERPRET_MODE_CODEPOINT) {
        if(!mjbsh_emoji_input_from_codepoints(argc, argv, &buffer, &size)) {
            fprintf(stderr, cmd_verbose ? "Invalid\n" : "N\n");

            return 1;
        }

        should_free = true;
    }

    mjb_emoji_sequence emoji;
    memset(&emoji, 0, sizeof(emoji));

    bool is_emoji_sequence = mjb_is_emoji_sequence(buffer, size, MJB_ENC_UTF_8);
    bool has_sequence_metadata = mjb_classify_emoji_sequence(buffer, size, MJB_ENC_UTF_8, &emoji) ==
        MJB_STATUS_OK;
    bool is_rgi = mjb_is_rgi_emoji(buffer, size, MJB_ENC_UTF_8);

    if(!has_sequence_metadata) {
        emoji.type = MJB_EMOJI_SEQUENCE_NONE;
        emoji.qualification = MJB_EMOJI_QUALIFICATION_NONE;
        emoji.codepoint_count = 0;
    }

    if(cmd_output_mode == OUTPUT_MODE_JSON) {
        mjbsh_emoji_print_json(buffer, size, is_emoji_sequence, is_rgi, &emoji);
    } else {
        mjbsh_emoji_print_plain(buffer, size, is_emoji_sequence, is_rgi, &emoji);
    }

    if(should_free) {
        free(buffer);
    }

    return 0;
}
