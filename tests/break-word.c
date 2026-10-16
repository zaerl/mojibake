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
    mjb_word_break_state state;
    state.index = 0;
    size_t index = 0;
    size_t successful_count = 0;
    size_t starts[1025];
    size_t start_count = mjb_test_codepoint_starts(buffer, byte_length, starts, 1025);
    size_t position_mismatches = 0;

    mjb_text text = mjb_text_utf8(buffer, byte_length);
    while((bt = mjb_word_break_next(&text, &state)) != MJB_BT_NOT_SET) {
        snprintf(test_name, 256, "#%u index %zu", current_line, index);

        if(bt == MJB_BT_MANDATORY) {
            bt = MJB_BT_ALLOWED;
        }

        // There is no event for the start of the text: event i is the boundary after codepoint
        // i, at the start of codepoint i + 1, and the last one is the end of the text.
        if(index < start_count && state.position != starts[index + 1]) {
            ++position_mismatches;
        }

        if((uint8_t)bt == (uint8_t)expected_types[index++]) {
            ++successful_count;
        } else {
            break;
        }
    }

    MJB_TEST_COVERAGE(mjb_word_break_next);
    ATT_ASSERT(index, successful_count, test_name);
    ATT_ASSERT(position_mismatches, (size_t)0, "Break positions are codepoint starts");
}

static void test_word_truncate(void) {
    mjb_word_break_state state;
    state.index = 0;

    mjb_text text = mjb_text_utf8(NULL, 1);
    ATT_ASSERT((uint8_t)mjb_word_break_next(&text, &state), (uint8_t)MJB_BT_NOT_SET,
        "Word break rejects NULL buffer");
    text = mjb_text_utf8("A", 1);
    ATT_ASSERT((uint8_t)mjb_word_break_next(&text, NULL), (uint8_t)MJB_BT_NOT_SET,
        "Word break rejects NULL state");

    // Edge cases
    text = mjb_text_utf8("", 0);
    ATT_ASSERT(mjb_word_prefix_length(&text, 3), (size_t)0, "Truncate word: empty string");
    text = mjb_text_utf8(NULL, 1);
    ATT_ASSERT(mjb_word_prefix_length(&text, 3), (size_t)0,
        "Truncate word: NULL string");
    text = mjb_text_utf8("Hello World", 11);
    ATT_ASSERT(mjb_word_prefix_length(&text, 0), (size_t)0,
        "Truncate word: 0 segments");

    // "Hello World": breaks at 5 (after Hello), 6 (after space), 11 (end)
    text = mjb_text_utf8("Hello World", 11);
    ATT_ASSERT(mjb_word_prefix_length(&text, 1), (size_t)5,
        "Truncate word: 1 segment");
    text = mjb_text_utf8("Hello World", 11);
    ATT_ASSERT(mjb_word_prefix_length(&text, 2), (size_t)6,
        "Truncate word: 2 segments");
    text = mjb_text_utf8("Hello World", 11);
    ATT_ASSERT(mjb_word_prefix_length(&text, 3), (size_t)11,
        "Truncate word: 3 segments (no-op)");
    text = mjb_text_utf8("Hello World", 11);
    ATT_ASSERT(mjb_word_prefix_length(&text, 5), (size_t)11,
        "Truncate word: 5 segments (no-op)");

    text = mjb_text_utf8("", 0);
    ATT_ASSERT(mjb_word_prefix_length_for_width(&text, MJB_TERMINAL_WIDTH_NARROW, 10),
        (size_t)0, "Truncate word width: empty string");
    text = mjb_text_utf8(NULL, 1);
    ATT_ASSERT(mjb_word_prefix_length_for_width(&text, MJB_TERMINAL_WIDTH_NARROW, 10),
        (size_t)0, "Truncate word width: NULL string");
    text = mjb_text_utf8("Hello World", 11);
    ATT_ASSERT(mjb_word_prefix_length_for_width(&text, MJB_TERMINAL_WIDTH_NARROW,
                   0),
        (size_t)0, "Truncate word width: 0 columns");

    // "Hello"=5 cols, " "=1 col would exceed 5
    text = mjb_text_utf8("Hello World", 11);
    ATT_ASSERT(mjb_word_prefix_length_for_width(&text, MJB_TERMINAL_WIDTH_NARROW,
                   5),
        (size_t)5, "Truncate word width: 5 columns");

    // "Hello " = 6 cols, "World" would exceed 6
    text = mjb_text_utf8("Hello World", 11);
    ATT_ASSERT(mjb_word_prefix_length_for_width(&text, MJB_TERMINAL_WIDTH_NARROW,
                   6),
        (size_t)6, "Truncate word width: 6 columns");
    text = mjb_text_utf8("Hello World", 11);
    ATT_ASSERT(mjb_word_prefix_length_for_width(&text, MJB_TERMINAL_WIDTH_NARROW,
                   11),
        (size_t)11, "Truncate word width: 11 columns (no-op)");
}

