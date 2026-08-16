/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <string.h>

#include "test.h"

static void segmentation_callback(const char *buffer, size_t byte_length, unsigned int current_line,
    mjb_break_type *expected_types) {
    char test_name[256];
    mjb_break_type bt = MJB_BT_NOT_SET;
    mjb_next_state state;
    state.index = 0;
    size_t index = 0;
    size_t successful_count = 0;

    while((bt = mjb_next_grapheme_break(buffer, byte_length, MJB_ENC_UTF_8, &state)) !=
        MJB_BT_NOT_SET) {
        snprintf(test_name, 256, "Index %zu", index);

        if(bt == MJB_BT_MANDATORY) {
            bt = MJB_BT_ALLOWED;
        }

        if((uint8_t)bt == (uint8_t)expected_types[index++]) {
            ++successful_count;
        } else {
            break;
        }
    }

    MJB_TEST_COVERAGE(mjb_next_grapheme_break);
    ATT_ASSERT(index, successful_count, test_name)
}

static void test_basic_segmentation(void) {
    mjb_next_state state;
    mjb_break_type bt = MJB_BT_NOT_SET;
    state.index = 0;
    size_t index = 0;

#define MJB_TEST_S \
    state.index = 0; \
    index = 0;

    ATT_ASSERT((uint8_t)mjb_next_grapheme_break(NULL, 1, MJB_ENC_UTF_8, &state),
        (uint8_t)MJB_BT_NOT_SET, "Segmentation rejects NULL buffer")
    ATT_ASSERT((uint8_t)mjb_next_grapheme_break("A", 1, MJB_ENC_UTF_8, NULL),
        (uint8_t)MJB_BT_NOT_SET, "Segmentation rejects NULL state")
    ATT_ASSERT((uint8_t)mjb_next_grapheme_break("", 0, MJB_ENC_UTF_8, &state),
        (uint8_t)MJB_BT_NOT_SET, "Empty string")
    ATT_ASSERT(mjb_truncate_grapheme(NULL, 1, MJB_ENC_UTF_8, 1), (size_t)0, "Truncate rejects NULL buffer")
    ATT_ASSERT(mjb_truncate_grapheme_width(NULL, 1, MJB_ENC_UTF_8,
        MJB_TERMINAL_WIDTH_NARROW, 1), (size_t)0,
        "Truncate width rejects NULL buffer")

    MJB_TEST_S
    mjb_break_type expected_a[] = { MJB_BT_ALLOWED };

    MJB_TEST_COVERAGE(mjb_next_grapheme_break);

    while((bt = mjb_next_grapheme_break("A", 1, MJB_ENC_UTF_8, &state)) != MJB_BT_NOT_SET) {
        ATT_ASSERT((uint8_t)bt, (uint8_t)expected_a[index++], "A test")
    }

    MJB_TEST_S
    mjb_break_type expected_ab[] = { MJB_BT_ALLOWED, MJB_BT_ALLOWED };

    MJB_TEST_COVERAGE(mjb_next_grapheme_break);

    while((bt = mjb_next_grapheme_break("AB", 2, MJB_ENC_UTF_8, &state)) != MJB_BT_NOT_SET) {
        ATT_ASSERT((uint8_t)bt, (uint8_t)expected_ab[index++], "AB test")
    }

    ATT_ASSERT(index, 2, "AB test break index")

    MJB_TEST_S
    mjb_break_type expected_abc[] = { MJB_BT_ALLOWED, MJB_BT_ALLOWED, MJB_BT_ALLOWED };
    MJB_TEST_COVERAGE(mjb_next_grapheme_break);

    while((bt = mjb_next_grapheme_break("ABC", 3, MJB_ENC_UTF_8, &state)) != MJB_BT_NOT_SET) {
        ATT_ASSERT((uint8_t)bt, (uint8_t)expected_abc[index++], "AB test")
    }

    ATT_ASSERT(index, 3, "ABC test break index")

    MJB_TEST_S
    mjb_break_type expected_brnl[] = { MJB_BT_ALLOWED, MJB_BT_NO_BREAK, MJB_BT_ALLOWED,
        MJB_BT_ALLOWED };
    MJB_TEST_COVERAGE(mjb_next_grapheme_break);

    while((bt = mjb_next_grapheme_break("A\r\nB", 4, MJB_ENC_UTF_8, &state)) != MJB_BT_NOT_SET) {
        ATT_ASSERT((uint8_t)bt, (uint8_t)expected_brnl[index++], "A\\r\\nB test")
    }

    ATT_ASSERT(index, 4, "A\\r\\nB test break index")

    MJB_TEST_S
    mjb_break_type expected_itit[] = { MJB_BT_NO_BREAK, MJB_BT_ALLOWED, MJB_BT_NO_BREAK,
        MJB_BT_ALLOWED };
    MJB_TEST_COVERAGE(mjb_next_grapheme_break);

    while((bt = mjb_next_grapheme_break("🇮🇹🇮🇹", 16, MJB_ENC_UTF_8, &state)) != MJB_BT_NOT_SET) {
        ATT_ASSERT((uint8_t)bt, (uint8_t)expected_itit[index++], "ITIT test")
    }

    ATT_ASSERT(index, 4, "ITIT test break index")

    // Unicode 18 GB9c no longer requires a preceding InCB=Consonant. A Linker followed by zero or
    // more InCB=Extend characters joins directly to the following InCB=Consonant.
    const char gb9c[] = "\xE0\xA5\x8D\xCC\x80\xE0\xA4\x95"; // 094D 0300 0915
    mjb_break_type expected_gb9c[] = { MJB_BT_NO_BREAK, MJB_BT_NO_BREAK, MJB_BT_ALLOWED };

    MJB_TEST_S
    MJB_TEST_COVERAGE(mjb_next_grapheme_break);

    while((bt = mjb_next_grapheme_break(gb9c, sizeof(gb9c) - 1, MJB_ENC_UTF_8, &state)) !=
        MJB_BT_NOT_SET) {
        ATT_ASSERT((uint8_t)bt, (uint8_t)expected_gb9c[index++], "Unicode 18 GB9c")
    }

    ATT_ASSERT(index, 3, "Unicode 18 GB9c break index")

#undef MJB_TEST_S
}

