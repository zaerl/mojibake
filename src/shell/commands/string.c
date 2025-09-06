/**
 * The Mojibake shell
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <string.h>

#include "../../mojibake.h"
#include "commands.h"

static char *get_case_string(const char *buffer, size_t length, mjb_case_type type, mjb_encoding encoding) {
    return mjb_case(buffer, length, type, encoding);
}

int case_command(int argc, char * const argv[], unsigned int flags) {
    char *output = get_case_string(argv[0], strlen(argv[0]), (mjb_case_type)flags, MJB_ENCODING_UTF_8);

    if(output == NULL) {
        return 1;
    }

    puts(output);

    if(output != argv[0]) {
        mjb_free(output);
    }

    return 0;
 }
