/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <string.h>

#include "test.h"
#include "../src/mojibake-internal.h"

void *test_breaking(void *arg) {
    char line[16384]; // 16384, see line #19335
    char generated_input[1024]; // String to be used by mjb_break_line
    mjb_break_type expected_types[1024];
    char test_name[256];
    unsigned int current_line = 1;

    FILE *file = fopen("./utils/generate/UCD/auxiliary/LineBreakTest.txt", "r");

    if(file == NULL) {
        ATT_ASSERT("Not opened", "Opened file", "Valid breaking test file")

        return NULL;
    }

    while(fgets(line, 16384, file)) {
        if(line[0] == '#' || strnlen(line, 16384) <= 1) {
            ++current_line;

            continue;
        }

        char *token, *string, *tofree;
        tofree = string = strdup(line + 3);
        unsigned int types_i = 0;
        unsigned int i = 0;
        unsigned int generated_index = 0;
        unsigned int allowed_count = 0;
        memset(expected_types, MJB_LBP_NOT_SET, 1024);
        memset(generated_input, 0, 1024);
        bool skip_line = false;

        // × (U+00D7) = 0xC3 0x97
        // ÷ (U+00F7) = 0xC3 0xB7
        while((token = strsep(&string, " ")) != NULL) {
            if(token == NULL || token[0] == '\0') {
                i = 0;

                continue;
            }

            // Odd index means break type
            if(i++ % 2 != 0) {
                if((unsigned char)token[1] == 0xB7) { // ÷
                    expected_types[types_i++] = MJB_BT_ALLOWED;
                    ++allowed_count;

                    if((unsigned char)token[2] == 0x09) { // Tab # comment until next line
                        break;
                    }

                    continue;
                } else if((unsigned char)token[1] == 0x97) {
                    expected_types[types_i++] = MJB_BT_NO_BREAK;

                    continue;
                }
            }

            mjb_codepoint codepoint = strtoul((const char*)(token), NULL, 16);

#if !MJB_DANGEROUSLY_ALLOW_EMBEDDED_NULLS
            if(codepoint == 0) {
                free(tofree);
                ++current_line;
                skip_line = true;

                break;
            }
#endif
            unsigned int encoded_size = mjb_codepoint_encode(codepoint, generated_input +
                generated_index, 1024 - generated_index, MJB_ENCODING_UTF_8);

            generated_index += encoded_size;
        }

        if(skip_line) {
            continue;
        }

        generated_input[generated_index] = '\0';

        #if !MJB_DANGEROUSLY_ALLOW_EMBEDDED_NULLS
        size_t generated_length = mjb_strnlen(generated_input, 1024, MJB_ENCODING_UTF_8);
#else
        size_t generated_length = types_i;
#endif
        snprintf(test_name, 256, "#%u %u/%u line breakings", current_line, allowed_count, types_i);
        ATT_ASSERT(types_i, generated_length, test_name)

        mjb_break_type bt = MJB_BT_NOT_SET;
        mjb_next_state state;
        state.index = 0;
        size_t index = 0;

        while((bt = mjb_break_line(generated_input, generated_index, MJB_ENCODING_UTF_8, &state)) != MJB_BT_NOT_SET) {
            snprintf(test_name, 256, "Index %zu", index);

            if(bt == MJB_BT_MANDATORY) {
                bt = MJB_BT_ALLOWED;
            }

            ATT_ASSERT((uint8_t)bt, (uint8_t)expected_types[index++], test_name)
        }

        free(tofree);
        ++current_line;
    }

    fclose(file);

    return NULL;
}
