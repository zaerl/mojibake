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

    while((bt = mjb_segmentation(buffer, size, MJB_ENCODING_UTF_8, &state)) != MJB_BT_NOT_SET) {
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

    // CURRENT_ASSERT mjb_segmentation
    // CURRENT_COUNT 711
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

    ATT_ASSERT((uint8_t)mjb_segmentation("", 0, MJB_ENCODING_UTF_8, &state), (uint8_t)MJB_BT_NOT_SET, "Empty string")

    MJB_TEST_S
    mjb_break_type expected_a[] = { MJB_BT_ALLOWED };

    while((bt = mjb_segmentation("A", 1, MJB_ENCODING_UTF_8, &state)) != MJB_BT_NOT_SET) {
        ATT_ASSERT((uint8_t)bt, (uint8_t)expected_a[index++], "A test")
    }

    MJB_TEST_S
    mjb_break_type expected_ab[] = { MJB_BT_ALLOWED, MJB_BT_ALLOWED };

    while((bt = mjb_segmentation("AB", 2, MJB_ENCODING_UTF_8, &state)) != MJB_BT_NOT_SET) {
        ATT_ASSERT((uint8_t)bt, (uint8_t)expected_ab[index++], "AB test")
    }
    ATT_ASSERT(index, 2, "AB test break index")

    MJB_TEST_S
    mjb_break_type expected_abc[] = { MJB_BT_ALLOWED, MJB_BT_ALLOWED, MJB_BT_ALLOWED };
    while((bt = mjb_segmentation("ABC", 3, MJB_ENCODING_UTF_8, &state)) != MJB_BT_NOT_SET) {
        ATT_ASSERT((uint8_t)bt, (uint8_t)expected_abc[index++], "AB test")
    }
    ATT_ASSERT(index, 3, "ABC test break index")

    MJB_TEST_S
    mjb_break_type expected_brnl[] = { MJB_BT_ALLOWED, MJB_BT_NO_BREAK, MJB_BT_ALLOWED, MJB_BT_ALLOWED };
    while((bt = mjb_segmentation("A\r\nB", 4, MJB_ENCODING_UTF_8, &state)) != MJB_BT_NOT_SET) {
        ATT_ASSERT((uint8_t)bt, (uint8_t)expected_brnl[index++], "A\\r\\nB test")
    }
    ATT_ASSERT(index, 4, "A\\r\\nB test break index")

    MJB_TEST_S
    mjb_break_type expected_itit[] = { MJB_BT_NO_BREAK, MJB_BT_ALLOWED, MJB_BT_NO_BREAK, MJB_BT_ALLOWED };
    while((bt = mjb_segmentation("🇮🇹🇮🇹", 16, MJB_ENCODING_UTF_8, &state)) != MJB_BT_NOT_SET) {
        ATT_ASSERT((uint8_t)bt, (uint8_t)expected_itit[index++], "ITIT test")
    }
    ATT_ASSERT(index, 4, "ITIT test break index")

    #undef MJB_TEST_S
}

static void test_truncate(void) {
    /* mjb_truncate: empty / zero */
    ATT_ASSERT(mjb_truncate("", 0, MJB_ENCODING_UTF_8, 3), (size_t)0, "Truncate: empty string")
    ATT_ASSERT(mjb_truncate("ABC", 3, MJB_ENCODING_UTF_8, 0), (size_t)0, "Truncate: 0 graphemes")

    /* ASCII: each byte is one grapheme cluster */
    ATT_ASSERT(mjb_truncate("ABC", 3, MJB_ENCODING_UTF_8, 1), (size_t)1, "Truncate: ABC to 1")
    ATT_ASSERT(mjb_truncate("ABC", 3, MJB_ENCODING_UTF_8, 2), (size_t)2, "Truncate: ABC to 2")
    ATT_ASSERT(mjb_truncate("ABC", 3, MJB_ENCODING_UTF_8, 3), (size_t)3, "Truncate: ABC to 3 (no-op)")
    ATT_ASSERT(mjb_truncate("ABC", 3, MJB_ENCODING_UTF_8, 5), (size_t)3, "Truncate: ABC to 5 (no-op)")

    /* Multi-byte: "aé" = 0x61 0xC3 0xA9 = 3 bytes, 2 grapheme clusters */
    ATT_ASSERT(mjb_truncate("a\xC3\xA9", 3, MJB_ENCODING_UTF_8, 1), (size_t)1, "Truncate: aé to 1 grapheme")
    ATT_ASSERT(mjb_truncate("a\xC3\xA9", 3, MJB_ENCODING_UTF_8, 2), (size_t)3, "Truncate: aé to 2 graphemes (no-op)")

    /* Flag emoji 🇺🇸 = two RI codepoints (4+4=8 bytes), one grapheme cluster */
    ATT_ASSERT(mjb_truncate("\xF0\x9F\x87\xBA\xF0\x9F\x87\xB8", 8, MJB_ENCODING_UTF_8, 1), (size_t)8, "Truncate: flag emoji to 1 grapheme (no-op)")

    /* mjb_truncate_width */
    ATT_ASSERT(mjb_truncate_width("", 0, MJB_ENCODING_UTF_8, MJB_WIDTH_CONTEXT_WESTERN, 5), (size_t)0, "Truncate width: empty string")
    ATT_ASSERT(mjb_truncate_width("ABC", 3, MJB_ENCODING_UTF_8, MJB_WIDTH_CONTEXT_WESTERN, 0), (size_t)0, "Truncate width: 0 columns")
    ATT_ASSERT(mjb_truncate_width("ABC", 3, MJB_ENCODING_UTF_8, MJB_WIDTH_CONTEXT_WESTERN, 2), (size_t)2, "Truncate width: ABC to 2 columns")
    ATT_ASSERT(mjb_truncate_width("ABC", 3, MJB_ENCODING_UTF_8, MJB_WIDTH_CONTEXT_WESTERN, 3), (size_t)3, "Truncate width: ABC to 3 columns (no-op)")
    ATT_ASSERT(mjb_truncate_width("ABC", 3, MJB_ENCODING_UTF_8, MJB_WIDTH_CONTEXT_WESTERN, 10), (size_t)3, "Truncate width: ABC to 10 columns (no-op)")
}

void *test_segmentation(void *arg) {
    test_basic_segmentation();
    test_truncate();
    read_test_file("./utils/generate/UCD/auxiliary/GraphemeBreakTest.txt", &segmentation_callback);

    return NULL;
}
