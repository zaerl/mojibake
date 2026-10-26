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
    (void)current_line;

    char test_name[256];
    mjb_break_type bt = MJB_BT_NOT_SET;
    mjb_grapheme_break_state state;
    state.index = 0;
    size_t index = 0;
    size_t successful_count = 0;
    size_t starts[1025];
    size_t start_count = mjb_test_codepoint_starts(buffer, byte_length, starts, 1025);
    size_t position_mismatches = 0;

    mjb_text text = mjb_text_utf8(buffer, byte_length);
    while((bt = mjb_grapheme_break_next(&text, &state)) != MJB_BT_NOT_SET) {
        snprintf(test_name, 256, "Index %zu", index);

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

    MJB_TEST_COVERAGE(mjb_grapheme_break_next);
    ATT_ASSERT(index, successful_count, test_name);
    ATT_ASSERT(position_mismatches, (size_t)0, "Break positions are codepoint starts");
}

static void test_basic_segmentation(void) {
    mjb_grapheme_break_state state;
    mjb_break_type bt = MJB_BT_NOT_SET;
    state.index = 0;
    size_t index = 0;

#define MJB_TEST_S \
    state.index = 0; \
    index = 0;
    // Read the next expected break, or MJB_BT_NOT_SET once the function returns more breaks than
    // expected, so the assertion fails instead of reading past the end of the array.
    mjb_text text = mjb_text_utf8(NULL, 1);
#define MJB_TEST_EXPECTED(array) \
    (index < sizeof(array) / sizeof((array)[0]) ? (uint8_t)(array)[index++] : \
                                                  (++index, (uint8_t)MJB_BT_NOT_SET))

    ATT_ASSERT((uint8_t)mjb_grapheme_break_next(&text, &state),
        (uint8_t)MJB_BT_NOT_SET, "Segmentation rejects NULL buffer");
    text = mjb_text_utf8("A", 1);
    ATT_ASSERT((uint8_t)mjb_grapheme_break_next(&text, NULL),
        (uint8_t)MJB_BT_NOT_SET, "Segmentation rejects NULL state");
    text = mjb_text_utf8("", 0);
    ATT_ASSERT((uint8_t)mjb_grapheme_break_next(&text, &state),
        (uint8_t)MJB_BT_NOT_SET, "Empty string");
    text = mjb_text_utf8(NULL, 1);
    ATT_ASSERT(mjb_grapheme_prefix_length(&text, 1), (size_t)0, "Truncate rejects NULL buffer");
    text = mjb_text_utf8(NULL, 1);
    ATT_ASSERT(mjb_grapheme_prefix_length_for_width(&text, MJB_TERMINAL_WIDTH_NARROW, 1), (size_t)0,
        "Truncate width rejects NULL buffer");

    MJB_TEST_S
    mjb_break_type expected_a[] = { MJB_BT_ALLOWED };

    MJB_TEST_COVERAGE(mjb_grapheme_break_next);

    text = mjb_text_utf8("A", 1);
    while((bt = mjb_grapheme_break_next(&text, &state)) != MJB_BT_NOT_SET) {
        ATT_ASSERT((uint8_t)bt, MJB_TEST_EXPECTED(expected_a), "A test");
    }

    MJB_TEST_S
    mjb_break_type expected_ab[] = { MJB_BT_ALLOWED, MJB_BT_ALLOWED };

    MJB_TEST_COVERAGE(mjb_grapheme_break_next);

    text = mjb_text_utf8("AB", 2);
    while((bt = mjb_grapheme_break_next(&text, &state)) != MJB_BT_NOT_SET) {
        ATT_ASSERT((uint8_t)bt, MJB_TEST_EXPECTED(expected_ab), "AB test");
    }

    ATT_ASSERT(index, 2, "AB test break index");

    MJB_TEST_S
    mjb_break_type expected_abc[] = { MJB_BT_ALLOWED, MJB_BT_ALLOWED, MJB_BT_ALLOWED };
    MJB_TEST_COVERAGE(mjb_grapheme_break_next);

    text = mjb_text_utf8("ABC", 3);
    while((bt = mjb_grapheme_break_next(&text, &state)) != MJB_BT_NOT_SET) {
        ATT_ASSERT((uint8_t)bt, MJB_TEST_EXPECTED(expected_abc), "AB test");
    }

    ATT_ASSERT(index, 3, "ABC test break index");

    MJB_TEST_S
    mjb_break_type expected_brnl[] = { MJB_BT_ALLOWED, MJB_BT_NO_BREAK, MJB_BT_ALLOWED,
        MJB_BT_ALLOWED };
    MJB_TEST_COVERAGE(mjb_grapheme_break_next);

    text = mjb_text_utf8("A\r\nB", 4);
    while((bt = mjb_grapheme_break_next(&text, &state)) != MJB_BT_NOT_SET) {
        ATT_ASSERT((uint8_t)bt, MJB_TEST_EXPECTED(expected_brnl), "A\\r\\nB test");
    }

    ATT_ASSERT(index, 4, "A\\r\\nB test break index");

    MJB_TEST_S
    mjb_break_type expected_itit[] = { MJB_BT_NO_BREAK, MJB_BT_ALLOWED, MJB_BT_NO_BREAK,
        MJB_BT_ALLOWED };
    MJB_TEST_COVERAGE(mjb_grapheme_break_next);

    text = mjb_text_utf8("🇮🇹🇮🇹", 16);
    while((bt = mjb_grapheme_break_next(&text, &state)) != MJB_BT_NOT_SET) {
        ATT_ASSERT((uint8_t)bt, MJB_TEST_EXPECTED(expected_itit), "ITIT test");
    }

    ATT_ASSERT(index, 4, "ITIT test break index");

    // Unicode 18 GB9c no longer requires a preceding InCB=Consonant. A Linker followed by zero or
    // more InCB=Extend characters joins directly to the following InCB=Consonant.
    const char gb9c[] = "\xE0\xA5\x8D\xCC\x80\xE0\xA4\x95"; // 094D 0300 0915
    mjb_break_type expected_gb9c[] = { MJB_BT_NO_BREAK, MJB_BT_NO_BREAK, MJB_BT_ALLOWED };

    MJB_TEST_S
    MJB_TEST_COVERAGE(mjb_grapheme_break_next);

    text = mjb_text_utf8(gb9c, sizeof(gb9c) - 1);
    while((bt = mjb_grapheme_break_next(&text, &state)) != MJB_BT_NOT_SET) {
        ATT_ASSERT((uint8_t)bt, MJB_TEST_EXPECTED(expected_gb9c), "Unicode 18 GB9c");
    }

    ATT_ASSERT(index, 3, "Unicode 18 GB9c break index");

#undef MJB_TEST_S
#undef MJB_TEST_EXPECTED
}

