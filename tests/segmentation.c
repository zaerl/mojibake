/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <string.h>

#include "test.h"

void segmentation_callback(const char *buffer, size_t size, unsigned int current_line, mjb_break_type *expected_types) {
    char test_name[256];
    mjb_break_type bt = MJB_BT_NOT_SET;
    mjb_next_state state;
    state.index = 0;
    size_t index = 0;
    size_t successful_count = 0;

    while((bt = mjb_segmentation(buffer, size, MJB_ENC_UTF_8, &state)) != MJB_BT_NOT_SET) {
        snprintf(test_name, 256, "Index %zu", index);

        if(bt == MJB_BT_MANDATORY) {
            bt = MJB_BT_ALLOWED;
        }

        if((uint8_t)bt == (uint8_t)expected_types[index++]) {
            ++successful_count;
        } else  {
            break;
        }
    }

    MJB_TEST_COVERAGE(mjb_segmentation);
    ATT_ASSERT(index, successful_count, test_name)
}

static void test_basic_segmentation(void) {
    mjb_next_state state;
    mjb_break_type bt = MJB_BT_NOT_SET;
    state.index = 0;
    size_t index = 0;

    #define MJB_TEST_S \
        state.index = 0; \
        index = 0; \

    ATT_ASSERT((uint8_t)mjb_segmentation(NULL, 1, MJB_ENC_UTF_8, &state),
        (uint8_t)MJB_BT_NOT_SET, "Segmentation rejects NULL buffer")
    ATT_ASSERT((uint8_t)mjb_segmentation("A", 1, MJB_ENC_UTF_8, NULL),
        (uint8_t)MJB_BT_NOT_SET, "Segmentation rejects NULL state")
    ATT_ASSERT((uint8_t)mjb_segmentation("", 0, MJB_ENC_UTF_8, &state), (uint8_t)MJB_BT_NOT_SET, "Empty string")
    ATT_ASSERT(mjb_truncate(NULL, 1, MJB_ENC_UTF_8, 1), (size_t)0, "Truncate rejects NULL buffer")
    ATT_ASSERT(mjb_truncate_width(NULL, 1, MJB_ENC_UTF_8, MJB_WIDTH_CONTEXT_WESTERN, 1),
        (size_t)0, "Truncate width rejects NULL buffer")

#if !defined(MJB_DANGEROUSLY_ALLOW_EMBEDDED_NULLS) || !MJB_DANGEROUSLY_ALLOW_EMBEDDED_NULLS
    const char utf16le_null[] = { '\0', '\0', 'A', '\0' };

    MJB_TEST_S
    ATT_ASSERT((uint8_t)mjb_segmentation(utf16le_null, sizeof(utf16le_null),
        MJB_ENC_UTF_16LE, &state), (uint8_t)MJB_BT_ALLOWED,
        "Segmentation stops at UTF-16LE NULL")
    ATT_ASSERT((uint8_t)mjb_segmentation(utf16le_null, sizeof(utf16le_null),
        MJB_ENC_UTF_16LE, &state), (uint8_t)MJB_BT_NOT_SET,
        "Segmentation finishes after UTF-16LE NULL")
#endif

    MJB_TEST_S
    mjb_break_type expected_a[] = { MJB_BT_ALLOWED };

    MJB_TEST_COVERAGE(mjb_segmentation);
    while((bt = mjb_segmentation("A", 1, MJB_ENC_UTF_8, &state)) != MJB_BT_NOT_SET) {
        ATT_ASSERT((uint8_t)bt, (uint8_t)expected_a[index++], "A test")
    }

    MJB_TEST_S
    mjb_break_type expected_ab[] = { MJB_BT_ALLOWED, MJB_BT_ALLOWED };

    MJB_TEST_COVERAGE(mjb_segmentation);
    while((bt = mjb_segmentation("AB", 2, MJB_ENC_UTF_8, &state)) != MJB_BT_NOT_SET) {
        ATT_ASSERT((uint8_t)bt, (uint8_t)expected_ab[index++], "AB test")
    }
    ATT_ASSERT(index, 2, "AB test break index")

    MJB_TEST_S
    mjb_break_type expected_abc[] = { MJB_BT_ALLOWED, MJB_BT_ALLOWED, MJB_BT_ALLOWED };
    MJB_TEST_COVERAGE(mjb_segmentation);
    while((bt = mjb_segmentation("ABC", 3, MJB_ENC_UTF_8, &state)) != MJB_BT_NOT_SET) {
        ATT_ASSERT((uint8_t)bt, (uint8_t)expected_abc[index++], "AB test")
    }
    ATT_ASSERT(index, 3, "ABC test break index")

    MJB_TEST_S
    mjb_break_type expected_brnl[] = { MJB_BT_ALLOWED, MJB_BT_NO_BREAK, MJB_BT_ALLOWED, MJB_BT_ALLOWED };
    MJB_TEST_COVERAGE(mjb_segmentation);
    while((bt = mjb_segmentation("A\r\nB", 4, MJB_ENC_UTF_8, &state)) != MJB_BT_NOT_SET) {
        ATT_ASSERT((uint8_t)bt, (uint8_t)expected_brnl[index++], "A\\r\\nB test")
    }
    ATT_ASSERT(index, 4, "A\\r\\nB test break index")

    MJB_TEST_S
    mjb_break_type expected_itit[] = { MJB_BT_NO_BREAK, MJB_BT_ALLOWED, MJB_BT_NO_BREAK, MJB_BT_ALLOWED };
    MJB_TEST_COVERAGE(mjb_segmentation);
    while((bt = mjb_segmentation("🇮🇹🇮🇹", 16, MJB_ENC_UTF_8, &state)) != MJB_BT_NOT_SET) {
        ATT_ASSERT((uint8_t)bt, (uint8_t)expected_itit[index++], "ITIT test")
    }
    ATT_ASSERT(index, 4, "ITIT test break index")

    #undef MJB_TEST_S
}

