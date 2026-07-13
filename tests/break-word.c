/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

 #include <stdio.h>
 #include <string.h>

 #include "../src/mojibake-internal.h"
 #include "test.h"

void break_word_callback(const char *buffer, size_t byte_length, unsigned int current_line,
    mjb_break_type *expected_types) {
    char test_name[256];
    mjb_break_type bt = MJB_BT_NOT_SET;
    mjb_next_word_state state;
    state.index = 0;
    size_t index = 0;
    size_t successful_count = 0;

    while((bt = mjb_break_word(buffer, byte_length, MJB_ENC_UTF_8, &state)) != MJB_BT_NOT_SET) {
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

    MJB_TEST_COVERAGE(mjb_break_word);
    ATT_ASSERT(index, successful_count, test_name)
}

static void test_truncate_word(void) {
    mjb_next_word_state state;
    state.index = 0;

    ATT_ASSERT((uint8_t)mjb_break_word(NULL, 1, MJB_ENC_UTF_8, &state), (uint8_t)MJB_BT_NOT_SET,
        "Word break rejects NULL buffer")
    ATT_ASSERT((uint8_t)mjb_break_word("A", 1, MJB_ENC_UTF_8, NULL), (uint8_t)MJB_BT_NOT_SET,
        "Word break rejects NULL state")

    // Edge cases
    ATT_ASSERT(mjb_truncate_word("", 0, MJB_ENC_UTF_8, 3), (size_t)0, "Truncate word: empty string")
    ATT_ASSERT(mjb_truncate_word(NULL, 1, MJB_ENC_UTF_8, 3), (size_t)0,
        "Truncate word: NULL string")
    ATT_ASSERT(mjb_truncate_word("Hello World", 11, MJB_ENC_UTF_8, 0), (size_t)0,
        "Truncate word: 0 segments")

    // "Hello World": breaks at 5 (after Hello), 6 (after space), 11 (end)
    ATT_ASSERT(mjb_truncate_word("Hello World", 11, MJB_ENC_UTF_8, 1), (size_t)5,
        "Truncate word: 1 segment")
    ATT_ASSERT(mjb_truncate_word("Hello World", 11, MJB_ENC_UTF_8, 2), (size_t)6,
        "Truncate word: 2 segments")
    ATT_ASSERT(mjb_truncate_word("Hello World", 11, MJB_ENC_UTF_8, 3), (size_t)11,
        "Truncate word: 3 segments (no-op)")
    ATT_ASSERT(mjb_truncate_word("Hello World", 11, MJB_ENC_UTF_8, 5), (size_t)11,
        "Truncate word: 5 segments (no-op)")

    ATT_ASSERT(mjb_truncate_word_width("", 0, MJB_ENC_UTF_8, MJB_WIDTH_CONTEXT_WESTERN, 10),
        (size_t)0, "Truncate word width: empty string")
    ATT_ASSERT(mjb_truncate_word_width(NULL, 1, MJB_ENC_UTF_8, MJB_WIDTH_CONTEXT_WESTERN, 10),
        (size_t)0, "Truncate word width: NULL string")
    ATT_ASSERT(mjb_truncate_word_width("Hello World", 11, MJB_ENC_UTF_8, MJB_WIDTH_CONTEXT_WESTERN,
                   0),
        (size_t)0, "Truncate word width: 0 columns")

    // "Hello"=5 cols, " "=1 col would exceed 5
    ATT_ASSERT(mjb_truncate_word_width("Hello World", 11, MJB_ENC_UTF_8, MJB_WIDTH_CONTEXT_WESTERN,
                   5),
        (size_t)5, "Truncate word width: 5 columns")

    // "Hello " = 6 cols, "World" would exceed 6
    ATT_ASSERT(mjb_truncate_word_width("Hello World", 11, MJB_ENC_UTF_8, MJB_WIDTH_CONTEXT_WESTERN,
                   6),
        (size_t)6, "Truncate word width: 6 columns")
    ATT_ASSERT(mjb_truncate_word_width("Hello World", 11, MJB_ENC_UTF_8, MJB_WIDTH_CONTEXT_WESTERN,
                   11),
        (size_t)11, "Truncate word width: 11 columns (no-op)")

#if !defined(MJB_DANGEROUSLY_ALLOW_EMBEDDED_NULLS) || !MJB_DANGEROUSLY_ALLOW_EMBEDDED_NULLS
    const char utf16le_with_null[] = { 'A', '\0', '\0', '\0', 'B', '\0' };

    ATT_ASSERT(mjb_truncate_word(utf16le_with_null, sizeof(utf16le_with_null), MJB_ENC_UTF_16LE, 5),
        (size_t)2, "Truncate word: UTF-16LE stops at NULL")
    ATT_ASSERT(mjb_truncate_word_width(utf16le_with_null, sizeof(utf16le_with_null),
                   MJB_ENC_UTF_16LE, MJB_WIDTH_CONTEXT_WESTERN, 10),
        (size_t)2, "Truncate word width: UTF-16LE stops at NULL")
#endif
}

int test_break_word(void *arg) {
    test_truncate_word();
    read_test_file("./utils/generate/unicode-data/UCD/auxiliary/WordBreakTest.txt",
        &break_word_callback);

    return 0;
}
