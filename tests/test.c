/**
 * The mojibake library tests
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <string.h>

#include "test.h"

static unsigned int tests_run;
static unsigned int tests_valid;

MJB_EXPORT void mjb_assert(char *message, bool test) {
    ++tests_run;

    if(test) {
        printf("Test: %s \x1B[32mOK\x1B[0m\n", message);
        ++tests_valid;
    } else {
        printf("\x1B[31mTest: %s FAIL\x1B[0m\n", message);
    }
}

MJB_EXPORT void mjb_print_character(mjb_character* character, mjb_codepoint codepoint) {
    if(!character) {
        return;
    }

    const char* format = "Character %u\n"
        "codepoint: %u\n"
        "name: '%s'\n"
        "block: %u\n"
        "category: %u\n"
        "combining: %u\n"
        "bidirectional: %u\n"
        "decomposition: %u\n"
        "decimal: '%s'\n"
        "digit: '%s'\n"
        "numeric: '%s'\n"
        "mirrored: %s\n"
        "uppercase: %u\n"
        "lowercase: %u\n"
        "titlecase: %u\n";

    printf(format,
        codepoint,
        character->codepoint,
        character->name,
        character->block,
        character->category,
        character->combining,
        character->bidirectional,
        character->decomposition,
        character->decimal,
        character->digit,
        character->numeric,
        character->mirrored ? "true" : "false",
        character->uppercase,
        character->lowercase,
        character->titlecase);
}

MJB_EXPORT void mjb_run_test(char *name, mjb_test test) {
    printf("\x1b[36m%s\x1B[0m\n", name);
    test();
    printf("\n");
}

MJB_EXPORT unsigned int mjb_valid_count() {
    return tests_valid;
}

MJB_EXPORT unsigned int mjb_total_count() {
    return tests_run;
}
