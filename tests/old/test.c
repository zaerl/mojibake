/**
 * The mojibake library tests
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "test.h"

static const char *db_name = "../src/mojibake.db";
static unsigned int tests_run;
static unsigned int tests_valid;
static int current_section = -1;
static bool wait_on_error = false;
static struct section {
    const char *name;
    unsigned int tests_run;
    unsigned int tests_valid;
    clock_t begin;
    clock_t end;
} sections[SECTIONS_COUNT] = {
    { "Array", 0, 0, 0, 0 },
    { "Codepoint", 0, 0, 0, 0 },
    { "DB", 0, 0, 0, 0 },
    { "Encoding", 0, 0, 0, 0 },
    { "Memory", 0, 0, 0, 0 },
    { "Normalization", 0, 0, 0, 0 },
    { "Plane", 0, 0, 0, 0 },
    { "Version", 0, 0, 0, 0 }
};

MJB_EXPORT void mjb_assert(char *message, bool test) {
    ++tests_run;
    ++sections[current_section].tests_run;
    char enter = 0;

    if(test) {
        printf("Test: %s \x1B[32mOK\x1B[0m\n", message);
        ++tests_valid;
        ++sections[current_section].tests_valid;
    } else {
        printf("\x1B[31mTest: %s FAIL\x1B[0m\n", message);

        if(wait_on_error) {
            printf("Press any key to continue... ");

            while(enter != '\r' && enter != '\n' && enter != 27) {
                enter = getchar();

                if(enter == 27) {
                    exit(EXIT_SUCCESS);
                }
            }
        }
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

MJB_EXPORT unsigned int mjb_valid_count(void) {
    return tests_valid;
}

MJB_EXPORT unsigned int mjb_total_count(void) {
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
        character->decimal,
        character->digit,
        character->numeric,
        character->mirrored ? "true" : "false",
        character->uppercase,
        character->lowercase,
        character->titlecase);
}

int main(int argc, const char *argv[]) {
    printf("\x1b[36mMojibake %s test\x1B[0m\n\n", mjb_version());

    bool sections[SECTIONS_COUNT] = { true, true, true, true, true, true, true, true };
    bool codepoint = true;
    bool db = true;
    bool encoding = true;
    bool memory = true;
    bool normalization = true;
    bool plane = true;
    bool version = true;

    if(argc > 1 && argv[1][0] == '-') {
        size_t length = strlen(argv[1]);

        for(size_t i = 1; i < length; ++i) {
            switch(argv[1][i]) {
                case 'a': /* array */
                    sections[0] = false;
                    break;

                case 'c': /* codepoint */
                    sections[1] = false;
                    break;

                case 'd': /* DB */
                    sections[2] = false;
                    break;

                case 'e': /* Encoding */
                    sections[3] = false;
                    break;

                case 'm': /* Memory */
                    sections[4] = false;
                    break;

                case 'n': /* Normalization */
                    sections[5] = false;
                    break;

                case 'p': /* Plane */
                    sections[6] = false;
                    break;

                case 'v': /* Version */
                    sections[7] = false;
                    break;

                case 'W': /* Block on error and wait for input */
                    wait_on_error = true;
            }
        }
    }

    /* Codepoint */
    if(sections[1]) {
        mjb_select_section(1);
        mjb_run_test("Codepoint character", mjb_codepoint_character_test);
        mjb_run_test("Codepoint block", mjb_codepoint_block_test);
        mjb_run_test("Codepoint is", mjb_codepoint_is_test);
        mjb_run_test("Codepoint is graphic", mjb_codepoint_is_graphic_test);
        mjb_run_test("Codepoint is valid", mjb_codepoint_is_valid_test);
        mjb_run_test("Codepoint is LC/UC/TC", mjb_codepoint_lc_uc_tc_test);
    }

    /* DB */
    if(sections[2]) {
        mjb_select_section(2);
        mjb_run_test("Ready", mjb_ready_test);
    }

    /* Encoding */
    if(sections[3]) {
        mjb_select_section(3);
        mjb_run_test("String encoding", mjb_string_encoding_test);
        mjb_run_test("String is ASCII", mjb_string_is_ascii_test);
        mjb_run_test("String is UTF-8", mjb_string_is_utf8_test);
    }

    /* Memory */
    if(sections[4]) {
        mjb_select_section(4);
        mjb_run_test("Memory", mjb_memory_test);
    }

    /* Normalization */
    if(sections[5]) {
        mjb_select_section(5);
        mjb_run_test("Normalize NFD/NFC/NFKD/NFKC", mjb_codepoint_normalize_test);
    }

    /* Plane */
    if(sections[6]) {
        mjb_select_section(6);
        mjb_run_test("Codespace plane is valid", mjb_plane_is_valid_test);
        mjb_run_test("Codespace plane name", mjb_plane_name_test);
    }

    /* Version */
    if(sections[7]) {
        mjb_select_section(7);
        mjb_run_test("Get version", mjb_version_test);
        mjb_run_test("Get version number", mjb_version_number_test);
        mjb_run_test("Get unicode version", mjb_unicode_version_test);
    }

    mjb_select_section(-1);

    unsigned int i = 0;
    unsigned int valid;
    unsigned int total;
    clock_t delta = 0;

    for(; i < SECTIONS_COUNT; ++i) {
        if(!sections[i]) {
            continue;
        }

        delta = mjb_section_delta(i);
        valid = mjb_section_valid_count(i);
        total = mjb_section_total_count(i);

        /* Green if valid and red if not */
        const char *colorCode = valid == total ? "\x1B[32m" : "\x1B[31m";

        printf("%s [%ju ms] tests valid/run: %s%d/%d\n\x1B[0m", mjb_section_name(i), delta, colorCode, valid, total);
    }

    valid = mjb_valid_count();
    total = mjb_total_count();

    /* Green if valid and red if not */
    const char *colorCode = valid == total ? "\x1B[32m" : "\x1B[31m";

    printf("\nTests valid/run: %s%d/%d\n\x1B[0m", colorCode, valid, total);

    return total == valid ? 0 : -1;
}
