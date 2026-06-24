/**
 * The Mojibake shell
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#include "../../mojibake.h"
#include "commands.h"

int mjbsh_codepoint_command(int argc, char * const argv[], unsigned int flags) {
    char buffer[5];
    char *endptr = NULL;

    errno = 0;
    unsigned long value = strtoul(argv[0], &endptr, 16);

    if(endptr == argv[0] || *endptr != '\0' || errno == ERANGE || value > MJB_CODEPOINT_MAX) {
        return 1;
    }

    mjb_codepoint codepoint = (mjb_codepoint)value;
    unsigned int length = mjb_codepoint_encode(codepoint, buffer, 5, MJB_ENCODING_UTF_8);

    if(length == 0) {
        return 1;
    }

    char *argv_buffer[] = { buffer };

    return mjbsh_character_command(1, argv_buffer, flags);
}
