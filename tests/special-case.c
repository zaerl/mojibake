/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <string.h>

#include "test.h"
#include "../src/utf8.h"

static int check_case(char *source, size_t source_size, char *target, size_t target_size,
    mjb_case_type type, unsigned int current_line, const char *step) {
    mjb_encoding encoding = MJB_ENCODING_UTF_8;
    char test_name[128];

    snprintf(test_name, 128, "#%u %s", current_line, step);

    if(source_size == 0 || target_size == 0) {
        return 0;
    }

    char *result = mjb_case(source, source_size, type, encoding);

    // CURRENT_ASSERT mjb_case
    // CURRENT_COUNT 309
    ATT_ASSERT(result, target, test_name)

    if(result != NULL && result != source) {
        mjb_free(result);
    }

    return 0;
}

void *test_special_case(void *arg) {
    char line[1024];
    unsigned int current_line = 1;
    FILE *file = fopen("./utils/generate/UCD/SpecialCasing.txt", "r");

    // 256 characters is enough for any test.
    const char source[256] = { 0 };
    const char lower[256] = { 0 };
    const char title[256] = { 0 };
    const char upper[256] = { 0 };

    size_t source_size = 0;
    size_t lower_size = 0;
    size_t title_size = 0;
    size_t upper_size = 0;

    if(file == NULL) {
        ATT_ASSERT("Not opened", "Opened file", "Valid special casing test file")

        return NULL;
    }

    while(fgets(line, 1024, file)) {
        if(line[0] == '#' || line[0] == '@' || strnlen(line, 512) <= 1) {
            // printf("skipping line %u (%s)\n", current_line, line);
            ++current_line;

            // TODO: add support for conditional mappings.
            if(strncmp(line, "# Conditional Mappings", 21) == 0) {
                break;
            }

            continue;
        }

        char *token, *string, *tofree;
        tofree = string = strdup(line);
        unsigned int field = 0;

        while((token = strsep(&string, ";")) != NULL) {
            switch(field) {
                case 0: // Source
                    source_size = get_string_from_codepoints(token, (char*)source, 256);
                    break;

                case 1: // Lower
                    lower_size = get_string_from_codepoints(token, (char*)lower, 256);
                    break;

                case 2: // Title
                    title_size = get_string_from_codepoints(token, (char*)title, 256);
                    break;

                case 3: // Upper
                    upper_size = get_string_from_codepoints(token, (char*)upper, 256);
                    break;
            }

            // Skip trailing comments
            if(++field == 4) {
                break;
            }
        }

        free(tofree);

        check_case((char*)source, source_size, (char*)lower, lower_size, MJB_CASE_LOWER, current_line, "lower");
        check_case((char*)source, source_size, (char*)title, title_size, MJB_CASE_TITLE, current_line, "title");
        check_case((char*)source, source_size, (char*)upper, upper_size, MJB_CASE_UPPER, current_line, "upper");

        memset((void*)source, 0, 256);
        memset((void*)lower, 0, 256);
        memset((void*)title, 0, 256);
        memset((void*)upper, 0, 256);

        ++current_line;
    }

    fclose(file);

    return NULL;
}
