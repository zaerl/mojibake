/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <string.h>

#include "test.h"
#include "../src/mojibake-internal.h"

/**
 * Get codepoints from a string
 * Example: "0044 0307", gives 2 codepoints
 */
static size_t get_utf8_string(char *buffer, char *codepoints, size_t size, char *breakings) {
    char *token, *string, *tofree;
    tofree = string = strdup(buffer);
    unsigned int index = 0;
    unsigned int breakings_index = 0;
    unsigned int i = 0;

    while((token = strsep(&string, " ")) != NULL) {
        if(i % 2 == 0) {
            breakings[breakings_index] = token[0];
            ++breakings_index;
        } else {
            mjb_codepoint codepoint = strtoul((const char*)token, NULL, 16);
            index += mjb_codepoint_encode(codepoint, codepoints + index, size - index, MJB_ENCODING_UTF_8);
        }

        ++i;
    }

    codepoints[++index] = '\0';
    breakings[breakings_index] = '\0';
    free(tofree);

    return index;
}

void *test_breaking(void *arg) {
    mjb_line_breaking_class lbc;

    ATT_ASSERT(mjb_codepoint_line_breaking_class(MJB_CODEPOINT_MAX + 1, &lbc), false, "Invalid")

    // CURRENT_ASSERT mjb_codepoint_line_breaking_class
    ATT_ASSERT(mjb_codepoint_line_breaking_class(0x0, &lbc), true, "NULL")
    ATT_ASSERT(lbc, MJB_LBC_CM, "CM")

    char line[2048] = { 0 };
    char source[2048 * 2] = { 0 };
    char breakings[256] = { 0 };

    unsigned int current_line = 1;
    FILE *file = fopen("./utils/generate/UCD/auxiliary/LineBreakTestModified.txt", "r");

    if(file == NULL) {
        ATT_ASSERT("Not opened", "Opened file", "Valid special casing test file")

        return NULL;
    }

    while(fgets(line, 2048, file)) {
        size_t length = strnlen(line, 2048);

        if(length <= 1) {
            ++current_line;

            continue;
        }

        char *string = strdup(line);

        // Remove trailing newline from the entire line before tokenizing
        size_t line_len = strlen(string);
        if(line_len > 0 && string[line_len - 1] == '\n') {
            string[line_len - 1] = '\0';
        }

        size_t source_size = get_utf8_string(line, (char*)source, 2048 * 2, breakings);
        char test_name[128];
        snprintf(test_name, 128, "#%u breaking [%s]", current_line, breakings);

        // CURRENT_ASSERT mjb_line_breaking
        // CURRENT_COUNT 16672
        char *calculated_breakings = mjb_line_break(source, source_size, MJB_ENCODING_UTF_8);
        ATT_ASSERT(calculated_breakings, breakings, test_name)

        ++current_line;
        free(string);

        if(calculated_breakings != NULL) {
            free(calculated_breakings);
        }
    }

    return NULL;
}
