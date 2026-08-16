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
    mjb_next_sentence_state state;
    state.index = 0;
    size_t index = 0;
    size_t successful_count = 0;

    while((bt = mjb_next_sentence_break(buffer, byte_length, MJB_ENC_UTF_8, &state)) !=
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

    MJB_TEST_COVERAGE(mjb_next_sentence_break);
    ATT_ASSERT(index, successful_count, test_name)
}

static void test_sentence_count(void) {
    size_t count = 6251;

    // Argument validation
    ATT_ASSERT_STATUS(mjb_sentence_count("A", 1, MJB_ENC_UTF_8, NULL),
        MJB_STATUS_INVALID_ARGUMENT, "Count rejects NULL count")
    ATT_ASSERT_STATUS(mjb_sentence_count(NULL, 1, MJB_ENC_UTF_8, &count),
        MJB_STATUS_INVALID_ARGUMENT, "Count rejects NULL buffer")
    ATT_ASSERT(count, (size_t)0, "Count is zero after NULL buffer")

    count = 6251;
    ATT_ASSERT_STATUS(mjb_sentence_count("A", 1, MJB_ENC_UNKNOWN, &count),
        MJB_STATUS_INVALID_ENCODING, "Count rejects invalid encoding")
    ATT_ASSERT(count, (size_t)0, "Count is zero after invalid encoding")

    // Empty input is valid and counts zero segments
    ATT_ASSERT_STATUS(mjb_sentence_count("", 0, MJB_ENC_UTF_8, &count), MJB_STATUS_OK,
        "Count: empty string status")
    ATT_ASSERT(count, (size_t)0, "Count: empty string")
    ATT_ASSERT_STATUS(mjb_sentence_count(NULL, 0, MJB_ENC_UTF_8, &count), MJB_STATUS_OK,
        "Count: NULL buffer with zero length status")

    // Text without a terminator is a single segment
    ATT_ASSERT_STATUS(mjb_sentence_count("One sentence", 12, MJB_ENC_UTF_8, &count),
        MJB_STATUS_OK, "Count: one sentence status")
    ATT_ASSERT(count, (size_t)1, "Count: one sentence")

    // Terminators split segments (SB4, SB11)
    ATT_ASSERT_STATUS(mjb_sentence_count("Hello. How are you? Fine!", 25, MJB_ENC_UTF_8, &count),
        MJB_STATUS_OK, "Count: three sentences status")
    ATT_ASSERT(count, (size_t)3, "Count: three sentences")
    ATT_ASSERT_STATUS(mjb_sentence_count("One.\nTwo.\n", 10, MJB_ENC_UTF_8, &count),
        MJB_STATUS_OK, "Count: newline paragraphs status")
    ATT_ASSERT(count, (size_t)2, "Count: newline paragraphs")

    // Trailing spaces attach to the previous sentence (SB10)
    ATT_ASSERT_STATUS(mjb_sentence_count("Hi.   ", 6, MJB_ENC_UTF_8, &count), MJB_STATUS_OK,
        "Count: trailing spaces status")
    ATT_ASSERT(count, (size_t)1, "Count: trailing spaces")

    // No break when the terminator is followed by lowercase (SB8)
    ATT_ASSERT_STATUS(mjb_sentence_count("Wait... then more.", 18, MJB_ENC_UTF_8, &count),
        MJB_STATUS_OK, "Count: ellipsis status")
    ATT_ASSERT(count, (size_t)1, "Count: ellipsis before lowercase")

    // Default rules carry no abbreviation list
    ATT_ASSERT_STATUS(mjb_sentence_count("Dr. Smith went.", 15, MJB_ENC_UTF_8, &count),
        MJB_STATUS_OK, "Count: abbreviation status")
    ATT_ASSERT(count, (size_t)2, "Count: abbreviation splits by default")

    // MJB_NUL_TERMINATED requests a terminator scan
    ATT_ASSERT_STATUS(mjb_sentence_count("Hi. Bye.", MJB_NUL_TERMINATED, MJB_ENC_UTF_8, &count),
        MJB_STATUS_OK, "Count: NUL-terminated status")
    ATT_ASSERT(count, (size_t)2, "Count: NUL-terminated")

    // UTF-16LE input
    const char utf16le_two[] = { 'A', '\0', '.', '\0', ' ', '\0', 'B', '\0', '.', '\0' };
    ATT_ASSERT_STATUS(mjb_sentence_count(utf16le_two, 10, MJB_ENC_UTF_16LE, &count),
        MJB_STATUS_OK, "Count: UTF-16LE status")
    ATT_ASSERT(count, (size_t)2, "Count: UTF-16LE two sentences")
}

int test_break_sentence(void *arg) {
    mjb_next_sentence_state state;
    state.index = 0;

    ATT_ASSERT((uint8_t)mjb_next_sentence_break(NULL, 1, MJB_ENC_UTF_8, &state), (uint8_t)MJB_BT_NOT_SET,
        "Sentence break rejects NULL buffer")
    ATT_ASSERT((uint8_t)mjb_next_sentence_break("A", 1, MJB_ENC_UTF_8, NULL), (uint8_t)MJB_BT_NOT_SET,
        "Sentence break rejects NULL state")

    test_sentence_count();

    read_test_file("./utils/generate/unicode-data/UCD/auxiliary/SentenceBreakTest.txt",
        &break_sentence_callback);

    return 0;
}
