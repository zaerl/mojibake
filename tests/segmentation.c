/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <string.h>

#include "test.h"

void *test_segmentation(void *arg) {
    mjb_next_state state;
    mjb_break_type bt;
    state.break_index = 0;

    ATT_ASSERT(mjb_segmentation("", 0, MJB_ENCODING_UTF_8, &state), MJB_BT_NOT_SET, "Empty string")

    mjb_break_type expected_a[] = { MJB_BT_ALLOWED };
    while((bt = mjb_segmentation("A", 1, MJB_ENCODING_UTF_8, &state)) != MJB_BT_NOT_SET) {
        ATT_ASSERT(bt, expected_a[state.break_index - 1], "A test")
    }

    state.break_index = 0;
    mjb_break_type expected_b[] = { MJB_BT_ALLOWED, MJB_BT_ALLOWED };
    while((bt = mjb_segmentation("AB", 1, MJB_ENCODING_UTF_8, &state)) != MJB_BT_NOT_SET) {
        ATT_ASSERT(bt, expected_b[state.break_index - 1], "AB test")
    }

    /*char line[2048] = { 0 };
    char breakings[256] = { 0 };

    unsigned int current_line = 1;
    FILE *file = fopen("./utils/generate/UCD/auxiliary/GraphemeBreakTestModified.txt", "r");

    if(file == NULL) {
        ATT_ASSERT("Not opened", "Opened file", "Valid segmentation test file")

        return NULL;
    }

    while(fgets(line, 2048, file)) {
        size_t length = strnlen(line, 2048);

        if(length <= 1) {
            ++current_line;

            continue;
        }

        char *source = strdup(line);
        size_t line_len = strlen(source);

        if(line_len > 0 && source[line_len - 1] == '\n') {
            source[line_len - 1] = '\0';
        }

        ++current_line;
        free(source);
    }

    fclose(file);*/

    return NULL;
}
