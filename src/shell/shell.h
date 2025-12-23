/**
 * The Mojibake shell
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdbool.h>

#include "../mojibake.h"

 typedef enum {
    INTERPRET_MODE_CODEPOINT,
    INTERPRET_MODE_CHARACTER
} mjbsh_interpret_mode;

typedef enum {
    OUTPUT_MODE_PLAIN,
    OUTPUT_MODE_JSON
} mjbsh_output_mode;

extern int cmd_show_colors;
extern unsigned int cmd_verbose;
extern mjbsh_interpret_mode cmd_interpret_mode;
extern mjbsh_output_mode cmd_output_mode;
extern unsigned int cmd_json_indent;
extern unsigned int cmd_width;

bool mjbsh_print_escaped_character(const char *buffer_utf8);

// Color formatting helper functions
const char* mjbsh_green(void);
const char* mjbsh_red(void);
const char* mjbsh_reset(void);
void mjbsh_show_cursor(bool show);

void mjbsh_value(const char* label, unsigned int nl, const char* format, ...);
void mjbsh_null(const char* label, unsigned int nl);
void mjbsh_bool(const char* label, unsigned int nl, bool value);
void mjbsh_numeric(const char* label, unsigned int nl, unsigned int value);
void mjbsh_id_name(const char* label, unsigned int id, const char* name, unsigned int nl);
void mjbsh_normalization(const char *buffer_utf8, size_t utf8_length, mjb_normalization form,
    const char *name, const char *label, unsigned int nl);
void mjbsh_codepoint(const char* label, unsigned int nl, mjb_codepoint codepoint);

bool mjbsh_parse_codepoint(const char *input, mjb_codepoint *codepoint);

const char* mjbsh_json_i(void);
const char* mjbsh_json_nl(void);
