/**
 * The Mojibake shell
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "../../mojibake.h"

typedef int (*command_function)(int argc, char * const argv[], unsigned int flags);

typedef struct {
    const char *name;
    const char *description;
    command_function function;
    unsigned int flags;
} command;

int character_command(int argc, char * const argv[], unsigned int flags);
int normalize_command(int argc, char * const argv[], unsigned int flags);
int normalize_string_command(int argc, char * const argv[], unsigned int flags);
int case_command(int argc, char * const argv[], unsigned int flags);
