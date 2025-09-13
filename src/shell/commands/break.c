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

static mjb_codepoint control_picture_codepoint(mjb_codepoint codepoint) {
    if(codepoint < 0x20) {
        // Add 0x2400 to the codepoint to make it a printable character by using the
        // "Control Pictures" block.
        codepoint += 0x2400;
    } else if(codepoint == 0x20) {
        codepoint = 0x2423;
    } else if(codepoint == 0x7F) {
        // The delete character.
        codepoint = 0x2421;
    }

    return codepoint;
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

    uint8_t state = MJB_UTF_ACCEPT;
    mjb_codepoint codepoint;

    const char *current = argv[0];

    while(*current && (size_t)(current - argv[0]) < input_size) {
        state = mjb_utf8_decode_step(state, *current, &codepoint);

        if(state == MJB_UTF_ACCEPT) {
            mjb_codepoint picture_codepoint = control_picture_codepoint(codepoint);
            bool is_control_picture = picture_codepoint != codepoint;
            char buffer_utf8[5];
            mjb_codepoint_encode(picture_codepoint, buffer_utf8, 5, MJB_ENCODING_UTF_8);

            if(is_control_picture) {
                printf("%s%s%s", color_green_start(), buffer_utf8, color_reset());
            } else {
                printf("%s", buffer_utf8);
            }
        }

        ++current;
    }

    puts("");

    size_t breaks_index = 0;
    bool check_break = true;
    current = argv[0];
    state = MJB_UTF_ACCEPT;

    if(output_size > 0) {
        for(size_t i = 0; i < input_real_size; ++i) {
            if(check_break && i == line_breaks[breaks_index].index) {
                printf("%s%s%s", color_green_start(), line_breaks[breaks_index].mandatory ? "!" :
                    "÷", color_reset());

                if(breaks_index == output_size - 1) {
                    check_break = false;
                    continue;
                }

                ++breaks_index;
            } else {
                printf("×");
            }
        }

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
    check_break = output_size > 0;
    breaks_index = 0;
    current = argv[0];
    state = MJB_UTF_ACCEPT;
    codepoint = 0x0;

    while(*current && (size_t)(current - argv[0]) < input_size) {
        state = mjb_utf8_decode_step(state, *current, &codepoint);

        if(state == MJB_UTF_ACCEPT) {
            bool can_break = false;
            bool is_mandatory = false;

            if(check_break && line_breaks[breaks_index].index == current_i) {
                can_break = true;
                is_mandatory = line_breaks[breaks_index].mandatory;

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

            if(can_break && is_mandatory) {
                flush_line(column);
                printf("│");
                column = 1;
            }

            unsigned int char_width = 1;
            bool is_overflow = column + char_width > cmd_width + 1;
            mjb_codepoint picture_codepoint = control_picture_codepoint(codepoint);
            bool is_control_picture = picture_codepoint != codepoint;

            char buffer_utf8[5];
            mjb_codepoint_encode(picture_codepoint, buffer_utf8, 5, MJB_ENCODING_UTF_8);

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

    return 0;
}
