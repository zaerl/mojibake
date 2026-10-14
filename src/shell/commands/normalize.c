/**
 * The Mojibake shell
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "../shell.h"

static const char *mjbsh_quick_check_name(mjb_quick_check_result quick_check, bool is_json) {
    switch(quick_check) {
        case MJB_QC_YES:
            return is_json ? "yes" : "Yes";
        case MJB_QC_NO:
            return is_json ? "no" : "No";
        case MJB_QC_MAYBE:
            return is_json ? "maybe" : "Maybe";
        default:
            return is_json ? "unknown" : "Unknown";
    }
}

// Print the quick check result as an extra field, in verbose mode only.
static void mjbsh_print_quick_check(const char *buffer, size_t size, mjb_normalization form) {
    bool is_json = cmd_output_mode == OUTPUT_MODE_JSON;
    mjb_quick_check_result quick_check = MJB_QC_NO;
    mjb_text text = mjb_text_utf8(buffer, size);
    mjb_status status = mjb_normalization_quick_check(&text, form, &quick_check);

    if(is_json) {
        printf(",%s%s\"quick_check\":%s", mjbsh_jnl(), mjbsh_ji(), cmd_json_indent > 0 ? " " : "");

        if(status == MJB_STATUS_OK) {
            printf("\"%s\"", mjbsh_quick_check_name(quick_check, true));
        } else {
            printf("null");
        }

        return;
    }

    if(status == MJB_STATUS_OK) {
        mjbsh_value("Quick check", 0, "%s", mjbsh_quick_check_name(quick_check, false));
    } else {
        mjbsh_null("Quick check", 0);
    }
}

// Shared body of the nfd, nfkd, nfc, nfkc, and nfkc-casefold commands.
static int mjbsh_normalize_buffer(const char *buffer, size_t size, unsigned int flags) {
    bool is_json = cmd_output_mode == OUTPUT_MODE_JSON;
    bool is_casefold = flags == MJBSH_NFKC_CASEFOLD;
    bool as_string = cmd_interpret_mode == INTERPRET_MODE_CHARACTER;
    mjb_normalization form = (mjb_normalization)flags;
    mjb_result result;
    bool ret = true;

    mjb_text text = mjb_text_utf8(buffer, size);
    mjb_status status = is_casefold ?
        mjb_nfkc_casefold(&text, MJB_MALFORMED_STOP, MJB_ENC_UTF_8, &result, NULL) :
        mjb_normalize(&text, MJB_MALFORMED_STOP, form, MJB_ENC_UTF_8, &result, NULL);

    if(status != MJB_STATUS_OK) {
        return mjbsh_error("%s", mjb_status_message(status));
    }

    if(is_json) {
        printf("{%s%s\"output\":%s\"", mjbsh_jnl(), mjbsh_ji(), cmd_json_indent > 0 ? " " : "");

        for(size_t i = 0; result.output != NULL && i < result.output_size; ++i) {
            if(!mjbsh_print_escaped_character(result.output + i)) {
                putchar((unsigned char)result.output[i]);
            }
        }

        putchar('"');

        if(cmd_verbose > 0 && !is_casefold) {
            mjbsh_print_quick_check(buffer, size, form);
        }

        printf("%s}\n", mjbsh_jnl());

        goto cleanup;
    }

    if(as_string) {
        printf("%s", mjbsh_green());
    }

    text = mjb_text_utf8(result.output, result.output_size);
    if(result.output_size > 0 &&
        mjb_for_each_codepoint(&text, MJB_MALFORMED_STOP,
            as_string ? mjbsh_next_string_character : mjbsh_next_codepoint,
            NULL) != MJB_STATUS_OK) {
        ret = false;
    }

    if(as_string) {
        printf("%s", mjbsh_reset());
    }

    puts("");

    if(ret && cmd_verbose > 0 && !is_casefold) {
        mjbsh_print_quick_check(buffer, size, form);
    }

cleanup:
    (void)mjb_result_free(&result);

    return ret ? 0 : 1;
}

int mjbsh_normalize_command(int argc, char *const argv[], unsigned int flags) {
    if(cmd_interpret_mode == INTERPRET_MODE_CHARACTER) {
        return mjbsh_normalize_buffer(argv[0], strlen(argv[0]), flags);
    }

    unsigned int index = 0;
    // 5 bytes per codepoint is more than enough.
    char *codepoints = (char *)malloc((size_t)argc * 5);

    if(codepoints == NULL) {
        return mjbsh_error("Out of memory");
    }

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

    int ret = mjbsh_normalize_buffer(codepoints, index, flags);

    free(codepoints);

    return ret;
}
