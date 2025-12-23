/**
 * The Mojibake shell
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>

#include "commands.h"

int mjbsh_codepoint_command(int argc, char * const argv[], unsigned int flags) {
    char buffer[5];

    mjb_codepoint codepoint;

    if(sscanf(argv[0], "%x", &codepoint) != 1) {
        return 1;
    }

    unsigned int length = mjb_codepoint_encode(codepoint, buffer, 5, MJB_ENCODING_UTF_8);

    if(length == 0) {
        return 1;
    }

    char *argv_buffer[] = { buffer };

    return mjbsh_character_command(1, argv_buffer, flags);
}
