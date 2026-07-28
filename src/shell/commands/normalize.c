/**
 * The Mojibake shell
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "../shell.h"

int mjbsh_normalize_string_command(int argc, char *const argv[], unsigned int flags) {
    mjb_result result;
    bool ret = true;

    mjb_status status = mjb_normalize(argv[0], strlen(argv[0]), MJB_ENC_UTF_8,
        (mjb_normalization)flags, MJB_ENC_UTF_8, &result);

    if(status != MJB_STATUS_OK) {
        return mjbsh_error("%s", mjb_status_message(status));
    }

    if(cmd_output_mode == OUTPUT_MODE_JSON) {
        mjbsh_print_json_result(result.output, result.output_size);
        goto cleanup;
    }

    printf("%s", mjbsh_green());

    if(result.output_size > 0 &&
        mjb_for_each_character(result.output, result.output_size, MJB_ENC_UTF_8,
            mjbsh_next_string_character) != MJB_STATUS_OK) {
        printf("%s", mjbsh_reset());
        puts("");
        ret = false;

        goto cleanup;
    }

    printf("%s", mjbsh_reset());
    puts("");

cleanup:
    if(result.output != NULL && result.output != argv[0]) {
        mjb_free(result.output);
    }

    return ret ? 0 : 1;
}

int mjbsh_normalize_command(int argc, char *const argv[], unsigned int flags) {
    if(cmd_interpret_mode == INTERPRET_MODE_CHARACTER) {
        return mjbsh_normalize_string_command(argc, argv, flags);
    }

    unsigned int index = 0;
    // 5 bytes per codepoint is more than enough.
    char *codepoints = (char *)malloc(argc * 5);

    for(int i = 0; i < argc; ++i) {
        mjb_codepoint codepoint = 0;

        if(!mjbsh_parse_codepoint(argv[i], &codepoint)) {
            free(codepoints);

            return mjbsh_error("Invalid codepoint input: %s", argv[i]);
        }

        index += mjb_codepoint_encode(codepoint, codepoints + index, (argc * 5) - index,
            MJB_ENC_UTF_8);
    }

    codepoints[index] = '\0';

    mjb_result result;
    bool ret = true;
    mjb_status status = mjb_normalize(codepoints, index, MJB_ENC_UTF_8, (mjb_normalization)flags,
        MJB_ENC_UTF_8, &result);

    if(status != MJB_STATUS_OK) {
        free(codepoints);

        return mjbsh_error("%s", mjb_status_message(status));
    }

    if(cmd_output_mode == OUTPUT_MODE_JSON) {
        mjbsh_print_json_result(result.output, result.output_size);

        goto cleanup;
    }

    if(result.output_size > 0 &&
        mjb_for_each_character(result.output, result.output_size, MJB_ENC_UTF_8,
            mjbsh_next_character) != MJB_STATUS_OK) {
        puts("");
        ret = false;

        goto cleanup;
    }

    puts("");

cleanup:
    if(result.output != NULL && result.output != codepoints) {
        mjb_free(result.output);
    }

    free(codepoints);

    return ret ? 0 : 1;
}