static void test_truncate(void) {
    // mjb_truncate: empty / zero
    ATT_ASSERT(mjb_truncate("", 0, MJB_ENC_UTF_8, 3), (size_t)0, "Truncate: empty string")
    ATT_ASSERT(mjb_truncate("ABC", 3, MJB_ENC_UTF_8, 0), (size_t)0, "Truncate: 0 graphemes")

    // ASCII: each byte is one grapheme cluster
    ATT_ASSERT(mjb_truncate("ABC", 3, MJB_ENC_UTF_8, 1), (size_t)1, "Truncate: ABC to 1")
    ATT_ASSERT(mjb_truncate("ABC", 3, MJB_ENC_UTF_8, 2), (size_t)2, "Truncate: ABC to 2")
    ATT_ASSERT(mjb_truncate("ABC", 3, MJB_ENC_UTF_8, 3), (size_t)3, "Truncate: ABC to 3 (no-op)")
    ATT_ASSERT(mjb_truncate("ABC", 3, MJB_ENC_UTF_8, 5), (size_t)3, "Truncate: ABC to 5 (no-op)")

    // Multi-byte: "aé" = 0x61 0xC3 0xA9 = 3 bytes, 2 grapheme clusters
    ATT_ASSERT(mjb_truncate("a\xC3\xA9", 3, MJB_ENC_UTF_8, 1), (size_t)1, "Truncate: aé to 1 grapheme")
    ATT_ASSERT(mjb_truncate("a\xC3\xA9", 3, MJB_ENC_UTF_8, 2), (size_t)3, "Truncate: aé to 2 graphemes (no-op)")

    // Flag emoji 🇺🇸 = two RI codepoints (4+4=8 bytes), one grapheme cluster
    ATT_ASSERT(mjb_truncate("\xF0\x9F\x87\xBA\xF0\x9F\x87\xB8", 8, MJB_ENC_UTF_8, 1), (size_t)8, "Truncate: flag emoji to 1 grapheme (no-op)")

    // mjb_truncate_width
    ATT_ASSERT(mjb_truncate_width("", 0, MJB_ENC_UTF_8, MJB_WIDTH_CONTEXT_WESTERN, 5), (size_t)0, "Truncate width: empty string")
    ATT_ASSERT(mjb_truncate_width("ABC", 3, MJB_ENC_UTF_8, MJB_WIDTH_CONTEXT_WESTERN, 0), (size_t)0, "Truncate width: 0 columns")
    ATT_ASSERT(mjb_truncate_width("ABC", 3, MJB_ENC_UTF_8, MJB_WIDTH_CONTEXT_WESTERN, 2), (size_t)2, "Truncate width: ABC to 2 columns")
    ATT_ASSERT(mjb_truncate_width("ABC", 3, MJB_ENC_UTF_8, MJB_WIDTH_CONTEXT_WESTERN, 3), (size_t)3, "Truncate width: ABC to 3 columns (no-op)")
    ATT_ASSERT(mjb_truncate_width("ABC", 3, MJB_ENC_UTF_8, MJB_WIDTH_CONTEXT_WESTERN, 10), (size_t)3, "Truncate width: ABC to 10 columns (no-op)")

#if !defined(MJB_DANGEROUSLY_ALLOW_EMBEDDED_NULLS) || !MJB_DANGEROUSLY_ALLOW_EMBEDDED_NULLS
    const char utf16le_with_null[] = { 'A', '\0', '\0', '\0', 'B', '\0' };
    const char utf16le_null_start[] = { '\0', '\0', 'A', '\0' };
    const char timeout_repro[] = {
        '\0', '\0', '\x10', '\xDF', '\x03', '\x1F', '\x01', '\0',
        '\0', '\0', '\0', '\0', '\0', '\0', '\xFF', '\xDC',
        '\xFF', '\0', '\xF3', '\xA1', ' ', 'S'
    };

    ATT_ASSERT(mjb_truncate(utf16le_with_null, sizeof(utf16le_with_null),
        MJB_ENC_UTF_16LE, 5), (size_t)2,
        "Truncate: UTF-16LE stops at NULL")
    ATT_ASSERT(mjb_truncate_width(utf16le_with_null, sizeof(utf16le_with_null),
        MJB_ENC_UTF_16LE, MJB_WIDTH_CONTEXT_WESTERN, 10), (size_t)2,
        "Truncate width: UTF-16LE stops at NULL")
    ATT_ASSERT(mjb_truncate_width(utf16le_null_start, sizeof(utf16le_null_start),
        MJB_ENC_UTF_16LE, MJB_WIDTH_CONTEXT_WESTERN, 1), (size_t)0,
        "Truncate width: UTF-16LE NULL start")
    ATT_ASSERT(mjb_truncate_width(timeout_repro, sizeof(timeout_repro),
        MJB_ENC_UTF_16LE, MJB_WIDTH_CONTEXT_WESTERN, 1), (size_t)0,
        "Truncate width: fuzzer timeout regression")
#endif

    const char malformed_utf8_width[] = { 'a', '\x17', '\xCE', '\x08', 's', 't' };

    ATT_ASSERT(mjb_truncate_width(malformed_utf8_width, sizeof(malformed_utf8_width),
        MJB_ENC_UTF_8, MJB_WIDTH_CONTEXT_WESTERN, 114),
        sizeof(malformed_utf8_width), "Truncate width: malformed UTF-8 regression")
}

int test_segmentation(void *arg) {
    test_basic_segmentation();
    test_truncate();
    read_test_file(
        "./utils/generate/unicode-data/UCD/auxiliary/GraphemeBreakTest.txt",
        &segmentation_callback
    );

    return 0;
}
