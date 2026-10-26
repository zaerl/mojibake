/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <string.h>

#include "../src/mojibake-internal.h"
#include "test.h"

static void break_sentence_callback(const char *buffer, size_t byte_length,
    unsigned int current_line, mjb_break_type *expected_types) {
    char test_name[256];
    mjb_break_type bt = MJB_BT_NOT_SET;
    mjb_sentence_break_state state;
    state.index = 0;
    size_t index = 0;
    size_t successful_count = 0;
    size_t starts[1025];
    size_t start_count = mjb_test_codepoint_starts(buffer, byte_length, starts, 1025);
    size_t position_mismatches = 0;

    mjb_text text = mjb_text_utf8(buffer, byte_length);
    while((bt = mjb_sentence_break_next(&text, &state)) != MJB_BT_NOT_SET) {
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

    MJB_TEST_COVERAGE(mjb_sentence_break_next);
    ATT_ASSERT(index, successful_count, test_name);
    ATT_ASSERT(position_mismatches, (size_t)0, "Break positions are codepoint starts");
}

static void test_sentence_count(void) {
    size_t count = 6251;

    // Argument validation
    mjb_text text = mjb_text_utf8("A", 1);
    ATT_ASSERT_STATUS(mjb_sentence_count(&text, MJB_MALFORMED_STOP, NULL, NULL),
        MJB_STATUS_INVALID_ARGUMENT, "Count rejects NULL count");
    text = mjb_text_utf8(NULL, 1);
    ATT_ASSERT_STATUS(mjb_sentence_count(&text, MJB_MALFORMED_STOP, &count, NULL),
        MJB_STATUS_INVALID_ARGUMENT, "Count rejects NULL buffer");
    ATT_ASSERT(count, (size_t)0, "Count is zero after NULL buffer");

    count = 6251;
    text = mjb_text_make("A", 1, MJB_ENC_UNKNOWN);
    ATT_ASSERT_STATUS(mjb_sentence_count(&text, MJB_MALFORMED_STOP, &count, NULL),
        MJB_STATUS_INVALID_ENCODING, "Count rejects invalid encoding");
    ATT_ASSERT(count, (size_t)0, "Count is zero after invalid encoding");

    // Empty input is valid and counts zero segments
    text = mjb_text_utf8("", 0);
    ATT_ASSERT_STATUS(mjb_sentence_count(&text, MJB_MALFORMED_STOP, &count, NULL), MJB_STATUS_OK,
        "Count: empty string status");
    ATT_ASSERT(count, (size_t)0, "Count: empty string");
    text = mjb_text_utf8(NULL, 0);
    ATT_ASSERT_STATUS(mjb_sentence_count(&text, MJB_MALFORMED_STOP, &count, NULL), MJB_STATUS_OK,
        "Count: NULL buffer with zero length status");

    mjb_diagnostic diagnostic;
    text = mjb_text_utf8("A\x80", 2);
    ATT_ASSERT_STATUS(mjb_sentence_count(&text, MJB_MALFORMED_STOP,
                          &count, &diagnostic),
        MJB_STATUS_MALFORMED_INPUT, "Count rejects malformed input");
    ATT_ASSERT(count, (size_t)0, "Count is zero after malformed input");
    ATT_ASSERT(diagnostic.byte_offset, (size_t)1, "Sentence count malformed offset");

    text = mjb_text_utf8("A\x80", 2);
    ATT_ASSERT_STATUS(mjb_sentence_count(&text, MJB_MALFORMED_REPLACE, &count, &diagnostic),
        MJB_STATUS_OK, "Sentence count replaces malformed input");
    ATT_ASSERT(count, (size_t)1, "Replacement remains in one sentence");
    ATT_ASSERT(diagnostic.byte_offset, (size_t)1,
        "Sentence replacement retains malformed offset");

    text = mjb_text_utf8("A\x80", 2);
    ATT_ASSERT_STATUS(mjb_sentence_count(&text, MJB_MALFORMED_SKIP,
                          &count, &diagnostic),
        MJB_STATUS_OK, "Sentence count skips malformed input");
    ATT_ASSERT(count, (size_t)1, "Skipping malformed input remains one sentence");

    // The incomplete tail E2 82 is one maximal subpart: one U+FFFD with a position of its own,
    // where the iterator used to drop the tail and end at its start.
    {
        const size_t expected[] = { 1, 2, 4 };
        text = mjb_text_utf8("ab\xE2\x82", 4);
        mjb_sentence_break_state state;
        state.index = 0;
        size_t positions = 0;
        bool positions_match = true;

        while(mjb_sentence_break_next(&text, &state) != MJB_BT_NOT_SET) {
            positions_match = positions_match && positions < 3 &&
                state.position == expected[positions];
            ++positions;
        }

        ATT_ASSERT(positions, (size_t)3, "Sentence positions: incomplete tail is one subpart");
        ATT_ASSERT(positions_match, true, "Sentence positions: incomplete tail offsets");
    }

    // Text without a terminator is a single segment
    text = mjb_text_utf8("One sentence", 12);
    ATT_ASSERT_STATUS(mjb_sentence_count(&text, MJB_MALFORMED_STOP, &count, NULL),
        MJB_STATUS_OK, "Count: one sentence status");
    ATT_ASSERT(count, (size_t)1, "Count: one sentence");

    // Terminators split segments (SB4, SB11)
    text = mjb_text_utf8("Hello. How are you? Fine!", 25);
    ATT_ASSERT_STATUS(mjb_sentence_count(&text, MJB_MALFORMED_STOP, &count, NULL),
        MJB_STATUS_OK, "Count: three sentences status");
    ATT_ASSERT(count, (size_t)3, "Count: three sentences");
    text = mjb_text_utf8("One.\nTwo.\n", 10);
    ATT_ASSERT_STATUS(mjb_sentence_count(&text, MJB_MALFORMED_STOP, &count, NULL),
        MJB_STATUS_OK, "Count: newline paragraphs status");
    ATT_ASSERT(count, (size_t)2, "Count: newline paragraphs");

    // Trailing spaces attach to the previous sentence (SB10)
    text = mjb_text_utf8("Hi.   ", 6);
    ATT_ASSERT_STATUS(mjb_sentence_count(&text, MJB_MALFORMED_STOP, &count, NULL), MJB_STATUS_OK,
        "Count: trailing spaces status");
    ATT_ASSERT(count, (size_t)1, "Count: trailing spaces");

    // No break when the terminator is followed by lowercase (SB8)
    text = mjb_text_utf8("Wait... then more.", 18);
    ATT_ASSERT_STATUS(mjb_sentence_count(&text, MJB_MALFORMED_STOP, &count, NULL),
        MJB_STATUS_OK, "Count: ellipsis status");
    ATT_ASSERT(count, (size_t)1, "Count: ellipsis before lowercase");

    // Default rules carry no abbreviation list
    text = mjb_text_utf8("Dr. Smith went.", 15);
    ATT_ASSERT_STATUS(mjb_sentence_count(&text, MJB_MALFORMED_STOP, &count, NULL),
        MJB_STATUS_OK, "Count: abbreviation status");
    ATT_ASSERT(count, (size_t)2, "Count: abbreviation splits by default");

    // MJB_NUL_TERMINATED requests a terminator scan
    text = mjb_text_utf8("Hi. Bye.", MJB_NUL_TERMINATED);
    ATT_ASSERT_STATUS(mjb_sentence_count(&text, MJB_MALFORMED_STOP, &count, NULL),
        MJB_STATUS_OK, "Count: NUL-terminated status");
    ATT_ASSERT(count, (size_t)2, "Count: NUL-terminated");

    // UTF-16LE input
    const char utf16le_two[] = { 'A', '\0', '.', '\0', ' ', '\0', 'B', '\0', '.', '\0' };
    text = mjb_text_make(utf16le_two, 10, MJB_ENC_UTF_16LE);
    ATT_ASSERT_STATUS(mjb_sentence_count(&text, MJB_MALFORMED_STOP, &count, NULL),
        MJB_STATUS_OK, "Count: UTF-16LE status");
    ATT_ASSERT(count, (size_t)2, "Count: UTF-16LE two sentences");
}

ATT_TEST(break_sentence) {
    {
        mjb_sentence_break_state state;
        state.index = 0;
        size_t count = 9;
        ATT_ASSERT((int)mjb_sentence_break_next(NULL, &state), (int)MJB_BT_NOT_SET,
            "Sentence break rejects NULL text");
        ATT_ASSERT_STATUS(mjb_sentence_count(NULL, MJB_MALFORMED_STOP, &count, NULL),
            MJB_STATUS_INVALID_ARGUMENT, "Sentence count rejects NULL text");
        ATT_ASSERT(count, (size_t)0, "Sentence count clears count after NULL text");
    }

    mjb_sentence_break_state state;
    state.index = 0;

    mjb_text text = mjb_text_utf8(NULL, 1);
    ATT_ASSERT((uint8_t)mjb_sentence_break_next(&text, &state), (uint8_t)MJB_BT_NOT_SET,
        "Sentence break rejects NULL buffer");
    text = mjb_text_utf8("A", 1);
    ATT_ASSERT((uint8_t)mjb_sentence_break_next(&text, NULL), (uint8_t)MJB_BT_NOT_SET,
        "Sentence break rejects NULL state");

    test_sentence_count();

    read_test_file("./unicode-data/UCD/auxiliary/SentenceBreakTest.txt", &break_sentence_callback);
}