static void test_truncate(void) {
    // mjb_truncate_grapheme: empty / zero
    ATT_ASSERT(mjb_truncate_grapheme("", 0, MJB_ENC_UTF_8, 3), (size_t)0, "Truncate: empty string")
    ATT_ASSERT(mjb_truncate_grapheme("ABC", 3, MJB_ENC_UTF_8, 0), (size_t)0, "Truncate: 0 graphemes")

    // ASCII: each byte is one grapheme cluster
    ATT_ASSERT(mjb_truncate_grapheme("ABC", 3, MJB_ENC_UTF_8, 1), (size_t)1, "Truncate: ABC to 1")
    ATT_ASSERT(mjb_truncate_grapheme("ABC", 3, MJB_ENC_UTF_8, 2), (size_t)2, "Truncate: ABC to 2")
    ATT_ASSERT(mjb_truncate_grapheme("ABC", 3, MJB_ENC_UTF_8, 3), (size_t)3, "Truncate: ABC to 3 (no-op)")
    ATT_ASSERT(mjb_truncate_grapheme("ABC", 3, MJB_ENC_UTF_8, 5), (size_t)3, "Truncate: ABC to 5 (no-op)")

    // Multi-byte: "aé" = 0x61 0xC3 0xA9 = 3 bytes, 2 grapheme clusters
    ATT_ASSERT(mjb_truncate_grapheme("a\xC3\xA9", 3, MJB_ENC_UTF_8, 1), (size_t)1,
        "Truncate: aé to 1 grapheme")
    ATT_ASSERT(mjb_truncate_grapheme("a\xC3\xA9", 3, MJB_ENC_UTF_8, 2), (size_t)3,
        "Truncate: aé to 2 graphemes (no-op)")

    // Flag emoji 🇺🇸 = two RI codepoints (4+4=8 bytes), one grapheme cluster
    ATT_ASSERT(mjb_truncate_grapheme("\xF0\x9F\x87\xBA\xF0\x9F\x87\xB8", 8, MJB_ENC_UTF_8, 1), (size_t)8,
        "Truncate: flag emoji to 1 grapheme (no-op)")

    // mjb_truncate_grapheme_width
    ATT_ASSERT(mjb_truncate_grapheme_width("", 0, MJB_ENC_UTF_8,
        MJB_TERMINAL_WIDTH_NARROW, 5), (size_t)0,
        "Truncate width: empty string")
    ATT_ASSERT(mjb_truncate_grapheme_width("ABC", 3, MJB_ENC_UTF_8,
        MJB_TERMINAL_WIDTH_NARROW, 0), (size_t)0,
        "Truncate width: 0 columns")
    ATT_ASSERT(mjb_truncate_grapheme_width("ABC", 3, MJB_ENC_UTF_8,
        MJB_TERMINAL_WIDTH_NARROW, 2), (size_t)2,
        "Truncate width: ABC to 2 columns")
    ATT_ASSERT(mjb_truncate_grapheme_width("ABC", 3, MJB_ENC_UTF_8,
        MJB_TERMINAL_WIDTH_NARROW, 3), (size_t)3,
        "Truncate width: ABC to 3 columns (no-op)")
    ATT_ASSERT(mjb_truncate_grapheme_width("ABC", 3, MJB_ENC_UTF_8,
        MJB_TERMINAL_WIDTH_NARROW, 10),
        (size_t)3, "Truncate width: ABC to 10 columns (no-op)")

    const char malformed_utf8_width[] = { 'a', '\x17', '\xCE', '\x08', 's', 't' };

    ATT_ASSERT(mjb_truncate_grapheme_width(malformed_utf8_width, sizeof(malformed_utf8_width), MJB_ENC_UTF_8,
        MJB_TERMINAL_WIDTH_NARROW, 114), (size_t)1,
        "Truncate width stops before malformed or stateful terminal input")

    const char *kiss = "\xf0\x9f\x91\xa8\xf0\x9f\x8f\xbb\xe2\x80\x8d\xe2\x9d\xa4\xef\xb8\x8f"
                       "\xe2\x80\x8d\xf0\x9f\x92\x8b\xe2\x80\x8d\xf0\x9f\x91\xa8\xf0\x9f\x8f\xbb";
    ATT_ASSERT(mjb_truncate_grapheme_width(kiss, strlen(kiss), MJB_ENC_UTF_8,
        MJB_TERMINAL_WIDTH_NARROW, 1), (size_t)0, "Emoji does not fit in one terminal cell")
    ATT_ASSERT(mjb_truncate_grapheme_width(kiss, strlen(kiss), MJB_ENC_UTF_8,
        MJB_TERMINAL_WIDTH_NARROW, 2), strlen(kiss), "Emoji fits in two terminal cells")
}