static void test_truncate(void) {
    // mjb_grapheme_prefix_length: empty / zero
    mjb_text text = mjb_text_utf8("", 0);
    ATT_ASSERT(mjb_grapheme_prefix_length(&text, 3), (size_t)0, "Truncate: empty string");
    text = mjb_text_utf8("ABC", 3);
    ATT_ASSERT(mjb_grapheme_prefix_length(&text, 0), (size_t)0, "Truncate: 0 graphemes");

    // ASCII: each byte is one grapheme cluster
    text = mjb_text_utf8("ABC", 3);
    ATT_ASSERT(mjb_grapheme_prefix_length(&text, 1), (size_t)1, "Truncate: ABC to 1");
    text = mjb_text_utf8("ABC", 3);
    ATT_ASSERT(mjb_grapheme_prefix_length(&text, 2), (size_t)2, "Truncate: ABC to 2");
    text = mjb_text_utf8("ABC", 3);
    ATT_ASSERT(mjb_grapheme_prefix_length(&text, 3), (size_t)3, "Truncate: ABC to 3 (no-op)");
    text = mjb_text_utf8("ABC", 3);
    ATT_ASSERT(mjb_grapheme_prefix_length(&text, 5), (size_t)3, "Truncate: ABC to 5 (no-op)");

    // Multi-byte: "aé" = 0x61 0xC3 0xA9 = 3 bytes, 2 grapheme clusters
    text = mjb_text_utf8("a\xC3\xA9", 3);
    ATT_ASSERT(mjb_grapheme_prefix_length(&text, 1), (size_t)1,
        "Truncate: aé to 1 grapheme");
    text = mjb_text_utf8("a\xC3\xA9", 3);
    ATT_ASSERT(mjb_grapheme_prefix_length(&text, 2), (size_t)3,
        "Truncate: aé to 2 graphemes (no-op)");

    // Flag emoji 🇺🇸 = two RI codepoints (4+4=8 bytes), one grapheme cluster
    text = mjb_text_utf8("\xF0\x9F\x87\xBA\xF0\x9F\x87\xB8", 8);
    ATT_ASSERT(mjb_grapheme_prefix_length(&text, 1), (size_t)8,
        "Truncate: flag emoji to 1 grapheme (no-op)");

    // mjb_grapheme_prefix_length_for_width
    text = mjb_text_utf8("", 0);
    ATT_ASSERT(mjb_grapheme_prefix_length_for_width(&text, MJB_TERMINAL_WIDTH_NARROW, 5), (size_t)0,
        "Truncate width: empty string");
    text = mjb_text_utf8("ABC", 3);
    ATT_ASSERT(mjb_grapheme_prefix_length_for_width(&text, MJB_TERMINAL_WIDTH_NARROW, 0), (size_t)0,
        "Truncate width: 0 columns");
    text = mjb_text_utf8("ABC", 3);
    ATT_ASSERT(mjb_grapheme_prefix_length_for_width(&text, MJB_TERMINAL_WIDTH_NARROW, 2), (size_t)2,
        "Truncate width: ABC to 2 columns");
    text = mjb_text_utf8("ABC", 3);
    ATT_ASSERT(mjb_grapheme_prefix_length_for_width(&text, MJB_TERMINAL_WIDTH_NARROW, 3), (size_t)3,
        "Truncate width: ABC to 3 columns (no-op)");
    text = mjb_text_utf8("ABC", 3);
    ATT_ASSERT(mjb_grapheme_prefix_length_for_width(&text, MJB_TERMINAL_WIDTH_NARROW, 10),
        (size_t)3, "Truncate width: ABC to 10 columns (no-op)");

    const char malformed_utf8_width[] = { 'a', '\x17', '\xCE', '\x08', 's', 't' };

    text = mjb_text_utf8(malformed_utf8_width, sizeof(malformed_utf8_width));
    ATT_ASSERT(mjb_grapheme_prefix_length_for_width(&text, MJB_TERMINAL_WIDTH_NARROW, 114), (size_t)1,
        "Truncate width stops before malformed or stateful terminal input");

    const char *kiss = "\xf0\x9f\x91\xa8\xf0\x9f\x8f\xbb\xe2\x80\x8d\xe2\x9d\xa4\xef\xb8\x8f"
                       "\xe2\x80\x8d\xf0\x9f\x92\x8b\xe2\x80\x8d\xf0\x9f\x91\xa8\xf0\x9f\x8f\xbb";
    text = mjb_text_utf8(kiss, strlen(kiss));
    ATT_ASSERT(mjb_grapheme_prefix_length_for_width(&text,
        MJB_TERMINAL_WIDTH_NARROW, 1), (size_t)0, "Emoji does not fit in one terminal cell");
    text = mjb_text_utf8(kiss, strlen(kiss));
    ATT_ASSERT(mjb_grapheme_prefix_length_for_width(&text,
        MJB_TERMINAL_WIDTH_NARROW, 2), strlen(kiss), "Emoji fits in two terminal cells");
}

