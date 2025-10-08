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
/*static size_t get_string_from_codepoints(char *buffer, char *codepoints, size_t size, char *breakings) {
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
}*/

void *test_breaking(void *arg) {
    mjb_line_breaking_class lbc;
    mjb_category category;

    ATT_ASSERT(mjb_codepoint_line_breaking_class(MJB_CODEPOINT_MAX + 1, &lbc, &category), false, "Invalid")

    // CURRENT_ASSERT mjb_codepoint_line_breaking_class
    ATT_ASSERT(mjb_codepoint_line_breaking_class(0x0, &lbc, &category), true, "NULL")
    ATT_ASSERT((unsigned int)lbc, (unsigned int)MJB_LBC_CM, "CM")

    char line[16384]; // 16384, see line #19335
    char generated_input[1024]; // String to be used by mjb_break_line
    char expected_string[1024]; // String that the test should generate
    char generated_string[1024]; // String that the test generated
    char test_name[256];
    unsigned int current_line = 1;

    FILE *file = fopen("./utils/generate/UCD/auxiliary/LineBreakTest.txt", "r");

    if(file == NULL) {
        ATT_ASSERT("Not opened", "Opened file", "Valid breaking test file")

        return NULL;
    }

    puts("Start breaking test");

    while(fgets(line, 16384, file)) {
        if(line[0] == '#' || strnlen(line, 16384) <= 1) {
            ++current_line;

            continue;
        }

        char *token, *string, *tofree;
        tofree = string = strdup(line);
        unsigned int field = 0;
        unsigned int generated_index = 0;
        unsigned int allowed_count = 0;

        while((token = strsep(&string, "\xC3")) != NULL) {
            if(token == NULL || token[0] == '\0') {
                continue;
            }

            char current_break = ' ';

            if((unsigned char)token[0] == 0xB7) { // ÷
                if((unsigned char)token[1] == 0x09) { // Tab # comment until next line
                    break;
                }

                current_break = '+';
                ++allowed_count;
            } else {
                current_break = 'x';
            }

            mjb_codepoint codepoint = strtoul((const char*)(token + 2), NULL, 16);
            expected_string[field] = current_break;

            unsigned int encoded_size = mjb_codepoint_encode(codepoint, generated_input +
                generated_index, 1024 - generated_index, MJB_ENCODING_UTF_8);

            generated_index += encoded_size;
            ++field;
        }

        expected_string[field] = '+';
        expected_string[field + 1] = '\0';
        generated_input[generated_index] = '\0';
        ++allowed_count;

        size_t output_size = 0;
        mjb_line_break *breakings = mjb_break_line(generated_input, generated_index,
            MJB_ENCODING_UTF_8, &output_size);

        snprintf(test_name, 256, "#%u generate %zu/%u breakings", current_line, output_size,
            allowed_count);
        ATT_ASSERT(output_size, allowed_count, test_name)

        size_t breaks_index = 0;
        memset(generated_string, '\0', 1024);

        if(output_size > 0) {
            // First element is always x
            generated_string[0] = 'x';

            for(size_t i = 0; i < field; ++i) {
                if(i == breakings[breaks_index].index) {
                    generated_string[i + 1] = '+';
                    ++breaks_index;
                } else {
                    generated_string[i + 1] = 'x';
                }
            }

            free(breakings);
        }

        snprintf(test_name, 256, "#%u generate breakings", current_line);
        ATT_ASSERT(generated_string, expected_string, test_name)

        free(tofree);
        ++current_line;
    }

    fclose(file);

    return NULL;
}