static void test_grapheme_count(void) {
    size_t count = 12345;

    // Argument validation
    ATT_ASSERT_STATUS(mjb_grapheme_count("A", 1, MJB_ENC_UTF_8, NULL),
        MJB_STATUS_INVALID_ARGUMENT, "Count rejects NULL count")
    ATT_ASSERT_STATUS(mjb_grapheme_count(NULL, 1, MJB_ENC_UTF_8, &count),
        MJB_STATUS_INVALID_ARGUMENT, "Count rejects NULL buffer")
    ATT_ASSERT(count, (size_t)0, "Count is zero after NULL buffer")

    count = 12345;
    ATT_ASSERT_STATUS(mjb_grapheme_count("A", 1, MJB_ENC_UNKNOWN, &count),
        MJB_STATUS_INVALID_ENCODING, "Count rejects invalid encoding")
    ATT_ASSERT(count, (size_t)0, "Count is zero after invalid encoding")

    // Empty input is valid and counts zero clusters
    ATT_ASSERT_STATUS(mjb_grapheme_count("", 0, MJB_ENC_UTF_8, &count), MJB_STATUS_OK,
        "Count: empty string status")
    ATT_ASSERT(count, (size_t)0, "Count: empty string")
    ATT_ASSERT_STATUS(mjb_grapheme_count(NULL, 0, MJB_ENC_UTF_8, &count), MJB_STATUS_OK,
        "Count: NULL buffer with zero length status")

    // ASCII: each byte is one grapheme cluster
    ATT_ASSERT_STATUS(mjb_grapheme_count("ABC", 3, MJB_ENC_UTF_8, &count), MJB_STATUS_OK,
        "Count: ABC status")
    ATT_ASSERT(count, (size_t)3, "Count: ABC")

    // "aé" = 0x61 0xC3 0xA9 = 3 bytes, 2 grapheme clusters
    ATT_ASSERT_STATUS(mjb_grapheme_count("a\xC3\xA9", 3, MJB_ENC_UTF_8, &count), MJB_STATUS_OK,
        "Count: aé status")
    ATT_ASSERT(count, (size_t)2, "Count: aé")

    // a + combining acute accent = one user-perceived character
    ATT_ASSERT_STATUS(mjb_grapheme_count("a\xCC\x81", 3, MJB_ENC_UTF_8, &count), MJB_STATUS_OK,
        "Count: a + combining acute status")
    ATT_ASSERT(count, (size_t)1, "Count: a + combining acute")

    // Flag emoji 🇺🇸 = two RI codepoints, one grapheme cluster; two flags do not pair across
    ATT_ASSERT_STATUS(mjb_grapheme_count("\xF0\x9F\x87\xBA\xF0\x9F\x87\xB8", 8, MJB_ENC_UTF_8,
        &count), MJB_STATUS_OK, "Count: flag emoji status")
    ATT_ASSERT(count, (size_t)1, "Count: flag emoji")
    ATT_ASSERT_STATUS(mjb_grapheme_count("\xF0\x9F\x87\xAE\xF0\x9F\x87\xB9"
        "\xF0\x9F\x87\xBA\xF0\x9F\x87\xB8", 16, MJB_ENC_UTF_8, &count), MJB_STATUS_OK,
        "Count: two flags status")
    ATT_ASSERT(count, (size_t)2, "Count: two flags")

    // Family ZWJ sequence 👨‍👩‍👦 = one grapheme cluster
    ATT_ASSERT_STATUS(mjb_grapheme_count("\xF0\x9F\x91\xA8\xE2\x80\x8D\xF0\x9F\x91\xA9"
        "\xE2\x80\x8D\xF0\x9F\x91\xA6", 18, MJB_ENC_UTF_8, &count), MJB_STATUS_OK,
        "Count: family ZWJ status")
    ATT_ASSERT(count, (size_t)1, "Count: family ZWJ sequence")

    // Hangul jamo L+V+T = one syllable cluster
    ATT_ASSERT_STATUS(mjb_grapheme_count("\xE1\x84\x80\xE1\x85\xA1\xE1\x86\xA8", 9, MJB_ENC_UTF_8,
        &count), MJB_STATUS_OK, "Count: Hangul jamo status")
    ATT_ASSERT(count, (size_t)1, "Count: Hangul jamo LVT syllable")

    // CRLF is a single cluster (GB3)
    ATT_ASSERT_STATUS(mjb_grapheme_count("a\r\nb", 4, MJB_ENC_UTF_8, &count), MJB_STATUS_OK,
        "Count: CRLF status")
    ATT_ASSERT(count, (size_t)3, "Count: a CRLF b")

    // Malformed byte mid-string counts as one replacement cluster
    ATT_ASSERT_STATUS(mjb_grapheme_count("a\x80z", 3, MJB_ENC_UTF_8, &count), MJB_STATUS_OK,
        "Count: malformed mid-string status")
    ATT_ASSERT(count, (size_t)3, "Count: malformed mid-string")

    // Incomplete trailing sequence does not add a cluster
    ATT_ASSERT_STATUS(mjb_grapheme_count("a\xC3", 2, MJB_ENC_UTF_8, &count), MJB_STATUS_OK,
        "Count: incomplete tail status")
    ATT_ASSERT(count, (size_t)1, "Count: incomplete tail")

    // Explicit lengths include embedded U+0000 codepoints
    const char embedded_nul[] = { 'A', '\0', 'B' };
    ATT_ASSERT_STATUS(mjb_grapheme_count(embedded_nul, 3, MJB_ENC_UTF_8, &count), MJB_STATUS_OK,
        "Count: embedded NUL status")
    ATT_ASSERT(count, (size_t)3, "Count: embedded NUL")

    // MJB_NUL_TERMINATED requests a terminator scan
    ATT_ASSERT_STATUS(mjb_grapheme_count("hi", MJB_NUL_TERMINATED, MJB_ENC_UTF_8, &count),
        MJB_STATUS_OK, "Count: NUL-terminated status")
    ATT_ASSERT(count, (size_t)2, "Count: NUL-terminated")

    // UTF-16LE input
    const char utf16le_ab[] = { 'A', '\0', 'B', '\0' };
    ATT_ASSERT_STATUS(mjb_grapheme_count(utf16le_ab, 4, MJB_ENC_UTF_16LE, &count), MJB_STATUS_OK,
        "Count: UTF-16LE status")
    ATT_ASSERT(count, (size_t)2, "Count: UTF-16LE AB")
}

int test_segmentation(void *arg) {
    test_basic_segmentation();
    test_truncate();
    test_grapheme_count();
    read_test_file("./utils/generate/unicode-data/UCD/auxiliary/GraphemeBreakTest.txt",
        &segmentation_callback);

    return 0;
}