static void test_grapheme_count(void) {
    size_t count = 12345;

    // Argument validation
    mjb_text text = mjb_text_utf8("A", 1);
    ATT_ASSERT_STATUS(mjb_grapheme_count(&text, MJB_MALFORMED_STOP, NULL, NULL),
        MJB_STATUS_INVALID_ARGUMENT, "Count rejects NULL count");
    text = mjb_text_utf8(NULL, 1);
    ATT_ASSERT_STATUS(mjb_grapheme_count(&text, MJB_MALFORMED_STOP, &count, NULL),
        MJB_STATUS_INVALID_ARGUMENT, "Count rejects NULL buffer");
    ATT_ASSERT(count, (size_t)0, "Count is zero after NULL buffer");

    count = 12345;
    text = mjb_text_make("A", 1, MJB_ENC_UNKNOWN);
    ATT_ASSERT_STATUS(mjb_grapheme_count(&text, MJB_MALFORMED_STOP, &count, NULL),
        MJB_STATUS_INVALID_ENCODING, "Count rejects invalid encoding");
    ATT_ASSERT(count, (size_t)0, "Count is zero after invalid encoding");

    // Empty input is valid and counts zero clusters
    text = mjb_text_utf8("", 0);
    ATT_ASSERT_STATUS(mjb_grapheme_count(&text, MJB_MALFORMED_STOP, &count, NULL), MJB_STATUS_OK,
        "Count: empty string status");
    ATT_ASSERT(count, (size_t)0, "Count: empty string");
    text = mjb_text_utf8(NULL, 0);
    ATT_ASSERT_STATUS(mjb_grapheme_count(&text, MJB_MALFORMED_STOP, &count, NULL), MJB_STATUS_OK,
        "Count: NULL buffer with zero length status");

    // ASCII: each byte is one grapheme cluster
    text = mjb_text_utf8("ABC", 3);
    ATT_ASSERT_STATUS(mjb_grapheme_count(&text, MJB_MALFORMED_STOP, &count, NULL), MJB_STATUS_OK,
        "Count: ABC status");
    ATT_ASSERT(count, (size_t)3, "Count: ABC");

    // "aé" = 0x61 0xC3 0xA9 = 3 bytes, 2 grapheme clusters
    text = mjb_text_utf8("a\xC3\xA9", 3);
    ATT_ASSERT_STATUS(mjb_grapheme_count(&text, MJB_MALFORMED_STOP, &count, NULL), MJB_STATUS_OK,
        "Count: aé status");
    ATT_ASSERT(count, (size_t)2, "Count: aé");

    // a + combining acute accent = one user-perceived character
    text = mjb_text_utf8("a\xCC\x81", 3);
    ATT_ASSERT_STATUS(mjb_grapheme_count(&text, MJB_MALFORMED_STOP, &count, NULL), MJB_STATUS_OK,
        "Count: a + combining acute status");
    ATT_ASSERT(count, (size_t)1, "Count: a + combining acute");

    // Flag emoji 🇺🇸 = two RI codepoints, one grapheme cluster; two flags do not pair across
    text = mjb_text_utf8("\xF0\x9F\x87\xBA\xF0\x9F\x87\xB8", 8);
    ATT_ASSERT_STATUS(mjb_grapheme_count(&text, MJB_MALFORMED_STOP,
        &count, NULL), MJB_STATUS_OK, "Count: flag emoji status");
    ATT_ASSERT(count, (size_t)1, "Count: flag emoji");
    text = mjb_text_utf8("\xF0\x9F\x87\xAE\xF0\x9F\x87\xB9"
                         "\xF0\x9F\x87\xBA\xF0\x9F\x87\xB8",
        16);
    ATT_ASSERT_STATUS(mjb_grapheme_count(&text, MJB_MALFORMED_STOP, &count, NULL), MJB_STATUS_OK,
        "Count: two flags status");
    ATT_ASSERT(count, (size_t)2, "Count: two flags");

    // Family ZWJ sequence 👨‍👩‍👦 = one grapheme cluster
    text = mjb_text_utf8("\xF0\x9F\x91\xA8\xE2\x80\x8D\xF0\x9F\x91\xA9"
                         "\xE2\x80\x8D\xF0\x9F\x91\xA6",
        18);
    ATT_ASSERT_STATUS(mjb_grapheme_count(&text, MJB_MALFORMED_STOP, &count, NULL), MJB_STATUS_OK,
        "Count: family ZWJ status");
    ATT_ASSERT(count, (size_t)1, "Count: family ZWJ sequence");

    // Hangul jamo L+V+T = one syllable cluster
    text = mjb_text_utf8("\xE1\x84\x80\xE1\x85\xA1\xE1\x86\xA8", 9);
    ATT_ASSERT_STATUS(mjb_grapheme_count(&text, MJB_MALFORMED_STOP,
        &count, NULL), MJB_STATUS_OK, "Count: Hangul jamo status");
    ATT_ASSERT(count, (size_t)1, "Count: Hangul jamo LVT syllable");

    // CRLF is a single cluster (GB3)
    text = mjb_text_utf8("a\r\nb", 4);
    ATT_ASSERT_STATUS(mjb_grapheme_count(&text, MJB_MALFORMED_STOP, &count, NULL), MJB_STATUS_OK,
        "Count: CRLF status");
    ATT_ASSERT(count, (size_t)3, "Count: a CRLF b");

    mjb_diagnostic diagnostic;
    text = mjb_text_utf8("a\x80z", 3);
    ATT_ASSERT_STATUS(mjb_grapheme_count(&text, MJB_MALFORMED_STOP,
                          &count, &diagnostic),
        MJB_STATUS_MALFORMED_INPUT, "Count rejects malformed input");
    ATT_ASSERT(count, (size_t)0, "Count is zero after malformed input");
    ATT_ASSERT(diagnostic.byte_offset, (size_t)1, "Grapheme count malformed offset");

    text = mjb_text_utf8("a\x80z", 3);
    ATT_ASSERT_STATUS(mjb_grapheme_count(&text, MJB_MALFORMED_REPLACE, &count, &diagnostic),
        MJB_STATUS_OK, "Grapheme count replaces malformed input");
    ATT_ASSERT(count, (size_t)3, "Replacement is a grapheme");
    ATT_ASSERT(diagnostic.byte_offset, (size_t)1,
        "Grapheme replacement retains malformed offset");

    text = mjb_text_utf8("a\x80z", 3);
    ATT_ASSERT_STATUS(mjb_grapheme_count(&text, MJB_MALFORMED_SKIP, &count, &diagnostic),
        MJB_STATUS_OK, "Grapheme count skips malformed input");
    ATT_ASSERT(count, (size_t)2, "Skipped malformed input is not a grapheme");

    text = mjb_text_utf8("a\xC3", 2);
    ATT_ASSERT_STATUS(mjb_grapheme_count(&text, MJB_MALFORMED_STOP, &count, NULL),
        MJB_STATUS_MALFORMED_INPUT, "Count rejects an incomplete tail");
    ATT_ASSERT(count, (size_t)0, "Count is zero after an incomplete tail");

    // Explicit lengths include embedded U+0000 codepoints
    const char embedded_nul[] = { 'A', '\0', 'B' };
    text = mjb_text_utf8(embedded_nul, 3);
    ATT_ASSERT_STATUS(mjb_grapheme_count(&text, MJB_MALFORMED_STOP, &count, NULL), MJB_STATUS_OK,
        "Count: embedded NUL status");
    ATT_ASSERT(count, (size_t)3, "Count: embedded NUL");

    // MJB_NUL_TERMINATED requests a terminator scan
    text = mjb_text_utf8("hi", MJB_NUL_TERMINATED);
    ATT_ASSERT_STATUS(mjb_grapheme_count(&text, MJB_MALFORMED_STOP, &count, NULL),
        MJB_STATUS_OK, "Count: NUL-terminated status");
    ATT_ASSERT(count, (size_t)2, "Count: NUL-terminated");

    // UTF-16LE input
    const char utf16le_ab[] = { 'A', '\0', 'B', '\0' };
    text = mjb_text_make(utf16le_ab, 4, MJB_ENC_UTF_16LE);
    ATT_ASSERT_STATUS(mjb_grapheme_count(&text, MJB_MALFORMED_STOP, &count, NULL), MJB_STATUS_OK,
        "Count: UTF-16LE status");
    ATT_ASSERT(count, (size_t)2, "Count: UTF-16LE AB");
}

