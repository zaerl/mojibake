/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <string.h>

#include "../src/mojibake-internal.h"
#include "test.h"

void break_sentence_callback(const char *buffer, size_t byte_length, unsigned int current_line,
    mjb_break_type *expected_types) {
    char test_name[256];
    mjb_break_type bt = MJB_BT_NOT_SET;
    mjb_next_sentence_state state;
    state.index = 0;
    size_t index = 0;
    size_t successful_count = 0;

    while((bt = mjb_break_sentence(buffer, byte_length, MJB_ENC_UTF_8, &state)) != MJB_BT_NOT_SET) {
        snprintf(test_name, 256, "#%u index %zu", current_line, index);

        if(bt == MJB_BT_MANDATORY) {
            bt = MJB_BT_ALLOWED;
        }

        if((uint8_t)bt == (uint8_t)expected_types[index++]) {
            ++successful_count;
        } else {
            break;
        }
    }

    MJB_TEST_COVERAGE(mjb_break_sentence);
    ATT_ASSERT(index, successful_count, test_name)
}

int test_break_sentence(void *arg) {
    mjb_next_sentence_state state;
    state.index = 0;

    ATT_ASSERT((uint8_t)mjb_break_sentence(NULL, 1, MJB_ENC_UTF_8, &state), (uint8_t)MJB_BT_NOT_SET,
        "Sentence break rejects NULL buffer")
    ATT_ASSERT((uint8_t)mjb_break_sentence("A", 1, MJB_ENC_UTF_8, NULL), (uint8_t)MJB_BT_NOT_SET,
        "Sentence break rejects NULL state")

#if !defined(MJB_DANGEROUSLY_ALLOW_EMBEDDED_NULLS) || !MJB_DANGEROUSLY_ALLOW_EMBEDDED_NULLS
    const char utf16le_null[] = { '\0', '\0', 'A', '\0' };

    state.index = 0;
    ATT_ASSERT((uint8_t)mjb_break_sentence(utf16le_null, sizeof(utf16le_null), MJB_ENC_UTF_16LE,
                   &state),
        (uint8_t)MJB_BT_ALLOWED, "Sentence break stops at UTF-16LE NULL")
    ATT_ASSERT((uint8_t)mjb_break_sentence(utf16le_null, sizeof(utf16le_null), MJB_ENC_UTF_16LE,
                   &state),
        (uint8_t)MJB_BT_NOT_SET, "Sentence break finishes after UTF-16LE NULL")
#endif

    read_test_file("./utils/generate/unicode-data/UCD/auxiliary/SentenceBreakTest.txt",
        &break_sentence_callback);

    return 0;
}
