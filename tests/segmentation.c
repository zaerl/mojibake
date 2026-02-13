/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <string.h>

#include "test.h"

static void test_basic_segmentation(void) {
    mjb_next_state state;
    mjb_break_type bt = MJB_BT_NOT_SET;
    state.index = 0;
    size_t index = 0;

    #define MJB_TEST_S \
        state.index = 0; \
        index = 0; \

    ATT_ASSERT(mjb_segmentation("", 0, MJB_ENCODING_UTF_8, &state), MJB_BT_NOT_SET, "Empty string")

    MJB_TEST_S
    mjb_break_type expected_a[] = { MJB_BT_ALLOWED };

    while((bt = mjb_segmentation("A", 1, MJB_ENCODING_UTF_8, &state)) != MJB_BT_NOT_SET) {
        ATT_ASSERT(bt, expected_a[index++], "A test")
    }

    MJB_TEST_S
    mjb_break_type expected_ab[] = { MJB_BT_ALLOWED, MJB_BT_ALLOWED };

    while((bt = mjb_segmentation("AB", 2, MJB_ENCODING_UTF_8, &state)) != MJB_BT_NOT_SET) {
        ATT_ASSERT(bt, expected_ab[index++], "AB test")
    }
    ATT_ASSERT(index, 2, "AB test break index")

    MJB_TEST_S
    mjb_break_type expected_abc[] = { MJB_BT_ALLOWED, MJB_BT_ALLOWED, MJB_BT_ALLOWED };
    while((bt = mjb_segmentation("ABC", 3, MJB_ENCODING_UTF_8, &state)) != MJB_BT_NOT_SET) {
        ATT_ASSERT(bt, expected_abc[index++], "AB test")
    }
    ATT_ASSERT(index, 3, "ABC test break index")

    MJB_TEST_S
    mjb_break_type expected_brnl[] = { MJB_BT_ALLOWED, MJB_BT_NO_BREAK, MJB_BT_ALLOWED, MJB_BT_ALLOWED };
    while((bt = mjb_segmentation("A\r\nB", 4, MJB_ENCODING_UTF_8, &state)) != MJB_BT_NOT_SET) {
        ATT_ASSERT(bt, expected_brnl[index++], "A\\r\\nB test")
    }
    ATT_ASSERT(index, 4, "A\\r\\nB test break index")

    MJB_TEST_S
    mjb_break_type expected_itit[] = { MJB_BT_NO_BREAK, MJB_BT_ALLOWED, MJB_BT_NO_BREAK, MJB_BT_ALLOWED };
    while((bt = mjb_segmentation("🇮🇹🇮🇹", 16, MJB_ENCODING_UTF_8, &state)) != MJB_BT_NOT_SET) {
        ATT_ASSERT(bt, expected_itit[index++], "ITIT test")
    }
    ATT_ASSERT(index, 4, "ITIT test break index")

    #undef MJB_TEST_S
}

void *test_segmentation(void *arg) {
    test_basic_segmentation();

    char line[2048]; // 16384, see line #19335
    char generated_input[1024]; // String to be used by mjb_break_line
    mjb_break_type expected_types[1024];
    char test_name[256];
    unsigned int current_line = 1;

    FILE *file = fopen("./utils/generate/UCD/auxiliary/GraphemeBreakTest.txt", "r");

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
        memset(expected_types, MJB_BT_NOT_SET, 1024);
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

        // CURRENT_ASSERT mjb_segmentation
        // CURRENT_COUNT 766
        generated_input[generated_index] = '\0';
        size_t generated_length = mjb_strnlen(generated_input, 1024, MJB_ENCODING_UTF_8);
        snprintf(test_name, 256, "#%u %u/%u grapheme breakings", current_line, allowed_count, types_i);
        ATT_ASSERT(types_i, generated_length, test_name)

        mjb_break_type bt = MJB_BT_NOT_SET;
        mjb_next_state state;
        state.index = 0;
        size_t index = 0;

        while((bt = mjb_segmentation(generated_input, generated_index, MJB_ENCODING_UTF_8, &state)) != MJB_BT_NOT_SET) {
            // snprintf(test_name, 256, "Index %zu", index);
            ATT_ASSERT(bt, expected_types[index++], test_name)
        }

        free(tofree);
        ++current_line;

        if(current_line == 766) {
            break;
        }
    }

    fclose(file);

    return NULL;
}