static size_t grapheme_count_of(const char *buffer, size_t byte_length) {
    mjb_text text = mjb_text_utf8(buffer, byte_length);
    size_t count = 0;

    if(mjb_grapheme_count(&text, MJB_MALFORMED_REPLACE, &count, NULL) != MJB_STATUS_OK) {
        return 0;
    }

    return count;
}

static void test_grapheme_regressions(void) {
    // GB12/13: the regional indicator count restarts after a control. Two flags separated by a
    // line break used to split the second flag.
    ATT_ASSERT(grapheme_count_of("\xF0\x9F\x87\xAE\xF0\x9F\x87\xB9\n\xF0\x9F\x87\xAE\xF0\x9F\x87\xB9",
                   17),
        (size_t)3, "Flag, LF, flag");
    ATT_ASSERT(grapheme_count_of("\xF0\x9F\x87\xAE\xF0\x9F\x87\xB9\r\n\xF0\x9F\x87\xAE"
                                 "\xF0\x9F\x87\xB9",
                   18),
        (size_t)3, "Flag, CR LF, flag");
    ATT_ASSERT(grapheme_count_of("\xF0\x9F\x87\xAE\xF0\x9F\x87\xB9\x01\xF0\x9F\x87\xAE"
                                 "\xF0\x9F\x87\xB9",
                   17),
        (size_t)3, "Flag, control, flag");
    ATT_ASSERT(grapheme_count_of("\xF0\x9F\x87\xAE\xF0\x9F\x87\xB9\n\xF0\x9F\x87\xAE\xF0\x9F\x87\xB9"
                                 "\xF0\x9F\x87\xAE\xF0\x9F\x87\xB9",
                   25),
        (size_t)4, "Flag, LF, two flags");

    // GB11: ZWJ must immediately precede the Extended_Pictographic. U+1F468 MAN, U+200D ZWJ,
    // U+0308 COMBINING DIAERESIS.
    ATT_ASSERT(grapheme_count_of("\xF0\x9F\x91\xA8\xE2\x80\x8D\xF0\x9F\x91\xA8", 11), (size_t)1,
        "ExtPict ZWJ ExtPict");
    ATT_ASSERT(grapheme_count_of("\xF0\x9F\x91\xA8\xCC\x88\xE2\x80\x8D\xF0\x9F\x91\xA8", 13),
        (size_t)1, "ExtPict Extend ZWJ ExtPict");
    ATT_ASSERT(grapheme_count_of("\xF0\x9F\x91\xA8\xE2\x80\x8D\xCC\x88\xF0\x9F\x91\xA8", 13),
        (size_t)2, "ExtPict ZWJ Extend ExtPict");
    ATT_ASSERT(grapheme_count_of("\xF0\x9F\x91\xA8\xE2\x80\x8D\xE2\x80\x8D\xF0\x9F\x91\xA8", 14),
        (size_t)2, "ExtPict ZWJ ZWJ ExtPict");
}

