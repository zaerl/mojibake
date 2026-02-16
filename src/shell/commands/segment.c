/**
 * The Mojibake shell
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../mojibake.h"
#include "../screen.h"
#include "../shell.h"
#include "commands.h"

static void print_break_analysis(const char* input) {
    bool first = true;
    size_t input_size = strlen(input);
    mjb_next_state state;
    mjb_break_type bt;
    state.index = 0;

    while((bt = mjb_segmentation(input, input_size, MJB_ENCODING_UTF_8, &state)) != MJB_BT_NOT_SET) {
        // EOT detection: after the eot call (line 74 in segmentation.c), index > input_size
        bool is_eot = (state.index > input_size);

        if(first) {
            print_break_symbol(MJB_BT_ALLOWED);

            // First iteration: print the starting codepoint
            print_codepoint(state.previous_codepoint != MJB_CODEPOINT_NOT_VALID
                ? state.previous_codepoint : state.current_codepoint);

            print_break_symbol(bt);

            // If previous was valid, print current; if not, we already printed current
            if(state.previous_codepoint != MJB_CODEPOINT_NOT_VALID && !is_eot) {
                print_codepoint(state.current_codepoint);
            }

            first = false;
        } else if(!is_eot) {
            print_break_symbol(bt);
            print_codepoint(state.current_codepoint);
        } else {
            print_break_symbol(bt);
        }
    }

    printf("\n");
}

static void display_break_output(const char* input) {
    mjbsh_clear_screen();
    printf("Unicode grapheme cluster segmentation\n");
    printf("Ctrl+C or ESC to exit\n");

    if(input == NULL || strlen(input) == 0) {
        fflush(stdout);

        return;
    }

    puts(input);
    print_break_analysis(input);
    fflush(stdout);
}

int mjbsh_segment_command(int argc, char * const argv[], unsigned int flags) {
    if(argc != 0) {
        print_break_analysis(argv[0]);

        return 0;
    }

    mjbsh_screen_mode(display_break_output);

    return 0;
}
