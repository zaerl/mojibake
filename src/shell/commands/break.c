/**
 * The Mojibake shell
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../mojibake.h"
#include "../../utf8.h"
#include "../shell.h"
#include "commands.h"

int break_command(int argc, char * const argv[], unsigned int flags) {
    size_t input_size = strlen(argv[0]);
    size_t input_real_size = mjb_strnlen(argv[0], input_size, MJB_ENCODING_UTF_8);
    size_t output_size = 0;
    mjb_line_break *line_breaks = mjb_break_line(argv[0], mjb_strnlen(argv[0], input_size,
        MJB_ENCODING_UTF_8), MJB_ENCODING_UTF_8, &output_size);

    if(line_breaks == NULL) {
        return 1;
    }

    uint8_t state = MJB_UTF8_ACCEPT;
    mjb_codepoint codepoint;

    const char *current = argv[0];
    size_t current_count = 0;
    char *counts = malloc(input_size);

    while(*current && (size_t)(current - argv[0]) < input_size) {
        state = mjb_utf8_decode_step(state, *current, &codepoint);

        if(state == MJB_UTF8_ACCEPT) {
            char double_space = 0;

            if(codepoint == 0x0A) {
                double_space = 'n';
            } else if(codepoint == 0x0D) {
                double_space = 'r';
            } else if(codepoint == 0x09) {
                double_space = 't';
            }

            if(double_space) {
                counts[current_count] = 2;
                printf("%s\\%c%s", color_green_start(), double_space, color_reset());
            } else {
                char buffer_utf8[5];
                mjb_codepoint_encode(codepoint, buffer_utf8, 5, MJB_ENCODING_UTF_8);
                printf("%s", buffer_utf8);
                counts[current_count] = 1;
            }

            ++current_count;
        }

        ++current;
    }

    puts("");

    if(output_size == 0) {
        return 0;
    }

    size_t breaks_index = 0;
    printf("%s", color_green_start());
    current = argv[0];
    state = MJB_UTF8_ACCEPT;

    for(size_t i = 0; i < input_real_size; ++i) {
        if(i == line_breaks[breaks_index].index) {
            printf("%c", '^');

            if(breaks_index == output_size - 1) {
                break;
            }

            ++breaks_index;
        } else {
            if(counts[i] == 2) {
                printf("  ");
            } else {
                printf(" ");
            }
        }
    }


    printf("%s", color_reset());
    puts("");
    fflush(stdout);

    free(line_breaks);
    free(counts);

    return 0;
}
