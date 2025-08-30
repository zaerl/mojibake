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

static void flush_line(unsigned int column) {
    // Fill remaining space to reach cmd_width total content
    while(column <= cmd_width) {
        printf(" ");
        ++column;
    }

    if(column > cmd_width + 1) {
        puts("");
    } else {
        printf("│\n");
    }
}

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

    size_t breaks_index = 0;
    current = argv[0];
    state = MJB_UTF8_ACCEPT;

    if(output_size > 0) {
        printf("%s", color_green_start());

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
    }

    // Draw top border
    printf("┌");

    for(unsigned int i = 0; i < cmd_width; i++) {
        printf("─");
    }

    printf("┐\n");

    unsigned int column = 0;
    unsigned int current_i = 0;
    bool check_break = output_size > 0;
    breaks_index = 0;
    current = argv[0];
    state = MJB_UTF8_ACCEPT;
    codepoint = 0x0;

    while(*current && (size_t)(current - argv[0]) < input_size) {
        state = mjb_utf8_decode_step(state, *current, &codepoint);

        if(state == MJB_UTF8_ACCEPT) {
            bool can_break = false;
            // bool is_mandatory = false;

            if(check_break && line_breaks[breaks_index].index == current_i) {
                can_break = true;
                // is_mandatory = line_breaks[breaks_index].mandatory;

                if(breaks_index == output_size - 1) {
                    check_break = false;
                } else {
                    ++breaks_index;
                }
            }

            if(column == 0) {
                printf("│");
                column = 1;
            }

            if(can_break) {
                flush_line(column);
                printf("│");
                column = 1;
            }

            if(/*codepoint != 0x0A*/true) {
                unsigned int char_width = 1;
                bool is_overflow = column + char_width > cmd_width + 1;
                bool is_control_picture = false;

                if(codepoint <= 0x20) {
                    // Add 0x2400 to the codepoint to make it a printable character by using the
                    // "Control Pictures" block.
                    codepoint += 0x2400;
                    is_control_picture = true;
                } else if(codepoint == 0x7F) {
                    codepoint = 0x2421;
                    is_control_picture = true;
                }

                char buffer_utf8[5];
                mjb_codepoint_encode(codepoint, buffer_utf8, 5, MJB_ENCODING_UTF_8);

                if(is_overflow) {
                    printf("%s%s%s", color_red_start(), buffer_utf8, color_reset());
                } else {
                    if(is_control_picture) {
                        printf("%s%s%s", color_green_start(), buffer_utf8, color_reset());
                    } else {
                        printf("%s", buffer_utf8);
                    }
                }

                column += char_width;
            }

            ++current_i;
        }

        ++current;
    }

    if(column != 0) {
        flush_line(column);
    }

    // Draw bottom border
    printf("└");

    for(unsigned int i = 0; i < cmd_width; i++) {
        printf("─");
    }

    printf("┘\n");

    free(line_breaks);
    free(counts);

    return 0;
}
