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
#include "../screen.h"
#include "../shell.h"
#include "commands.h"

/*static void flush_line(unsigned int column) {
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
}*/

static void print_break_analysis(const char* input) {
    bool first = true;
    size_t input_size = strlen(input);
    // size_t input_real_size = mjb_strnlen(input, input_size, MJB_ENCODING_UTF_8);
    mjb_next_state state;
    mjb_break_type bt;
    state.index = 0;

    while((bt = mjb_break_line(input, input_size, MJB_ENCODING_UTF_8, &state)) != MJB_BT_NOT_SET) {
        bool is_eot = (state.index > input_size);

        if(first) {
            print_break_symbol(MJB_BT_NO_BREAK);

            // First iteration: print the starting codepoint
            mjbsh_print_codepoint(state.previous_codepoint != MJB_CODEPOINT_NOT_VALID
                ? state.previous_codepoint : state.current_codepoint);

            print_break_symbol(bt);

            // If previous was valid, print current; if not, we already printed current
            if(state.previous_codepoint != MJB_CODEPOINT_NOT_VALID && !is_eot) {
                mjbsh_print_codepoint(state.current_codepoint);
            }

            first = false;
        } else if(!is_eot) {
            print_break_symbol(bt);
            mjbsh_print_codepoint(state.current_codepoint);
        } else {
            print_break_symbol(bt);
        }
    }

/*
    puts("");

    size_t breaks_index = 0;
    bool check_break = true;
    state = MJB_UTF_ACCEPT;

    if(output_size > 0) {
        for(size_t i = 0; i < input_real_size; ++i) {
            if(check_break && i == line_breaks[breaks_index].index) {
                printf("%s%s%s", mjbsh_green(), line_breaks[breaks_index].mandatory ? "!" :
                    "÷", mjbsh_reset());

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

    mjbsh_table_top();

    unsigned int column = 0;
    unsigned int current_i = 0;
    check_break = output_size > 0;
    breaks_index = 0;
    state = MJB_UTF_ACCEPT;
    codepoint = 0x0;

    for(size_t i = 0; i < input_size; ++i) {
        state = mjb_utf8_decode_step(state, input[i], &codepoint);

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
            mjb_codepoint picture_codepoint = mjbsh_control_picture_codepoint(codepoint);
            bool is_control_picture = picture_codepoint != codepoint;

            char buffer_utf8[5];
            mjb_codepoint_encode(picture_codepoint, buffer_utf8, 5, MJB_ENCODING_UTF_8);

            if(is_overflow) {
                printf("%s%s%s", mjbsh_red(), buffer_utf8, mjbsh_reset());
            } else {
                if(is_control_picture) {
                    printf("%s%s%s", mjbsh_green(), buffer_utf8, mjbsh_reset());
                } else {
                    printf("%s", buffer_utf8);
                }
            }

            column += char_width;
            ++current_i;
        }
    }

    if(column != 0) {
        flush_line(column);
    }

    mjbsh_table_bottom();
    free(line_breaks);
*/
}

static void display_break_output(const char* input) {
    mjbsh_clear_screen();
    printf("Break the input into line breaks\n");
    printf("Ctrl+C or ESC to exit\n");

    if(input == NULL || strlen(input) == 0) {
        fflush(stdout);

        return;
    }

    puts(input);
    print_break_analysis(input);
    fflush(stdout);
}

int mjbsh_break_command(int argc, char * const argv[], unsigned int flags) {
    if(argc != 0) {
        print_break_analysis(argv[0]);

        return 0;
    }

    mjbsh_screen_mode(display_break_output);

    return 0;
}
