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
unsigned int cmd_verbose = 0;
mjbsh_interpret_mode cmd_interpret_mode = INTERPRET_MODE_CHARACTER;
mjbsh_output_mode cmd_output_mode = OUTPUT_MODE_PLAIN;
unsigned int cmd_json_indent = 0;
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

// Color table: [0] = no color, [1] = color enabled
typedef enum {
    MJBSH_COLOR_GREEN = 0,
    MJBSH_COLOR_RED,
    MJBSH_COLOR_YELLOW,
    MJBSH_COLOR_RESET
} mjbsh_color_id;

static const char* mjbsh_color_table[2][4] = {
    {"", "", "", ""},
    {"\x1B[32m", "\x1B[31m", "\x1B[33m", "\x1B[0m"}
};

static inline const char* mjbsh_color(mjbsh_color_id id) {
    return mjbsh_color_table[cmd_show_colors != 0][(int)id];
}

static void mjbsh_to_json_key(const char* label, char* buf, size_t bufsize) {
    size_t i = 0;
    while(i < bufsize - 1 && label[i] != '\0') {
        char c = label[i];
        if(c == ' ' || c == '-') {
            c = '_';
        } else {
            c = (char)tolower((unsigned char)c);
        }
        buf[i] = c;
        ++i;
    }
    buf[i] = '\0';
}

static void mjbsh_json_field(const char* label) {
    char key[256];
    mjbsh_to_json_key(label, key, sizeof(key));
    printf("%s%s\"%s\":%s", mjbsh_json_i(), mjbsh_json_i(), key, cmd_json_indent > 0 ? " " : "");
}

static void mjbsh_json_nested_field(const char* label) {
    char key[256];
    mjbsh_to_json_key(label, key, sizeof(key));
    printf("%s%s%s\"%s\":%s", mjbsh_json_i(), mjbsh_json_i(), mjbsh_json_i(), key, cmd_json_indent > 0 ? " " : "");
}

static void mjbsh_print_nl(unsigned int nl) {
    if(cmd_output_mode == OUTPUT_MODE_JSON) {
        printf("%s%s", nl >= 1 ? "," : "", mjbsh_json_nl());
    } else {
        puts("");
    }
}

static bool mjbsh_next_current_character(mjb_character *character, mjb_next_character_type type) {
    current_codepoint = character->codepoint;

    return false;
}

