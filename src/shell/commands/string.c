/**
 * The Mojibake shell
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "../shell.h"

int mjbsh_case_command(int argc, char *const argv[], unsigned int flags) {
    mjb_result result = { NULL, 0, false };

    if(mjb_case(argv[0], strlen(argv[0]), MJB_ENC_UTF_8, (mjb_case_type)flags, MJB_ENC_UTF_8,
           &result) != MJB_STATUS_OK) {
        return 1;
    }

    puts(result.output);

    if(result.transformed) {
        mjb_free(result.output);
    }

    return 0;
}
