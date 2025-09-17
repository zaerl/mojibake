/**
 * The Mojibake shell
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../mojibake.h"
#include "../shell.h"
#include "../characters.h"
#include "commands.h"

int normalize_string_command(int argc, char * const argv[], unsigned int flags) {
    mjb_result result;

    bool ret = mjb_normalize(argv[0], strlen(argv[0]), MJB_ENCODING_UTF_8, (mjb_normalization)flags, &result);

    if(!ret) {
        fprintf(stderr, cmd_verbose ? "Invalid\n" : "N\n");

        return 1;
    }

    printf("%s", color_green_start());
    mjb_next_character(result.output, result.output_size, MJB_ENCODING_UTF_8, next_string_character);
    printf("%s", color_reset());
    puts("");

    if(result.output != NULL && result.output != argv[0]) {
        mjb_free(result.output);
    }

    return 0;
}

int normalize_command(int argc, char * const argv[], unsigned int flags) {
    if(cmd_interpret_mode == INTERPRET_MODE_CHARACTER) {
        return normalize_string_command(argc, argv, flags);
    }

    unsigned int index = 0;
    // 5 bytes per codepoint is more than enough.
    char* codepoints = (char*)malloc(argc * 5);

    for(int i = 0; i < argc; ++i) {
        mjb_codepoint codepoint = 0;

        if(!parse_codepoint(argv[i], &codepoint)) {
            fprintf(stderr, cmd_verbose ? "Invalid\n" : "N\n");

            free(codepoints);

            return 1;
        }

        index += mjb_codepoint_encode(codepoint, codepoints + index, (argc * 5) - index, MJB_ENCODING_UTF_8);
    }

    codepoints[++index] = '\0';

    mjb_result result;
    bool ret = mjb_normalize(codepoints, index, MJB_ENCODING_UTF_8, (mjb_normalization)flags, &result);

    if(!ret) {
        fprintf(stderr, cmd_verbose ? "Invalid\n" : "N\n");

        free(codepoints);

        return 1;
    }

    mjb_next_character(result.output, result.output_size, MJB_ENCODING_UTF_8, next_character);
    puts("");

    if(result.output != NULL && result.output != codepoints) {
        mjb_free(result.output);
    }

    free(codepoints);

    return 0;
}
