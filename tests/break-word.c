/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

 #include <stdio.h>
 #include <string.h>

 #include "test.h"
 #include "../src/mojibake-internal.h"

void break_word_callback(const char *buffer, size_t size, unsigned int current_line, mjb_break_type *expected_types) {
    char test_name[256];
    mjb_break_type bt = MJB_BT_NOT_SET;
    mjb_next_word_state state;
    state.index = 0;
    size_t index = 0;
    size_t successful_count = 0;

    while((bt = mjb_break_word(buffer, size, MJB_ENCODING_UTF_8, &state)) != MJB_BT_NOT_SET) {
        snprintf(test_name, 256, "#%u index %zu", current_line, index);

        if(bt == MJB_BT_MANDATORY) {
            bt = MJB_BT_ALLOWED;
        }

        if((uint8_t)bt == (uint8_t)expected_types[index++]) {
            ++successful_count;
        } else  {
            break;
        }
    }

    // CURRENT_ASSERT mjb_break_word
    // CURRENT_COUNT 1826
    ATT_ASSERT(index, successful_count, test_name)
}

 void *test_break_word(void *arg) {
    read_test_file("./utils/generate/UCD/auxiliary/WordBreakTest.txt", &break_word_callback);

    return NULL;
 }
