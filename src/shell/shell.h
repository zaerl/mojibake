/**
 * The Mojibake shell
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#ifndef MJB_SHELL_H
#define MJB_SHELL_H

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// clang-format off
#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include "../utf16.h"
    #include "getopt/getopt.h"
    #include <io.h>
    #include <windows.h>
    #define isatty _isatty
    #ifndef STDOUT_FILENO
        #define STDOUT_FILENO _fileno(stdout)
    #endif
#else
    #include <getopt.h>
    #include <signal.h>
    #include <sys/select.h>
    #include <termios.h>
    #include <unistd.h>
#endif
// clang-format on

#include "../mojibake.h"

typedef void (*mjbsh_screen_fn)(const char *input);

typedef enum {
    MJBSH_KEY_UP,
    MJBSH_KEY_DOWN,
    MJBSH_KEY_LEFT,
    MJBSH_KEY_RIGHT
} mjbsh_key;

typedef void (*mjbsh_key_fn)(mjbsh_key key);

typedef enum {
    INTERPRET_MODE_CODEPOINT,
    INTERPRET_MODE_CHARACTER
} mjbsh_interpret_mode;

typedef enum {
    OUTPUT_MODE_PLAIN,
    OUTPUT_MODE_JSON
} mjbsh_output_mode;

typedef int (*mjbsh_command_function)(int argc, char *const argv[], unsigned int flags);

typedef struct mjb_command {
    const char *name;
    const char *description;
    mjbsh_command_function function;
    unsigned int flags;
    int max_arguments;
    bool accepts_codepoint_list;
} mjbsh_command;

extern int cmd_show_colors;
extern bool cmd_show_allowed_symbols;
extern unsigned int cmd_verbose;
extern mjbsh_interpret_mode cmd_interpret_mode;
extern mjbsh_output_mode cmd_output_mode;
extern unsigned int cmd_json_indent;

bool mjbsh_print_escaped_character(const char *buffer_utf8);
void mjbsh_print_json_result(const char *output, size_t output_size);
void mjbsh_print_codepoint(mjb_codepoint codepoint);
void mjbsh_print_break_symbol(mjb_break_type bt);

// Color formatting helper functions
const char *mjbsh_green(void);
const char *mjbsh_red(void);
const char *mjbsh_yellow(void);
const char *mjbsh_reset(void);
void mjbsh_show_cursor(bool show);

void mjbsh_value(const char *label, unsigned int nl, const char *format, ...);
void mjbsh_null(const char *label, unsigned int nl);
void mjbsh_bool(const char *label, unsigned int nl, bool value);
void mjbsh_numeric(const char *label, unsigned int nl, unsigned int value);
void mjbsh_id_name(const char *label, unsigned int id, const char *name, unsigned int nl);
void mjbsh_normalization(const char *buffer_utf8, size_t utf8_length, mjb_normalization form,
    const char *name, const char *label, unsigned int nl);
void mjbsh_codepoint(const char *label, unsigned int nl, mjb_codepoint codepoint);

bool mjbsh_parse_codepoint(const char *input, mjb_codepoint *codepoint);

const char *mjbsh_ji(void);
const char *mjbsh_jnl(void);

// Utils
mjb_codepoint mjbsh_control_picture_codepoint(mjb_codepoint codepoint);
bool mjbsh_property_is_bool(mjb_property property);

// Maps
const char *mjbsh_category_name(mjb_category category);
char *mjbsh_ccc_name(mjb_canonical_combining_class ccc);
const char *mjbsh_bidi_name(mjb_bidi_class bidi);
const char *mjbsh_decomposition_name(mjb_decomposition decomposition);
// const char *mjbsh_line_breaking_class_name(mjb_line_breaking_class line_breaking_class);
const char *mjbsh_east_asian_width_name(mjb_east_asian_width east_asian_width);

// Characters
bool mjbsh_next_character(mjb_character *character, mjb_character_position type);
bool mjbsh_next_array_character(mjb_character *character, mjb_character_position type);
bool mjbsh_next_string_character(mjb_character *character, mjb_character_position type);
bool mjbsh_next_escaped_character(mjb_character *character, mjb_character_position type);

// Screen
void mjbsh_clear_screen(void);
void mjbsh_screen_mode(mjbsh_screen_fn fn, mjbsh_key_fn key_fn);

// Commands
int mjbsh_bidi_command(int argc, char *const argv[], unsigned int flags);
int mjbsh_break_command(int argc, char *const argv[], unsigned int flags);
int mjbsh_case_command(int argc, char *const argv[], unsigned int flags);
int mjbsh_character_command(int argc, char *const argv[], unsigned int flags);
int mjbsh_codepoint_command(int argc, char *const argv[], unsigned int flags);
int mjbsh_emoji_command(int argc, char *const argv[], unsigned int flags);
int mjbsh_filter_command(int argc, char *const argv[], unsigned int flags);
int mjbsh_locale_command(int argc, char *const argv[], unsigned int flags);
int mjbsh_normalize_command(int argc, char *const argv[], unsigned int flags);
int mjbsh_normalize_string_command(int argc, char *const argv[], unsigned int flags);

#endif // MJB_SHELL_H
