/**
 * The Mojibake shell
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <string.h>

#include "../../mojibake.h"
#include "../shell.h"
#include "commands.h"

static const char *mjbsh_locale_error_name(mjb_error error) {
    switch(error) {
        case MJB_ERROR_NONE: return "none";
        case MJB_ERROR_INVALID_ARGUMENT: return "invalid argument";
        case MJB_ERROR_UNSUPPORTED: return "unsupported";
        default: return "unknown";
    }
}

static void mjbsh_locale_field(const char *label, unsigned int nl, const char *value) {
    if(value[0] == '\0') {
        mjbsh_null(label, nl);

        return;
    }

    mjbsh_value(label, nl, "%s", value);
}

int mjbsh_locale_command(int argc, char * const argv[], unsigned int flags) {
    const char *input = argv[0];
    mjb_locale_id locale;
    mjb_error error = MJB_ERROR_NONE;

    if(mjb_locale_parse(input, strlen(input), MJB_ENC_UTF_8, &locale, &error) != MJB_STATUS_OK) {
        fprintf(stderr, "locale: could not parse BCP 47 language tag: %s\n",
            mjbsh_locale_error_name(error));

        return 1;
    }

    if(cmd_output_mode == OUTPUT_MODE_JSON) {
        printf("{%s", mjbsh_jnl());
    }

    mjbsh_value("Input", 1, "%s", input);
    mjbsh_locale_field("Language", 1, locale.language);
    mjbsh_locale_field("Extlang", 1, locale.extlang);
    mjbsh_locale_field("Script", 1, locale.script);
    mjbsh_locale_field("Region", 1, locale.region);
    mjbsh_locale_field("Variant", 1, locale.variant);
    mjbsh_locale_field("Extensions", 1, locale.extensions);
    mjbsh_locale_field("Private use", 1, locale.private_use);
    mjbsh_locale_field("Grandfathered", 0, locale.grandfathered);

    if(cmd_output_mode == OUTPUT_MODE_JSON) {
        printf("%s}", mjbsh_jnl());
    }

    return 0;
}