static void test_word_count(void) {
    size_t count = 6251;

    // Argument validation
    mjb_text text = mjb_text_utf8("A", 1);
    ATT_ASSERT_STATUS(mjb_word_count(&text, MJB_MALFORMED_STOP, NULL, NULL),
        MJB_STATUS_INVALID_ARGUMENT, "Count rejects NULL count");
    text = mjb_text_utf8(NULL, 1);
    ATT_ASSERT_STATUS(mjb_word_count(&text, MJB_MALFORMED_STOP, &count, NULL),
        MJB_STATUS_INVALID_ARGUMENT, "Count rejects NULL buffer");
    ATT_ASSERT(count, (size_t)0, "Count is zero after NULL buffer");

    count = 6251;
    text = mjb_text_make("A", 1, MJB_ENC_UNKNOWN);
    ATT_ASSERT_STATUS(mjb_word_count(&text, MJB_MALFORMED_STOP, &count, NULL),
        MJB_STATUS_INVALID_ENCODING, "Count rejects invalid encoding");
    ATT_ASSERT(count, (size_t)0, "Count is zero after invalid encoding");

    // Empty input is valid and counts zero words
    text = mjb_text_utf8("", 0);
    ATT_ASSERT_STATUS(mjb_word_count(&text, MJB_MALFORMED_STOP, &count, NULL), MJB_STATUS_OK,
        "Count: empty string status");
    ATT_ASSERT(count, (size_t)0, "Count: empty string");
    text = mjb_text_utf8(NULL, 0);
    ATT_ASSERT_STATUS(mjb_word_count(&text, MJB_MALFORMED_STOP, &count, NULL), MJB_STATUS_OK,
        "Count: NULL buffer with zero length status");

    mjb_diagnostic diagnostic;
    text = mjb_text_utf8("A\x80", 2);
    ATT_ASSERT_STATUS(mjb_word_count(&text, MJB_MALFORMED_STOP, &count,
                          &diagnostic),
        MJB_STATUS_MALFORMED_INPUT, "Count rejects malformed input");
    ATT_ASSERT(count, (size_t)0, "Count is zero after malformed input");
    ATT_ASSERT(diagnostic.byte_offset, (size_t)1, "Word count malformed offset");

    const char malformed_word[] = "A\x80"
                                  "B";
    text = mjb_text_utf8(malformed_word, sizeof(malformed_word) - 1);
    ATT_ASSERT_STATUS(mjb_word_count(&text, MJB_MALFORMED_REPLACE, &count, &diagnostic),
        MJB_STATUS_OK, "Word count replaces malformed input");
    ATT_ASSERT(count, (size_t)2, "Replacement separates word segments");
    ATT_ASSERT(diagnostic.byte_offset, (size_t)1, "Word replacement retains malformed offset");

    text = mjb_text_utf8(malformed_word, sizeof(malformed_word) - 1);
    ATT_ASSERT_STATUS(mjb_word_count(&text, MJB_MALFORMED_SKIP, &count, &diagnostic),
        MJB_STATUS_OK, "Word count skips malformed input");
    ATT_ASSERT(count, (size_t)1, "Skipping malformed input joins adjacent letters");

    // Punctuation, whitespace, and symbol segments are not counted
    text = mjb_text_utf8("Hello, world! It works.", 23);
    ATT_ASSERT_STATUS(mjb_word_count(&text, MJB_MALFORMED_STOP, &count, NULL),
        MJB_STATUS_OK, "Count: sentence status");
    ATT_ASSERT(count, (size_t)4, "Count: sentence has four words");
    text = mjb_text_utf8("...", 3);
    ATT_ASSERT_STATUS(mjb_word_count(&text, MJB_MALFORMED_STOP, &count, NULL), MJB_STATUS_OK,
        "Count: punctuation-only status");
    ATT_ASSERT(count, (size_t)0, "Count: punctuation-only");
    text = mjb_text_utf8("   ", 3);
    ATT_ASSERT_STATUS(mjb_word_count(&text, MJB_MALFORMED_STOP, &count, NULL), MJB_STATUS_OK,
        "Count: spaces-only status");
    ATT_ASSERT(count, (size_t)0, "Count: spaces-only");

    // WB6/WB7 keep letters together across certain punctuation
    text = mjb_text_utf8("don't stop", 10);
    ATT_ASSERT_STATUS(mjb_word_count(&text, MJB_MALFORMED_STOP, &count, NULL), MJB_STATUS_OK,
        "Count: apostrophe status");
    ATT_ASSERT(count, (size_t)2, "Count: apostrophe stays inside the word");
    text = mjb_text_utf8("e.g. example.com", 16);
    ATT_ASSERT_STATUS(mjb_word_count(&text, MJB_MALFORMED_STOP, &count, NULL),
        MJB_STATUS_OK, "Count: abbreviation status");
    ATT_ASSERT(count, (size_t)2, "Count: mid-letter punctuation stays inside the word");

    // Hyphenated compounds count each part (WB999 breaks at the hyphens)
    text = mjb_text_utf8("state-of-the-art", 16);
    ATT_ASSERT_STATUS(mjb_word_count(&text, MJB_MALFORMED_STOP, &count, NULL),
        MJB_STATUS_OK, "Count: hyphenated status");
    ATT_ASSERT(count, (size_t)4, "Count: hyphenated compound counts each part");

    // Numbers are word-like (WB11/WB12 keep formatted numbers together)
    text = mjb_text_utf8("3.14 is pi", 10);
    ATT_ASSERT_STATUS(mjb_word_count(&text, MJB_MALFORMED_STOP, &count, NULL), MJB_STATUS_OK,
        "Count: number status");
    ATT_ASSERT(count, (size_t)3, "Count: formatted number is one word");

    // ExtendNumLet joins words; a connector alone is not a word (WB13a, WB13b)
    text = mjb_text_utf8("foo_bar baz", 11);
    ATT_ASSERT_STATUS(mjb_word_count(&text, MJB_MALFORMED_STOP, &count, NULL), MJB_STATUS_OK,
        "Count: connector status");
    ATT_ASSERT(count, (size_t)2, "Count: connector joins one word");
    text = mjb_text_utf8("_ _", 3);
    ATT_ASSERT_STATUS(mjb_word_count(&text, MJB_MALFORMED_STOP, &count, NULL), MJB_STATUS_OK,
        "Count: connector-only status");
    ATT_ASSERT(count, (size_t)0, "Count: connector-only segments are not words");

    // Emoji are not word-like
    text = mjb_text_utf8("a \xF0\x9F\x91\x8D b", 8);
    ATT_ASSERT_STATUS(mjb_word_count(&text, MJB_MALFORMED_STOP, &count, NULL),
        MJB_STATUS_OK, "Count: emoji status");
    ATT_ASSERT(count, (size_t)2, "Count: emoji between words");

    // No dictionary segmentation: ideographs count one word per character
    text = mjb_text_utf8("\xE6\x9D\xB1\xE4\xBA\xAC", 6);
    ATT_ASSERT_STATUS(mjb_word_count(&text, MJB_MALFORMED_STOP, &count, NULL),
        MJB_STATUS_OK, "Count: ideograph status");
    ATT_ASSERT(count, (size_t)2, "Count: one word per ideograph");

    // Katakana runs stay together (WB13)
    text = mjb_text_utf8("\xE3\x82\xAB\xE3\x82\xBF\xE3\x82\xAB\xE3\x83\x8A", 12);
    ATT_ASSERT_STATUS(mjb_word_count(&text, MJB_MALFORMED_STOP, &count, NULL), MJB_STATUS_OK, "Count: katakana status");
    ATT_ASSERT(count, (size_t)1, "Count: katakana run is one word");

    // MJB_NUL_TERMINATED requests a terminator scan
    text = mjb_text_utf8("Hi there", MJB_NUL_TERMINATED);
    ATT_ASSERT_STATUS(mjb_word_count(&text, MJB_MALFORMED_STOP, &count, NULL),
        MJB_STATUS_OK, "Count: NUL-terminated status");
    ATT_ASSERT(count, (size_t)2, "Count: NUL-terminated");

    // UTF-16LE input
    const char utf16le_two[] = { 'H', '\0', 'i', '\0', ' ', '\0', 'y', '\0', 'o', '\0', 'u', '\0' };
    text = mjb_text_make(utf16le_two, 12, MJB_ENC_UTF_16LE);
    ATT_ASSERT_STATUS(mjb_word_count(&text, MJB_MALFORMED_STOP, &count, NULL), MJB_STATUS_OK,
        "Count: UTF-16LE status");
    ATT_ASSERT(count, (size_t)2, "Count: UTF-16LE two words");
}

ATT_TEST(break_word) {
    {
        mjb_word_break_state state;
        state.index = 0;
        size_t count = 9;
        ATT_ASSERT((int)mjb_word_break_next(NULL, &state), (int)MJB_BT_NOT_SET,
            "Word break rejects NULL text");
        ATT_ASSERT(mjb_word_prefix_length(NULL, 1), (size_t)0, "Truncate word rejects NULL text");
        ATT_ASSERT(mjb_word_prefix_length_for_width(NULL, MJB_TERMINAL_WIDTH_NARROW, 1), (size_t)0,
            "Truncate word width rejects NULL text");
        ATT_ASSERT_STATUS(mjb_word_count(NULL, MJB_MALFORMED_STOP, &count, NULL),
            MJB_STATUS_INVALID_ARGUMENT, "Word count rejects NULL text");
        ATT_ASSERT(count, (size_t)0, "Word count clears count after NULL text");
    }

    test_word_truncate();
    test_word_count();
    read_test_file("./unicode-data/UCD/auxiliary/WordBreakTest.txt", &break_word_callback);
}
