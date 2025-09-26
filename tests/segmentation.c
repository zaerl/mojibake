/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <string.h>

#include "test.h"

void *test_segmentation(void *arg) {
    ATT_ASSERT(mjb_segmentation("", 0, MJB_ENCODING_UTF_8), true, "Empty string")

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
