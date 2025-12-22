/**
 * The Mojibake shell
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <string.h>

#include "../../mojibake.h"
#include "../screen.h"

static mjb_filter filter_flags = MJB_FILTER_NONE;

static void print_filter_analysis(const char* input) {
    mjb_result result;
    size_t input_size = strlen(input);

    bool ret = mjb_string_filter(input, input_size, MJB_ENCODING_UTF_8, MJB_ENCODING_UTF_8,
        (mjb_filter)filter_flags, &result);

    if(!ret) {
        puts("Could not filter string");
        fflush(stdout);
        return;
    }

    puts(result.output);

    if(result.output != NULL && result.output != input) {
        mjb_free(result.output);
    }
}

void display_filter_output(const char* input) {
    clear_screen();
    printf("Filter the input\n");
    printf("Ctrl+C or ESC to exit\n");

    if(input == NULL || strlen(input) == 0) {
        fflush(stdout);

        return;
    }

    puts(input);
    print_filter_analysis(input);
    fflush(stdout);
}

int filter_command(int argc, char * const argv[], unsigned int flags) {
    filter_flags = flags;

    if(argc != 0) {
        print_filter_analysis(argv[0]);

        return 0;
    }

    screen_mode(display_filter_output);

    return 0;
}
