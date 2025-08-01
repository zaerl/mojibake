/**
 * The Mojibake shell
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/mojibake.h"
#include "characters.h"
#include "shell.h"

// Global command-line option variables
int cmd_show_colors = 0;
bool cmd_verbose = false;
interpret_mode cmd_interpret_mode = INTERPRET_MODE_CODEPOINT;
output_mode cmd_output_mode = OUTPUT_MODE_PLAIN;

static mjb_codepoint current_codepoint = MJB_CODEPOINT_NOT_VALID;

bool next_current_character(mjb_character *character, mjb_next_character_type type) {
    current_codepoint = character->codepoint;

    return false;
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

    if(buffer_utf8[0] >= 0x00 && buffer_utf8[0] <= 0x1F) {
        printf("\\u%04X", buffer_utf8[0]);

        return true;
    }

    return false;
}

// Color formatting helper functions
const char* color_green_start(void) {
    return cmd_show_colors ? "\x1B[32m" : "";
}

const char* color_reset(void) {
    return cmd_show_colors ? "\x1B[0m" : "";
}

void print_value(const char* label, const char* format, ...) {
    va_list args;
    va_start(args, format);

    if(cmd_output_mode == OUTPUT_MODE_JSON) {
        char json_label[256];

        json_label[0] = tolower(label[0]);
        strncpy(json_label + 1, label + 1, 255);
        json_label[255] = '\0';

        printf("  \"%s\": \"%s", json_label, color_green_start());
    } else {
        printf("%s: %s", label, color_green_start());
    }

    vprintf(format, args);
    printf("%s%s", color_reset(), cmd_output_mode == OUTPUT_MODE_JSON ? "\"" : "");

    if(cmd_output_mode == OUTPUT_MODE_JSON && strcmp(label, "Titlecase") != 0) {
        puts(",");
    } else {
        puts("");
    }

    va_end(args);
}

void print_null_value(const char* label) {
    if(cmd_output_mode == OUTPUT_MODE_JSON) {
        char json_label[256];

        json_label[0] = tolower(label[0]);
        strncpy(json_label + 1, label + 1, 255);
        json_label[255] = '\0';

        printf("  \"%s\": %snull%s", json_label, color_green_start(), color_reset());
    } else {
        printf("%s: %sN/A%s", label, color_green_start(), color_reset());
    }

    if(cmd_output_mode == OUTPUT_MODE_JSON && strcmp(label, "Titlecase") != 0) {
        puts(",");
    } else {
        puts("");
    }
}

void print_id_name_value(const char* label, unsigned int id, const char* name) {
    if(cmd_output_mode != OUTPUT_MODE_JSON) {
        return;
    }

    printf("  \"%s\": {\n    \"code\": %s%u%s,\n    \"value\": \"%s%s%s\"\n  }\n", label,
        color_green_start(), id, color_reset(), color_green_start(), name, color_reset());
}

void print_normalization(char *buffer_utf8, size_t utf8_length, mjb_normalization form, char *name, char *label) {
    bool is_json = cmd_output_mode == OUTPUT_MODE_JSON;
    size_t out_length = 0;

    char *out = mjb_normalize(buffer_utf8, utf8_length, &out_length, MJB_ENCODING_UTF_8, form);

    if(out) {
        if(is_json) {
            printf("  \"%s\": \"%s", name, color_green_start());
            mjb_next_character(out, out_length, MJB_ENCODING_UTF_8, next_escaped_character);
            printf("%s\"\n", color_reset());
        } else {
            print_value(is_json ? name : label, "%s", out);
        }
    } else {
        print_null_value(is_json ? name : label);
    }

    if(is_json) {
        printf("  \"%s_normalization\": [%s", name, color_green_start());
    } else {
        printf("%s normalization: %s", label, color_green_start());
    }

    mjb_next_character(out, out_length, MJB_ENCODING_UTF_8, is_json ? next_array_character : next_character);
    printf("%s%s\n", color_reset(), is_json ? "]," : "");

    free(out);
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
