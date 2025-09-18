/**
 * The Mojibake shell
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../mojibake.h"
#include "characters.h"
#include "shell.h"

// Global command-line option variables
int cmd_show_colors = 0;
bool cmd_verbose = false;
interpret_mode cmd_interpret_mode = INTERPRET_MODE_CHARACTER;
output_mode cmd_output_mode = OUTPUT_MODE_PLAIN;
unsigned int cmd_json_indent = 2;
unsigned int cmd_width = 80;

static mjb_codepoint current_codepoint = MJB_CODEPOINT_NOT_VALID;

// JSON indent levels. Having such an array speeds up the code.
static const char* indents[11] = {
    "",
    " ",
    "  ",
    "    ",
    "      ",
    "        ",
    "          ",
    "            ",
    "              ",
    "                ",
    "                  ",
};

void print_nl(unsigned int nl) {
    if(cmd_output_mode == OUTPUT_MODE_JSON) {
        printf("%s%s", nl >= 1 ? "," : "", json_nl());
    } else {
        puts("");
    }
}

bool next_current_character(mjb_character *character, mjb_next_character_type type) {
    current_codepoint = character->codepoint;

    return false;
}

void clear_screen(void) {
    printf("\033[2J\033[H");
    fflush(stdout);
}

void set_raw_mode(struct termios *orig_termios) {
    struct termios raw = *orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void restore_mode(struct termios *orig_termios) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, orig_termios);
}

bool print_escaped_character(char buffer_utf8[5]) {
    switch(buffer_utf8[0]) {
        case '"':
            printf("\\\"");
            return true;
        case '\\':
            printf("\\\\");
            return true;
        case '\b':
            printf("\\b");
            return true;
        case '\f':
            printf("\\f");
            return true;
        case '\n':
            printf("\\n");
            return true;
        case '\r':
            printf("\\r");
            return true;
        case '\t':
            printf("\\t");
            return true;
    }

    if(buffer_utf8[0] <= 0x1F) {
        printf("\\u%04X", buffer_utf8[0]);

        return true;
    }

    return false;
}

// Color formatting helper functions
const char* color_green_start(void) {
    return cmd_show_colors ? "\x1B[32m" : "";
}

const char* color_red_start(void) {
    return cmd_show_colors ? "\x1B[31m" : "";
}

const char* color_reset(void) {
    return cmd_show_colors ? "\x1B[0m" : "";
}

void print_value(const char* label, unsigned int nl, const char* format, ...) {
    va_list args;
    va_start(args, format);

    if(cmd_output_mode == OUTPUT_MODE_JSON) {
        char json_label[256];

        json_label[0] = tolower(label[0]);
        strncpy(json_label + 1, label + 1, 255);
        json_label[255] = '\0';

        printf("%s%s\"%s\":%s\"%s", json_i(), json_i(), json_label,
            cmd_json_indent == 0 ? "" : " ", color_green_start());
    } else {
        printf("%s: %s", label, color_green_start());
    }

    vprintf(format, args);
    printf("%s%s", color_reset(), cmd_output_mode == OUTPUT_MODE_JSON ? "\"" : "");

    print_nl(nl);
    va_end(args);
}

void print_generic_value(const char* label, unsigned int nl, const char* value) {
    if(cmd_output_mode == OUTPUT_MODE_JSON) {
        char json_label[256];

        json_label[0] = tolower(label[0]);
        strncpy(json_label + 1, label + 1, 255);
        json_label[255] = '\0';

        printf("%s%s\"%s\":%s%s%s%s", json_i(), json_i(), json_label, cmd_json_indent == 0 ? "" : " ",
            color_green_start(), value, color_reset());
    } else {
        printf("%s: %s%s%s", label, color_green_start(), value, color_reset());
    }

    print_nl(nl);
}

void print_null_value(const char* label, unsigned int nl) {
    if(cmd_output_mode == OUTPUT_MODE_JSON) {
        print_generic_value(label, nl, "null");
    } else {
        print_value(label, nl, "N/A");
    }
}

void print_bool_value(const char* label, unsigned int nl, bool value) {
    if(cmd_output_mode == OUTPUT_MODE_JSON) {
        print_generic_value(label, nl, value ? "true" : "false");
    } else {
        print_generic_value(label, nl, value ? "Y" : "N");
    }
}

void print_id_name_value(const char* label, unsigned int id, const char* name, unsigned int nl) {
    if(cmd_output_mode != OUTPUT_MODE_JSON) {
        return;
    }

    const char* val_indent = cmd_json_indent == 0 ? "" : " ";

    // {"label": {"code": id, "value": "name"}}
    printf("%s%s\"%s\":%s{%s", json_i(), json_i(), label, val_indent, json_nl());

    printf("%s%s%s\"code\":%s%s%u%s,%s",
        json_i(), json_i(), json_i(), val_indent,
        color_green_start(), id, color_reset(), json_nl());

    printf("%s%s%s\"value\":%s\"%s%s%s\"%s%s%s}",
        json_i(), json_i(), json_i(), val_indent,
        color_green_start(), name, color_reset(), json_nl(), json_i(), json_i());

    print_nl(nl);
}

void print_normalization(const char *buffer_utf8, size_t utf8_length, mjb_normalization form,
    const char *name, const char *label, unsigned int nl) {
    bool is_json = cmd_output_mode == OUTPUT_MODE_JSON;

    mjb_result result;
    bool ret = mjb_normalize(buffer_utf8, utf8_length, MJB_ENCODING_UTF_8, form, &result);

    if(ret) {
        if(is_json) {
            printf("%s%s\"%s\":%s\"%s", json_i(), json_i(), name, cmd_json_indent == 0 ? "" : " ", color_green_start());
            mjb_next_character(result.output, result.output_size, MJB_ENCODING_UTF_8, next_escaped_character);
            printf("%s\",%s", color_reset(), json_nl());
        } else {
            print_value(is_json ? name : label, true, "%s", result.output);
        }
    } else {
        print_null_value(is_json ? name : label, 1);
    }

    if(is_json) {
        printf("%s%s\"%s_normalization\":%s[%s", json_i(), json_i(), name, cmd_json_indent == 0 ? "" : " ", color_green_start());
    } else {
        printf("%s normalization: %s", label, color_green_start());
    }

    mjb_next_character(result.output, result.output_size, MJB_ENCODING_UTF_8, is_json ? next_array_character : next_character);

    if(is_json) {
        printf("%s]", color_reset());
    } else {
        printf("%s", color_reset());
    }

    print_nl(nl);

    if(result.output != NULL && result.output != buffer_utf8) {
        mjb_free(result.output);
    }
}

void print_codepoint(const char* label, unsigned int nl, mjb_codepoint codepoint) {
    if(cmd_output_mode == OUTPUT_MODE_JSON) {
        char json_label[256];

        json_label[0] = tolower(label[0]);
        strncpy(json_label + 1, label + 1, 255);
        json_label[255] = '\0';

        printf("%s%s\"%s\":%s%s%u%s", json_i(), json_i(), json_label,
            cmd_json_indent == 0 ? "" : " ", color_green_start(), (unsigned int)codepoint, color_reset());
    } else {
        printf("%s: %sU+%04X%s", label, color_green_start(), (unsigned int)codepoint, color_reset());
    }

    print_nl(nl);
}

bool parse_codepoint(const char *input, mjb_codepoint *codepoint) {
    char *endptr;
    mjb_codepoint value = 0;

    if(cmd_interpret_mode == INTERPRET_MODE_CODEPOINT) {
        if(strncmp(input, "U+", 2) == 0 || strncmp(input, "u+", 2) == 0) {
            // Parse as hex after "U+" prefix
            value = strtoul(input + 2, &endptr, 16);
        } else {
            // Try parsing as hex
            value = strtoul(input, &endptr, 16);
        }
    } else {
        mjb_next_character(input, strlen(input), MJB_ENCODING_UTF_8, next_current_character);

        if(current_codepoint == MJB_CODEPOINT_NOT_VALID) {
            return false;
        }

        *codepoint = current_codepoint;
        current_codepoint = MJB_CODEPOINT_NOT_VALID;

        return true;
    }

    if(*endptr != '\0') {
        return false;
    }

    *codepoint = value;

    return true;
}

const char* json_i(void) {
    if(cmd_output_mode != OUTPUT_MODE_JSON) {
        return "";
    }

    return indents[cmd_json_indent];
}

const char* json_nl(void) {
    if(cmd_output_mode != OUTPUT_MODE_JSON) {
        return "";
    }

    return cmd_json_indent == 0 ? "" : "\n";
}