typedef struct grapheme_props {
    int32_t gcb;
    int32_t incb;
    bool ext_pict;
} grapheme_props;

static grapheme_props grapheme_props_of(mjb_codepoint codepoint) {
    grapheme_props props;
    int32_t value = 0;
    bool binary = false;

    props.gcb = MJB_GBP_OTHER;
    props.incb = MJB_INCB_NOT_SET;
    props.ext_pict = false;

    if(mjb_codepoint_property_int(codepoint, MJB_PR_GRAPHEME_CLUSTER_BREAK, &value) ==
            MJB_STATUS_OK &&
        value != MJB_GBP_NOT_SET) {
        props.gcb = value;
    }

    if(mjb_codepoint_property_int(codepoint, MJB_PR_INDIC_CONJUNCT_BREAK, &value) ==
        MJB_STATUS_OK) {
        props.incb = value;
    }

    if(mjb_codepoint_property_binary(codepoint, MJB_PR_EXTENDED_PICTOGRAPHIC, &binary) ==
        MJB_STATUS_OK) {
        props.ext_pict = binary;
    }

    return props;
}

static bool grapheme_is_control(int32_t gcb) {
    return gcb == MJB_GBP_CONTROL || gcb == MJB_GBP_CR || gcb == MJB_GBP_LF;
}

