/**
 * The Mojibake shell
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <string.h>

#include "../../mojibake.h"

int filter_command(int argc, char * const argv[], unsigned int flags) {
    mjb_result result;
    bool ret = mjb_string_filter(argv[0], strlen(argv[0]), MJB_ENCODING_UTF_8, MJB_ENCODING_UTF_8, (mjb_filter)flags, &result);

    if(!ret) {
        return 1;
    }

    puts(result.output);

    if(result.output != NULL && result.output != argv[0]) {
        mjb_free(result.output);
    }

    return 0;
}
