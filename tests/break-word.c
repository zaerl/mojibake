/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <string.h>

#include "../src/mojibake-internal.h"
#include "test.h"

static void break_word_callback(const char *buffer, size_t byte_length, unsigned int current_line,
    mjb_break_type *expected_types) {
    char test_name[256];
    mjb_break_type bt = MJB_BT_NOT_SET;
    mjb_next_word_state state;
    state.index = 0;
    size_t index = 0;
    size_t successful_count = 0;

    while((bt = mjb_next_word_break(buffer, byte_length, MJB_ENC_UTF_8, &state)) !=
        MJB_BT_NOT_SET) {
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

    MJB_TEST_COVERAGE(mjb_next_word_break);
    ATT_ASSERT(index, successful_count, test_name)
}

static void test_truncate_word(void) {
    mjb_next_word_state state;
    state.index = 0;

    ATT_ASSERT((uint8_t)mjb_next_word_break(NULL, 1, MJB_ENC_UTF_8, &state), (uint8_t)MJB_BT_NOT_SET,
        "Word break rejects NULL buffer")
    ATT_ASSERT((uint8_t)mjb_next_word_break("A", 1, MJB_ENC_UTF_8, NULL), (uint8_t)MJB_BT_NOT_SET,
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

    ATT_ASSERT(mjb_truncate_word_width("", 0, MJB_ENC_UTF_8, MJB_TERMINAL_WIDTH_NARROW, 10),
        (size_t)0, "Truncate word width: empty string")
    ATT_ASSERT(mjb_truncate_word_width(NULL, 1, MJB_ENC_UTF_8, MJB_TERMINAL_WIDTH_NARROW, 10),
        (size_t)0, "Truncate word width: NULL string")
    ATT_ASSERT(mjb_truncate_word_width("Hello World", 11, MJB_ENC_UTF_8,
                   MJB_TERMINAL_WIDTH_NARROW,
                   0),
        (size_t)0, "Truncate word width: 0 columns")

    // "Hello"=5 cols, " "=1 col would exceed 5
    ATT_ASSERT(mjb_truncate_word_width("Hello World", 11, MJB_ENC_UTF_8,
                   MJB_TERMINAL_WIDTH_NARROW,
                   5),
        (size_t)5, "Truncate word width: 5 columns")

    // "Hello " = 6 cols, "World" would exceed 6
    ATT_ASSERT(mjb_truncate_word_width("Hello World", 11, MJB_ENC_UTF_8,
                   MJB_TERMINAL_WIDTH_NARROW,
                   6),
        (size_t)6, "Truncate word width: 6 columns")
    ATT_ASSERT(mjb_truncate_word_width("Hello World", 11, MJB_ENC_UTF_8,
                   MJB_TERMINAL_WIDTH_NARROW,
                   11),
        (size_t)11, "Truncate word width: 11 columns (no-op)")
}

static void test_word_count(void) {
    size_t count = 6251;

    // Argument validation
    ATT_ASSERT_STATUS(mjb_word_count("A", 1, MJB_ENC_UTF_8, NULL),
        MJB_STATUS_INVALID_ARGUMENT, "Count rejects NULL count")
    ATT_ASSERT_STATUS(mjb_word_count(NULL, 1, MJB_ENC_UTF_8, &count),
        MJB_STATUS_INVALID_ARGUMENT, "Count rejects NULL buffer")
    ATT_ASSERT(count, (size_t)0, "Count is zero after NULL buffer")

    count = 6251;
    ATT_ASSERT_STATUS(mjb_word_count("A", 1, MJB_ENC_UNKNOWN, &count),
        MJB_STATUS_INVALID_ENCODING, "Count rejects invalid encoding")
    ATT_ASSERT(count, (size_t)0, "Count is zero after invalid encoding")

    // Empty input is valid and counts zero words
    ATT_ASSERT_STATUS(mjb_word_count("", 0, MJB_ENC_UTF_8, &count), MJB_STATUS_OK,
        "Count: empty string status")
    ATT_ASSERT(count, (size_t)0, "Count: empty string")
    ATT_ASSERT_STATUS(mjb_word_count(NULL, 0, MJB_ENC_UTF_8, &count), MJB_STATUS_OK,
        "Count: NULL buffer with zero length status")

    // Punctuation, whitespace, and symbol segments are not counted
    ATT_ASSERT_STATUS(mjb_word_count("Hello, world! It works.", 23, MJB_ENC_UTF_8, &count),
        MJB_STATUS_OK, "Count: sentence status")
    ATT_ASSERT(count, (size_t)4, "Count: sentence has four words")
    ATT_ASSERT_STATUS(mjb_word_count("...", 3, MJB_ENC_UTF_8, &count), MJB_STATUS_OK,
        "Count: punctuation-only status")
    ATT_ASSERT(count, (size_t)0, "Count: punctuation-only")
    ATT_ASSERT_STATUS(mjb_word_count("   ", 3, MJB_ENC_UTF_8, &count), MJB_STATUS_OK,
        "Count: spaces-only status")
    ATT_ASSERT(count, (size_t)0, "Count: spaces-only")

    // WB6/WB7 keep letters together across certain punctuation
    ATT_ASSERT_STATUS(mjb_word_count("don't stop", 10, MJB_ENC_UTF_8, &count), MJB_STATUS_OK,
        "Count: apostrophe status")
    ATT_ASSERT(count, (size_t)2, "Count: apostrophe stays inside the word")
    ATT_ASSERT_STATUS(mjb_word_count("e.g. example.com", 16, MJB_ENC_UTF_8, &count),
        MJB_STATUS_OK, "Count: abbreviation status")
    ATT_ASSERT(count, (size_t)2, "Count: mid-letter punctuation stays inside the word")

    // Hyphenated compounds count each part (WB999 breaks at the hyphens)
    ATT_ASSERT_STATUS(mjb_word_count("state-of-the-art", 16, MJB_ENC_UTF_8, &count),
        MJB_STATUS_OK, "Count: hyphenated status")
    ATT_ASSERT(count, (size_t)4, "Count: hyphenated compound counts each part")

    // Numbers are word-like (WB11/WB12 keep formatted numbers together)
    ATT_ASSERT_STATUS(mjb_word_count("3.14 is pi", 10, MJB_ENC_UTF_8, &count), MJB_STATUS_OK,
        "Count: number status")
    ATT_ASSERT(count, (size_t)3, "Count: formatted number is one word")

    // ExtendNumLet joins words; a connector alone is not a word (WB13a, WB13b)
    ATT_ASSERT_STATUS(mjb_word_count("foo_bar baz", 11, MJB_ENC_UTF_8, &count), MJB_STATUS_OK,
        "Count: connector status")
    ATT_ASSERT(count, (size_t)2, "Count: connector joins one word")
    ATT_ASSERT_STATUS(mjb_word_count("_ _", 3, MJB_ENC_UTF_8, &count), MJB_STATUS_OK,
        "Count: connector-only status")
    ATT_ASSERT(count, (size_t)0, "Count: connector-only segments are not words")

    // Emoji are not word-like
    ATT_ASSERT_STATUS(mjb_word_count("a \xF0\x9F\x91\x8D b", 8, MJB_ENC_UTF_8, &count),
        MJB_STATUS_OK, "Count: emoji status")
    ATT_ASSERT(count, (size_t)2, "Count: emoji between words")

    // No dictionary segmentation: ideographs count one word per character
    ATT_ASSERT_STATUS(mjb_word_count("\xE6\x9D\xB1\xE4\xBA\xAC", 6, MJB_ENC_UTF_8, &count),
        MJB_STATUS_OK, "Count: ideograph status")
    ATT_ASSERT(count, (size_t)2, "Count: one word per ideograph")

    // Katakana runs stay together (WB13)
    ATT_ASSERT_STATUS(mjb_word_count("\xE3\x82\xAB\xE3\x82\xBF\xE3\x82\xAB\xE3\x83\x8A", 12,
        MJB_ENC_UTF_8, &count), MJB_STATUS_OK, "Count: katakana status")
    ATT_ASSERT(count, (size_t)1, "Count: katakana run is one word")

    // MJB_NUL_TERMINATED requests a terminator scan
    ATT_ASSERT_STATUS(mjb_word_count("Hi there", MJB_NUL_TERMINATED, MJB_ENC_UTF_8, &count),
        MJB_STATUS_OK, "Count: NUL-terminated status")
    ATT_ASSERT(count, (size_t)2, "Count: NUL-terminated")

    // UTF-16LE input
    const char utf16le_two[] = { 'H', '\0', 'i', '\0', ' ', '\0', 'y', '\0', 'o', '\0', 'u', '\0' };
    ATT_ASSERT_STATUS(mjb_word_count(utf16le_two, 12, MJB_ENC_UTF_16LE, &count), MJB_STATUS_OK,
        "Count: UTF-16LE status")
    ATT_ASSERT(count, (size_t)2, "Count: UTF-16LE two words")
}

int test_break_word(void *arg) {
    test_truncate_word();
    test_word_count();
    read_test_file("./utils/generate/unicode-data/UCD/auxiliary/WordBreakTest.txt",
        &break_word_callback);

    return 0;
}