// True when there is a grapheme boundary between b and c whatever precedes b. The rules that need
// more context (GB9c, GB11, GB12/13) can only join c to b in the three pairs excluded at the end.
static bool grapheme_is_safe_break(grapheme_props b, grapheme_props c) {
    if(b.gcb == MJB_GBP_CR && c.gcb == MJB_GBP_LF) {
        return false;
    }

    if(grapheme_is_control(b.gcb) || grapheme_is_control(c.gcb)) {
        return true;
    }

    if((b.gcb == MJB_GBP_L &&
           (c.gcb == MJB_GBP_L || c.gcb == MJB_GBP_V || c.gcb == MJB_GBP_LV ||
               c.gcb == MJB_GBP_LVT)) ||
        ((b.gcb == MJB_GBP_LV || b.gcb == MJB_GBP_V) &&
            (c.gcb == MJB_GBP_V || c.gcb == MJB_GBP_T)) ||
        ((b.gcb == MJB_GBP_LVT || b.gcb == MJB_GBP_T) && c.gcb == MJB_GBP_T) ||
        c.gcb == MJB_GBP_EXTEND || c.gcb == MJB_GBP_ZWJ || c.gcb == MJB_GBP_SPACING_MARK ||
        b.gcb == MJB_GBP_PREPEND) {
        return false;
    }

    return !((c.incb == MJB_INCB_CONSONANT &&
                 (b.incb == MJB_INCB_LINKER || b.incb == MJB_INCB_EXTEND)) ||
        (c.ext_pict && b.gcb == MJB_GBP_ZWJ) ||
        (c.gcb == MJB_GBP_REGIONAL_INDICATOR && b.gcb == MJB_GBP_REGIONAL_INDICATOR));
}

