/**
 * The Mojibake shell
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "../shell.h"

static mjb_filter_flags filter_flags = MJB_FILTER_NONE;

static int mjbsh_print_filter_analysis(const char *input) {
    mjb_result result;
    size_t input_size = strlen(input);

    mjb_status status = mjb_filter(input, input_size, MJB_ENC_UTF_8, filter_flags, MJB_ENC_UTF_8,
        &result);

    if(status != MJB_STATUS_OK) {
        fprintf(stderr, "Could not filter string\n");

        return 1;
    }

    if(cmd_output_mode == OUTPUT_MODE_JSON) {
        mjbsh_print_json_result(result.output, result.output_size);
    } else {
        puts(result.output);
    }

    if(result.output != NULL && result.output != input) {
        mjb_free(result.output);
    }

    return 0;
}

static void mjbsh_display_filter_output(const char *input) {
    mjbsh_clear_screen();
    printf("Filter the input\n");
    printf("Ctrl+C to exit\n");

    if(input == NULL || strlen(input) == 0) {
        fflush(stdout);

        return;
    }

    puts(input);
    (void)mjbsh_print_filter_analysis(input);
    fflush(stdout);
}

int mjbsh_filter_command(int argc, char *const argv[], unsigned int flags) {
    filter_flags = (mjb_filter_flags)flags;

    if(argc != 0) {
        return mjbsh_print_filter_analysis(argv[0]);
    }

    if(cmd_output_mode == OUTPUT_MODE_JSON) {
        fprintf(stderr, "filter: JSON output requires an input\n");

        return 1;
    }

    mjbsh_screen_mode(mjbsh_display_filter_output, NULL);

    return 0;
}
