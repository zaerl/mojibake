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
static int current_section = -1;
static struct section {
    const char *name;
    unsigned int tests_run;
    unsigned int tests_valid;
    clock_t begin;
    clock_t end;
} sections[6] = {
    { "Codepoint", 0, 0, 0, 0 },
    { "Db", 0, 0, 0, 0 },
    { "Encoding", 0, 0, 0, 0 },
    { "Normalization", 0, 0, 0, 0 },
    { "Plane", 0, 0, 0, 0 },
    { "Version", 0, 0, 0, 0 }
};

MJB_EXPORT void mjb_assert(char *message, bool test) {
    ++tests_run;
    ++sections[current_section].tests_run;

    if(test) {
        printf("Test: %s \x1B[32mOK\x1B[0m\n", message);
        ++tests_valid;
        ++sections[current_section].tests_valid;
    } else {
        printf("\x1B[31mTest: %s FAIL\x1B[0m\n", message);
    }
}

MJB_EXPORT void mjb_run_test(char *name, mjb_test test) {
    printf("\x1b[36m%s\x1B[0m\n", name);
    test();
    printf("\n");
}

MJB_EXPORT void mjb_select_section(int section) {
    if(current_section != -1) {
        sections[current_section].end = clock();
    }

    current_section = section;

    if(section != -1) {
        sections[section].begin = clock();
    }
}

MJB_EXPORT unsigned int mjb_valid_count() {
    return tests_valid;
}

MJB_EXPORT unsigned int mjb_total_count() {
    return tests_run;
}

MJB_EXPORT const char *mjb_section_name(unsigned int section) {
    return sections[section].name;
}

MJB_EXPORT unsigned int mjb_section_valid_count(unsigned int section) {
    return sections[section].tests_valid;
}

MJB_EXPORT unsigned int mjb_section_total_count(unsigned int section) {
    return sections[section].tests_run;
}

MJB_EXPORT clock_t mjb_section_delta(unsigned int section) {
    return sections[section].end - sections[section].begin;
}

MJB_EXPORT void mjb_print_character(mjb_character *character, mjb_codepoint codepoint) {
    if(!character) {
        return;
    }

    const char *format = "Character %u\n"
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