// Find the last grapheme boundary before offset by walking back to a safe boundary with
// mjb_decode_previous, then replaying the forward iterator from there. It shares no state with the
// forward iterator, so the two must agree.
static size_t grapheme_previous_boundary(const char *buffer, size_t byte_length, size_t offset) {
    mjb_text text = mjb_text_utf8(buffer, byte_length);
    size_t position = offset;
    size_t safe = 0;
    mjb_codepoint codepoint;

    if(mjb_decode_previous(&text, MJB_MALFORMED_REPLACE, &position, &codepoint, NULL) !=
        MJB_STATUS_OK) {
        return 0;
    }

    grapheme_props c = grapheme_props_of(codepoint);

    while(position > 0) {
        size_t before = position;

        if(mjb_decode_previous(&text, MJB_MALFORMED_REPLACE, &before, &codepoint, NULL) !=
            MJB_STATUS_OK) {
            break;
        }

        grapheme_props b = grapheme_props_of(codepoint);

        if(grapheme_is_safe_break(b, c)) {
            safe = position;
            break;
        }

        position = before;
        c = b;
    }

    mjb_text tail = mjb_text_utf8(buffer + safe, byte_length - safe);
    mjb_grapheme_break_state state;
    state.index = 0;
    size_t last = safe;
    mjb_break_type bt;

    while((bt = mjb_grapheme_break_next(&tail, &state)) != MJB_BT_NOT_SET &&
        safe + state.position < offset) {
        if(bt != MJB_BT_NO_BREAK) {
            last = safe + state.position;
        }
    }

    return last;
}

static uint32_t grapheme_random_next(uint32_t *seed) {
    // xorshift32: deterministic on every platform.
    uint32_t x = *seed;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *seed = x;

    return x;
}

// Compare the forward iterator with an independent backward search on random strings built from
// codepoints that exercise every rule, plus malformed bytes.
static void test_grapheme_differential(void) {
    static const mjb_codepoint pool[] = { 'a', 0x0D, 0x0A, 0x01, 0x1100, 0x1161, 0x11A8, 0xAC00,
        0xAC01, 0x0600, 0x0903, 0x0300, 0x200D, 0x1F468, 0x1F3FB, 0x2764, 0xFE0F, 0x1F1EE, 0x1F1F9,
        0x0915, 0x094D, 0x0924, 0x093C, 0xFFFD };
    // A lone continuation byte and two truncated sequences, with explicit sizes.
    static const char malformed[3][2] = { { '\x80', 0 }, { '\xE2', '\x82' }, { '\xF0', '\x9F' } };
    static const size_t malformed_sizes[3] = { 1, 2, 2 };
    const size_t pool_size = sizeof(pool) / sizeof(pool[0]);
    uint32_t seed = 0x6D6A6221;
    size_t mismatches = 0;
    size_t checked = 0;

    for(size_t round = 0; round < 20000; ++round) {
        char buffer[256];
        size_t length = 0;
        size_t items = (size_t)(grapheme_random_next(&seed) % 32) + 1;

        for(size_t i = 0; i < items; ++i) {
            uint32_t pick = grapheme_random_next(&seed) % (uint32_t)(pool_size + 3);

            if(pick < pool_size) {
                length += mjb_codepoint_encode(pool[pick], buffer + length, 4, MJB_ENC_UTF_8);
            } else {
                size_t which = pick - pool_size;

                for(size_t k = 0; k < malformed_sizes[which]; ++k) {
                    buffer[length++] = malformed[which][k];
                }
            }
        }

        // Forward: every codepoint position, and which of them are boundaries.
        size_t positions[256];
        bool is_boundary[256];
        size_t position_count = 1;
        positions[0] = 0;
        is_boundary[0] = true;

        mjb_text text = mjb_text_utf8(buffer, length);
        mjb_grapheme_break_state state;
        state.index = 0;
        mjb_break_type bt;

        while((bt = mjb_grapheme_break_next(&text, &state)) != MJB_BT_NOT_SET &&
            position_count < 256) {
            positions[position_count] = state.position;
            is_boundary[position_count++] = bt != MJB_BT_NO_BREAK;
        }

        // Backward: the last boundary before every position must match.
        size_t expected = 0;

        for(size_t i = 1; i < position_count; ++i) {
            if(grapheme_previous_boundary(buffer, length, positions[i]) != expected) {
                ++mismatches;
            }

            if(is_boundary[i]) {
                expected = positions[i];
            }

            ++checked;
        }
    }

    ATT_ASSERT(checked > 0, true, "Grapheme differential checked positions");
    ATT_ASSERT(mismatches, (size_t)0, "Grapheme forward and backward boundaries agree");
}

