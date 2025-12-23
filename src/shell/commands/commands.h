/**
 * The Mojibake shell
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "../../mojibake.h"

typedef int (*mjbsh_command_function)(int argc, char * const argv[], unsigned int flags);

typedef struct mjb_command {
    const char *name;
    const char *description;
    mjbsh_command_function function;
    unsigned int flags;
} mjbsh_command;

int mjbsh_break_command(int argc, char * const argv[], unsigned int flags);
int mjbsh_case_command(int argc, char * const argv[], unsigned int flags);
int mjbsh_character_command(int argc, char * const argv[], unsigned int flags);
int mjbsh_codepoint_command(int argc, char * const argv[], unsigned int flags);
int mjbsh_filter_command(int argc, char * const argv[], unsigned int flags);
int mjbsh_normalize_command(int argc, char * const argv[], unsigned int flags);
int mjbsh_normalize_string_command(int argc, char * const argv[], unsigned int flags);