bool mjbsh_print_escaped_character(const char *buffer_utf8) {
    unsigned char c = (unsigned char)buffer_utf8[0];

    switch(c) {
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

    if(c <= 0x1F) {
        printf("\\u%04X", c);

        return true;
    }

    return false;
}

void mjbsh_print_codepoint(mjb_codepoint codepoint) {
    if(codepoint == MJB_CODEPOINT_NOT_VALID) {
        return;
    }

    mjb_codepoint picture_codepoint = mjbsh_control_picture_codepoint(codepoint);
    bool is_control_picture = picture_codepoint != codepoint;

    char buffer_utf8[5];
    mjb_codepoint_encode(picture_codepoint, buffer_utf8, 5, MJB_ENCODING_UTF_8);

    if(is_control_picture) {
        printf("%s%s%s", mjbsh_yellow(), buffer_utf8, mjbsh_reset());
    } else {
        printf("%s", buffer_utf8);
    }
}

void mjbsh_print_break_symbol(mjb_break_type bt) {
    if(bt == MJB_BT_ALLOWED) {
        printf("%s÷%s", mjbsh_green(), mjbsh_reset());
    } else if(bt == MJB_BT_NO_BREAK) {
        printf("%s×%s", mjbsh_red(), mjbsh_reset());
    } else if(bt == MJB_BT_MANDATORY) {
        printf("%s!%s", mjbsh_yellow(), mjbsh_reset());
    }
}

// Color formatting helper functions
const char* mjbsh_green(void) {
    return mjbsh_color(MJBSH_COLOR_GREEN);
}

const char* mjbsh_red(void) {
    return mjbsh_color(MJBSH_COLOR_RED);
}

const char* mjbsh_yellow(void) {
    return mjbsh_color(MJBSH_COLOR_YELLOW);
}

const char* mjbsh_reset(void) {
    return mjbsh_color(MJBSH_COLOR_RESET);
}

void mjbsh_show_cursor(bool show) {
    if(cmd_output_mode == OUTPUT_MODE_JSON) {
        return;
    }

    printf("\x1B[?25%s", show ? "h" : "l");
}

void mjbsh_value(const char* label, unsigned int nl, const char* format, ...) {
    va_list args;
    va_start(args, format);

    if(cmd_output_mode == OUTPUT_MODE_JSON) {
        mjbsh_json_field(label);
        printf("\"%s", mjbsh_green());
    } else {
        printf("%s: %s", label, mjbsh_green());
    }

    vprintf(format, args);
    printf("%s%s", mjbsh_reset(), cmd_output_mode == OUTPUT_MODE_JSON ? "\"" : "");

    mjbsh_print_nl(nl);
    va_end(args);
}

static void mjbsh_print_generic_value(const char* label, unsigned int nl, const char* value) {
    if(cmd_output_mode == OUTPUT_MODE_JSON) {
        mjbsh_json_field(label);
        printf("%s%s%s", mjbsh_green(), value, mjbsh_reset());
    } else {
        printf("%s: %s%s%s", label, mjbsh_green(), value, mjbsh_reset());
    }

    mjbsh_print_nl(nl);
}

void mjbsh_null(const char* label, unsigned int nl) {
    if(cmd_output_mode == OUTPUT_MODE_JSON) {
        mjbsh_print_generic_value(label, nl, "null");
    } else {
        mjbsh_value(label, nl, "N/A");
    }
}

void mjbsh_bool(const char* label, unsigned int nl, bool value) {
    if(cmd_output_mode == OUTPUT_MODE_JSON) {
        mjbsh_print_generic_value(label, nl, value ? "true" : "false");
    } else {
        mjbsh_print_generic_value(label, nl, value ? "Y" : "N");
    }
}

void mjbsh_numeric(const char* label, unsigned int nl, unsigned int value) {
    char num_str[32];
    snprintf(num_str, sizeof(num_str), "%u", value);

    mjbsh_print_generic_value(label, nl, num_str);
}

void mjbsh_id_name(const char* label, unsigned int id, const char* name, unsigned int nl) {
    if(cmd_output_mode != OUTPUT_MODE_JSON) {
        mjbsh_value(label, nl, "[%d] %s", id, name);
        return;
    }

    // {"label": {"code": id, "value": "name"}}
    mjbsh_json_field(label);
    printf("{%s", mjbsh_json_nl());

    mjbsh_json_nested_field("code");
    printf("%s%u%s,%s", mjbsh_green(), id, mjbsh_reset(), mjbsh_json_nl());

    mjbsh_json_nested_field("value");
    printf("\"%s%s%s\"%s%s%s}", mjbsh_green(), name, mjbsh_reset(), mjbsh_json_nl(), mjbsh_json_i(), mjbsh_json_i());

    mjbsh_print_nl(nl);
}

void mjbsh_normalization(const char *buffer_utf8, size_t utf8_length, mjb_normalization form,
    const char *name, const char *label, unsigned int nl) {
    bool is_json = cmd_output_mode == OUTPUT_MODE_JSON;

    mjb_result result;
    bool ret = mjb_normalize(buffer_utf8, utf8_length, MJB_ENCODING_UTF_8, form, &result);

    if(ret) {
        if(is_json) {
            printf("%s%s\"%s\":%s\"%s", mjbsh_json_i(), mjbsh_json_i(), name, cmd_json_indent == 0 ? "" : " ", mjbsh_green());
            mjb_next_character(result.output, result.output_size, MJB_ENCODING_UTF_8, mjbsh_next_escaped_character);
            printf("%s\",%s", mjbsh_reset(), mjbsh_json_nl());
        } else {
            mjbsh_value(is_json ? name : label, true, "%s", result.output);
        }
    } else {
        mjbsh_null(is_json ? name : label, 1);
    }

    if(is_json) {
        printf("%s%s\"%s_normalization\":%s[%s", mjbsh_json_i(), mjbsh_json_i(), name, cmd_json_indent == 0 ? "" : " ", mjbsh_green());
    } else {
        printf("%s normalization: %s", label, mjbsh_green());
    }

    mjb_next_character(result.output, result.output_size, MJB_ENCODING_UTF_8, is_json ? mjbsh_next_array_character : mjbsh_next_character);

    if(is_json) {
        printf("%s]", mjbsh_reset());
    } else {
        printf("%s", mjbsh_reset());
    }

    mjbsh_print_nl(nl);

    if(result.output != NULL && result.output != buffer_utf8) {
        mjb_free(result.output);
    }
}

void mjbsh_codepoint(const char* label, unsigned int nl, mjb_codepoint codepoint) {
    if(cmd_output_mode == OUTPUT_MODE_JSON) {
        mjbsh_json_field(label);
        printf("%s%u%s", mjbsh_green(), (unsigned int)codepoint, mjbsh_reset());
    } else {
        printf("%s: %sU+%04X%s", label, mjbsh_green(), (unsigned int)codepoint, mjbsh_reset());
    }

    mjbsh_print_nl(nl);
}

bool mjbsh_parse_codepoint(const char *input, mjb_codepoint *codepoint) {
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
        mjb_next_character(input, strlen(input), MJB_ENCODING_UTF_8, mjbsh_next_current_character);

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

const char* mjbsh_json_i(void) {
    if(cmd_output_mode != OUTPUT_MODE_JSON) {
        return "";
    }

    return indents[cmd_json_indent];
}

const char* mjbsh_json_nl(void) {
    if(cmd_output_mode != OUTPUT_MODE_JSON) {
        return "";
    }

    return cmd_json_indent == 0 ? "" : "\n";
}

mjb_codepoint mjbsh_control_picture_codepoint(mjb_codepoint codepoint) {
    if(codepoint < 0x20) {
        // Add 0x2400 to the codepoint to make it a printable character by using the
        // "Control Pictures" block.
        codepoint += 0x2400;
    } else if(codepoint == 0x20) {
        codepoint = 0x2423;
    } else if(codepoint == 0x7F) {
        // The delete character.
        codepoint = 0x2421;
    }

    return codepoint;
}

bool mjbsh_property_is_bool(mjb_property property) {
    // See mjb_property enum in unicode.h
    return property == MJB_PR_NFD_QUICK_CHECK || property == MJB_PR_NFKD_QUICK_CHECK ||
        property >= MJB_PR_ASCII_HEX_DIGIT;
}