ATT_TEST(segmentation) {
    {
        MJB_TEST_COVERAGE(mjb_grapheme_break_next);

        // state.position is the byte offset of each reported position. Each maximal subpart of a
        // malformed subsequence decodes to one U+FFFD and counts as one position, both in the
        // middle of the text and in an incomplete sequence at its end.
        const size_t expected_middle[] = { 1, 2, 3, 4, 5 };
        mjb_text middle = mjb_text_utf8("ab\x80"
                                        "cd",
            5);
        mjb_grapheme_break_state state;
        state.index = 0;
        size_t count = 0;
        bool positions_match = true;

        while(mjb_grapheme_break_next(&middle, &state) != MJB_BT_NOT_SET) {
            positions_match = positions_match && count < 5 &&
                state.position == expected_middle[count];
            ++count;
        }

        ATT_ASSERT(count, (size_t)5, "Grapheme positions around a malformed byte");
        ATT_ASSERT(positions_match, true, "Grapheme positions skip nothing around a malformed byte");

        mjb_text tail = mjb_text_utf8("ab\xE2\x82", 4);
        state.index = 0;
        size_t last_position = 0;
        count = 0;

        while(mjb_grapheme_break_next(&tail, &state) != MJB_BT_NOT_SET) {
            last_position = state.position;
            ++count;
        }

        // b ÷ U+FFFD at 2, then U+FFFD ÷ eot at 4: the incomplete tail E2 82 is one subpart.
        ATT_ASSERT(count, (size_t)3, "Grapheme iteration covers an incomplete tail");
        ATT_ASSERT(last_position, (size_t)4, "Grapheme last position follows an incomplete tail");

        // The first cluster ends before the malformed run, at its exact start offset.
        mjb_text run = mjb_text_utf8("a\x80\x80"
                                     "b",
            4);
        ATT_ASSERT(mjb_grapheme_prefix_length(&run, 1), (size_t)1,
            "Grapheme prefix ends before a malformed run");
        ATT_ASSERT(mjb_grapheme_prefix_length(&run, 2), (size_t)2,
            "Grapheme prefix: each malformed subpart is a cluster");

        // An unknown encoding cannot be segmented.
        mjb_text unknown = mjb_text_make("ab", 2, MJB_ENC_UNKNOWN);
        state.index = 0;
        ATT_ASSERT((int)mjb_grapheme_break_next(&unknown, &state), (int)MJB_BT_NOT_SET,
            "Grapheme break rejects an unknown encoding");
        ATT_ASSERT(mjb_grapheme_prefix_length(&unknown, 1), (size_t)0,
            "Grapheme prefix of an unknown encoding is empty");
    }

    {
        mjb_grapheme_break_state state;
        state.index = 0;
        size_t count = 9;
        ATT_ASSERT((int)mjb_grapheme_break_next(NULL, &state), (int)MJB_BT_NOT_SET,
            "Grapheme break rejects NULL text");
        ATT_ASSERT(mjb_grapheme_prefix_length(NULL, 1), (size_t)0,
            "Truncate grapheme rejects NULL text");
        ATT_ASSERT(mjb_grapheme_prefix_length_for_width(NULL, MJB_TERMINAL_WIDTH_NARROW, 1), (size_t)0,
            "Truncate grapheme width rejects NULL text");
        ATT_ASSERT_STATUS(mjb_grapheme_count(NULL, MJB_MALFORMED_STOP, &count, NULL),
            MJB_STATUS_INVALID_ARGUMENT, "Grapheme count rejects NULL text");
        ATT_ASSERT(count, (size_t)0, "Grapheme count clears count after NULL text");
    }

    test_basic_segmentation();
    test_truncate();
    test_grapheme_count();
    test_grapheme_regressions();
    test_grapheme_differential();
    read_test_file("./unicode-data/UCD/auxiliary/GraphemeBreakTest.txt", &segmentation_callback);
}
