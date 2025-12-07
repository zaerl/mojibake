/**
 * The Mojibake shell
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdbool.h>

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <conio.h>
    #include <io.h>
    #define STDIN_FILENO _fileno(stdin)

    // Windows terminal state structure
    typedef struct mjb_terminal_state {
        HANDLE h_stdin;
        DWORD orig_mode;
    } terminal_state;
#else
    #include <termios.h>
    #include <unistd.h>

    // Unix terminal state structure
    typedef struct termios terminal_state;
#endif

#include "../mojibake.h"

 typedef enum {
    INTERPRET_MODE_CODEPOINT,
    INTERPRET_MODE_CHARACTER
} interpret_mode;

typedef enum {
    OUTPUT_MODE_PLAIN,
    OUTPUT_MODE_JSON
} output_mode;

extern int cmd_show_colors;
extern unsigned int cmd_verbose;
extern interpret_mode cmd_interpret_mode;
extern output_mode cmd_output_mode;
extern unsigned int cmd_json_indent;
extern unsigned int cmd_width;

bool print_escaped_character(const char *buffer_utf8);

// Color formatting helper functions
const char* color_green_start(void);
const char* color_red_start(void);
const char* color_reset(void);

// Terminal helper functions
void clear_screen(void);
void set_raw_mode(terminal_state *term_state);
void restore_mode(terminal_state *term_state);

void print_value(const char* label, unsigned int nl, const char* format, ...);
void print_null_value(const char* label, unsigned int nl);
void print_bool_value(const char* label, unsigned int nl, bool value);
void print_id_name_value(const char* label, unsigned int id, const char* name, unsigned int nl);
void print_normalization(const char *buffer_utf8, size_t utf8_length, mjb_normalization form,
    const char *name, const char *label, unsigned int nl);
void print_codepoint(const char* label, unsigned int nl, mjb_codepoint codepoint);

bool parse_codepoint(const char *input, mjb_codepoint *codepoint);

const char* json_i(void);
const char* json_nl(void);
