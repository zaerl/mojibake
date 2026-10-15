/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "test.h"

static void assert_filter_into_matches(const char *input, size_t input_size, mjb_encoding encoding,
    mjb_filter_flags filters, mjb_encoding output_encoding, const char *message) {
    mjb_result allocated;

    mjb_text text = mjb_text_make(input, input_size, encoding);
    ATT_ASSERT_STATUS(mjb_filter(&text, MJB_MALFORMED_REPLACE, filters,
                          output_encoding, &allocated, NULL),
        MJB_STATUS_OK, message);

    mjb_buffer into = mjb_buffer_make(NULL, 0, output_encoding);
    ATT_ASSERT_STATUS(mjb_filter_into(&text, MJB_MALFORMED_REPLACE, filters, &into, NULL),
        MJB_STATUS_OK, message);
    ATT_ASSERT(into.size, allocated.output_size, message);

    unsigned char output[256];
    ATT_ASSERT(into.size <= sizeof(output), true, message);
    into = mjb_buffer_make(output, sizeof(output), output_encoding);
    ATT_ASSERT_STATUS(mjb_filter_into(&text, MJB_MALFORMED_REPLACE, filters, &into, NULL),
        MJB_STATUS_OK, message);
    ATT_ASSERT(into.size, allocated.output_size, message);
    ATT_ASSERT(memcmp(output, allocated.output, into.size), 0, message);

    mjb_result_free(&allocated);
}

ATT_TEST(filter) {
    // Normalizing input made only of skipped malformed bytes leaves nothing to filter. The filter
    // output must not request a zero-size allocation for it.
    {
        mjb_result skipped = { NULL, 0, 0, false };
        mjb_text malformed = mjb_text_utf8("\x80", 1);
        ATT_ASSERT_STATUS(mjb_filter(&malformed, MJB_MALFORMED_SKIP,
                              (mjb_filter_flags)(MJB_FILTER_NORMALIZE | MJB_FILTER_SPACES),
                              MJB_ENC_UTF_8, &skipped, NULL),
            MJB_STATUS_OK, "Filter input with only skipped malformed bytes");
        ATT_ASSERT(skipped.output_size, (size_t)0,
            "Filter input with only skipped malformed bytes is empty");
        ATT_ASSERT_STATUS(mjb_result_free(&skipped), MJB_STATUS_OK,
            "Free filter result of input with only skipped malformed bytes");
    }

    mjb_result result;
    mjb_encoding enc = MJB_ENC_UTF_8;

    MJB_TEST_COVERAGE(mjb_filter_into);

    mjb_text into_text = mjb_text_make(NULL, 1, enc);
    mjb_buffer into = mjb_buffer_make(NULL, 9, enc);

    ATT_ASSERT_STATUS(mjb_filter_into(&into_text, MJB_MALFORMED_REPLACE, MJB_FILTER_NONE, &into,
                          NULL),
        MJB_STATUS_INVALID_ARGUMENT, "Filter into rejects NULL buffer");
    ATT_ASSERT(into.size, (size_t)0, "Filter into resets size for invalid input");
    into = mjb_buffer_make(NULL, 9, enc);
    ATT_ASSERT_STATUS(mjb_filter_into(NULL, MJB_MALFORMED_REPLACE, MJB_FILTER_NONE, &into, NULL),
        MJB_STATUS_INVALID_ARGUMENT, "Filter into rejects NULL text");
    ATT_ASSERT(into.size, (size_t)0, "Filter into resets size for NULL text");
    into_text = mjb_text_make("A", 1, enc);
    ATT_ASSERT_STATUS(mjb_filter_into(&into_text, MJB_MALFORMED_REPLACE, MJB_FILTER_NONE, NULL,
                          NULL),
        MJB_STATUS_INVALID_ARGUMENT, "Filter into rejects NULL output");
    into = mjb_buffer_make(NULL, 9, enc);
    ATT_ASSERT_STATUS(mjb_filter_into(&into_text, (mjb_malformed_policy)99, MJB_FILTER_NONE,
                          &into, NULL),
        MJB_STATUS_INVALID_ARGUMENT, "Filter into rejects invalid malformed policy");
    ATT_ASSERT(into.size, (size_t)0, "Invalid filter policy resets output size");

    const char *into_input = "Hello\t\t\nworld";
    const char *into_expected = "Hello world";
    into_text = mjb_text_make(into_input, strlen(into_input), enc);
    into = mjb_buffer_make(NULL, 0, enc);

    ATT_ASSERT_STATUS(mjb_filter_into(&into_text, MJB_MALFORMED_REPLACE, MJB_FILTER_COLLAPSE_SPACES,
                          &into, NULL),
        MJB_STATUS_OK, "Query filtered output size");
    ATT_ASSERT(into.size, strlen(into_expected), "Filtered output required payload size");

    unsigned char into_output[12];
    unsigned char untouched_output[sizeof(into_output)];
    memset(into_output, 0xA5, sizeof(into_output));
    memset(untouched_output, 0xA5, sizeof(untouched_output));
    size_t small_capacity = strlen(into_expected) - 1;
    into = mjb_buffer_make(into_output, small_capacity, enc);

    ATT_ASSERT_STATUS(mjb_filter_into(&into_text, MJB_MALFORMED_REPLACE, MJB_FILTER_COLLAPSE_SPACES,
                          &into, NULL),
        MJB_STATUS_OUTPUT_TOO_SMALL, "Filter into reports a small output buffer");
    ATT_ASSERT(into.size, strlen(into_expected), "Small filter output reports required size");
    ATT_ASSERT(memcmp(into_output + small_capacity, untouched_output + small_capacity,
                   sizeof(into_output) - small_capacity),
        0, "Small filter output is not written past its capacity");

    into = mjb_buffer_make(into_output, strlen(into_expected), enc);
    ATT_ASSERT_STATUS(mjb_filter_into(&into_text, MJB_MALFORMED_REPLACE, MJB_FILTER_COLLAPSE_SPACES,
                          &into, NULL),
        MJB_STATUS_OK, "Filter into exact-size output buffer");
    ATT_ASSERT(into.size, strlen(into_expected), "Filter into written payload size");
    ATT_ASSERT(memcmp(into_output, into_expected, into.size), 0, "Filter into output bytes");
    ATT_ASSERT((unsigned int)into_output[into.size], 0xA5u,
        "Filter into does not write a terminator");

    const char *normalize_input = "Cafe\xCC\x81";
    const char *normalize_expected = "Caf\xC3\xA9";
    into_text = mjb_text_make(normalize_input, strlen(normalize_input), enc);
    into = mjb_buffer_make(into_output, sizeof(into_output), enc);

    ATT_ASSERT_STATUS(mjb_filter_into(&into_text, MJB_MALFORMED_REPLACE,
                          (mjb_filter_flags)(MJB_FILTER_NORMALIZE | MJB_FILTER_SPACES), &into,
                          NULL),
        MJB_STATUS_OK, "Filter into normalizes before filtering");
    ATT_ASSERT(into.size, strlen(normalize_expected), "Normalized filter into size");
    ATT_ASSERT(memcmp(into_output, normalize_expected, into.size), 0,
        "Normalized filter into output");

    // The NFC-only path passes the output buffer straight to mjb_normalize_into
    into = mjb_buffer_make(into_output, sizeof(into_output), MJB_ENC_UTF_16LE);
    ATT_ASSERT_STATUS(mjb_filter_into(&into_text, MJB_MALFORMED_REPLACE, MJB_FILTER_NORMALIZE,
                          &into, NULL),
        MJB_STATUS_OK, "Filter into normalizes into another encoding");
    ATT_ASSERT(into.size, (size_t)8, "Normalized UTF-16LE filter into size");
    ATT_ASSERT(memcmp(into_output, "C\0a\0f\0\xE9\0", 8), 0,
        "Normalized UTF-16LE filter into output");

    assert_filter_into_matches("\x1\x2\tA\x1F", 5, enc, MJB_FILTER_CONTROLS, enc,
        "Filter into controls matches allocating filter");
    assert_filter_into_matches("\xD9\xA1\xD9\xA2", 4, enc, MJB_FILTER_NUMERIC, enc,
        "Filter into numeric mapping matches allocating filter");
    assert_filter_into_matches("a\xCC\x80\xCC\x81\xCC\x82\xCC\x83\xCC\x84", 11, enc,
        MJB_FILTER_LIMIT_COMBINING, enc, "Filter into combining limit matches allocating filter");
    assert_filter_into_matches("A\xC0"
                               "B",
        3, enc, MJB_FILTER_NONE, enc,
        "Filter into malformed replacement matches allocating filter");
    assert_filter_into_matches("A", 1, enc, MJB_FILTER_NONE, MJB_ENC_UTF_16LE,
        "Filter into encoding conversion matches allocating filter");
    assert_filter_into_matches(normalize_input, strlen(normalize_input), enc, MJB_FILTER_NORMALIZE,
        enc, "Filter into normalization matches allocating filter");

    mjb_text text = mjb_text_make(NULL, 1, enc);
    ATT_ASSERT_STATUS(mjb_filter(&text, MJB_MALFORMED_REPLACE, MJB_FILTER_NONE, enc, &result, NULL),
        MJB_STATUS_INVALID_ARGUMENT, "Filter rejects NULL buffer");
    text = mjb_text_make("", 0, enc);
    ATT_ASSERT_STATUS(mjb_filter(&text, MJB_MALFORMED_REPLACE, MJB_FILTER_NONE, enc, NULL, NULL),
        MJB_STATUS_INVALID_ARGUMENT, "Filter rejects NULL result");

    text = mjb_text_make("", 0, enc);
    ATT_ASSERT_STATUS(mjb_filter(&text, MJB_MALFORMED_REPLACE, MJB_FILTER_NONE, enc, &result, NULL), MJB_STATUS_OK,
        "Filter empty string");
    ATT_ASSERT(result.output, (char *)"", "Filter empty string output");
    ATT_ASSERT(result.output_size, 0, "Filter empty string size");
    ATT_ASSERT(result.owned, false, "Filter empty string owned");
    mjb_result_free(&result);

    text = mjb_text_make("", 0, enc);
    ATT_ASSERT_STATUS(mjb_filter(&text, MJB_MALFORMED_REPLACE, MJB_FILTER_NORMALIZE, enc, &result, NULL),
        MJB_STATUS_OK, "Filter normalize empty string");
    ATT_ASSERT(result.output, (char *)"", "Filter normalize empty string output");
    ATT_ASSERT(result.output_size, 0, "Filter normalize empty string size");
    ATT_ASSERT(result.owned, false, "Filter normalize empty string owned");
    mjb_result_free(&result);

    text = mjb_text_make("   ", 0, enc);
    ATT_ASSERT_STATUS(mjb_filter(&text, MJB_MALFORMED_REPLACE, MJB_FILTER_SPACES, enc, &result, NULL),
        MJB_STATUS_OK, "Filter spaces");
    ATT_ASSERT(result.output, (char *)"   ", "Filter spaces output");
    mjb_result_free(&result);

    text = mjb_text_make("A", 1, enc);
    ATT_ASSERT_STATUS(mjb_filter(&text, MJB_MALFORMED_REPLACE, MJB_FILTER_NONE, MJB_ENC_UTF_16LE, &result, NULL),
        MJB_STATUS_OK, "Filter converts output encoding without other changes");
    ATT_ASSERT(result.owned, true, "Filter output encoding conversion owned");
    ATT_ASSERT(result.output_size, (size_t)2, "Filter output encoding conversion size");
    ATT_ASSERT((int)memcmp(result.output, "A\0", 2), 0, "Filter output encoding conversion bytes");
    mjb_result_free(&result);

    text = mjb_text_make("A", 1, enc);
    ATT_ASSERT_STATUS(mjb_filter(&text, MJB_MALFORMED_REPLACE, MJB_FILTER_NORMALIZE, MJB_ENC_UTF_16LE,
                          &result, NULL),
        MJB_STATUS_OK, "Filter normalize converts output encoding");
    ATT_ASSERT(result.owned, true, "Filter normalize output encoding owned");
    ATT_ASSERT(result.output_size, (size_t)2, "Filter normalize output encoding size");
    ATT_ASSERT((int)memcmp(result.output, "A\0", 2), 0, "Filter normalize output encoding bytes");
    mjb_result_free(&result);

    text = mjb_text_make("\xC3\xA9", 2, enc);
    ATT_ASSERT_STATUS(mjb_filter(&text, MJB_MALFORMED_REPLACE, MJB_FILTER_NONE, MJB_ENC_ASCII,
                          &result, NULL),
        MJB_STATUS_UNSUPPORTED, "Filter rejects unrepresentable ASCII output");
    text = mjb_text_make("A", 1, enc);
    ATT_ASSERT_STATUS(mjb_filter(&text, MJB_MALFORMED_REPLACE, MJB_FILTER_NONE, MJB_ENC_UTF_32,
                          &result, NULL),
        MJB_STATUS_INVALID_ENCODING, "Filter rejects output without byte order");

    // clang-format off
    const char *spaces = "\x20" // U+0020 SPACE
        "\xC2\xA0"      // U+00A0 NO-BREAK SPACE
        "\xE1\x9A\x80"  // U+1680 OGHAM SPACE MARK
        "\xE2\x80\x80"  // U+2000 EN QUAD
        "\xE2\x80\x81"  // U+2001 EM QUAD
        "\xE2\x80\x82"  // U+2002 EN SPACE
        "\xE2\x80\x83"  // U+2003 EM SPACE
        "\xE2\x80\x84"  // U+2004 THREE-PER-EM SPACE
        "\xE2\x80\x85"  // U+2005 FOUR-PER-EM SPACE
        "\xE2\x80\x86"  // U+2006 SIX-PER-EM SPACE
        "\xE2\x80\x87"  // U+2007 FIGURE SPACE
        "\xE2\x80\x88"  // U+2008 PUNCTUATION SPACE
        "\xE2\x80\x89"  // U+2009 THIN SPACE
        "\xE2\x80\x8A"  // U+200A HAIR SPACE
        "\xE2\x80\xAF"  // U+202F NARROW NO-BREAK SPACE
        "\xE2\x81\x9F"  // U+205F MEDIUM MATHEMATICAL SPACE
        "\xE3\x80\x80"; // U+3000 IDEOGRAPHIC SPACE
    // clang-format on

    text = mjb_text_make(spaces, strlen(spaces), enc);

    ATT_ASSERT_STATUS(mjb_filter(&text, MJB_MALFORMED_REPLACE, MJB_FILTER_SPACES, enc,
                          &result, NULL),
        MJB_STATUS_OK, "Filter spaces");
    ATT_ASSERT(result.output, (char *)"                 ", "Filter spaces output");
    ATT_ASSERT(result.output_size, 17, "Filter spaces output size");
    mjb_result_free(&result);

    text = mjb_text_make(spaces, strlen(spaces), enc);
    ATT_ASSERT_STATUS(mjb_filter(&text, MJB_MALFORMED_REPLACE,
        (mjb_filter_flags)(MJB_FILTER_NORMALIZE | MJB_FILTER_SPACES), enc, &result, NULL), MJB_STATUS_OK,
        "Filter spaces and normalize");
    ATT_ASSERT(result.output, (char *)"                 ", "Filter spaces and normalize output");
    ATT_ASSERT(result.output_size, 17, "Filter spaces and normalize output size");
    mjb_result_free(&result);

    // Test whitespace collapsing with consecutive spaces
    const char *multiple_spaces = "hello    world";
    text = mjb_text_make(multiple_spaces, strlen(multiple_spaces), enc);
    ATT_ASSERT_STATUS(mjb_filter(&text, MJB_MALFORMED_REPLACE,
        MJB_FILTER_COLLAPSE_SPACES, enc, &result, NULL), MJB_STATUS_OK, "Collapse multiple spaces");
    ATT_ASSERT(result.output, (char *)"hello world", "Collapse multiple spaces output");
    ATT_ASSERT(result.output_size, 11, "Collapse multiple spaces output size");
    mjb_result_free(&result);

    // Test whitespace collapsing with tabs and newlines
    const char *mixed_whitespace = "hello\t\t\n\nworld";
    text = mjb_text_make(mixed_whitespace, strlen(mixed_whitespace), enc);
    ATT_ASSERT_STATUS(mjb_filter(&text, MJB_MALFORMED_REPLACE,
        MJB_FILTER_COLLAPSE_SPACES, enc, &result, NULL), MJB_STATUS_OK, "Collapse mixed whitespace");
    ATT_ASSERT(result.output, (char *)"hello world", "Collapse mixed whitespace output");
    ATT_ASSERT(result.output_size, 11, "Collapse mixed whitespace output size");
    mjb_result_free(&result);

    // Test whitespace collapsing with leading whitespace
    const char *leading_whitespace = "   hello world";
    text = mjb_text_make(leading_whitespace, strlen(leading_whitespace), enc);
    ATT_ASSERT_STATUS(mjb_filter(&text, MJB_MALFORMED_REPLACE,
        MJB_FILTER_COLLAPSE_SPACES, enc, &result, NULL), MJB_STATUS_OK, "Collapse leading whitespace");
    ATT_ASSERT(result.output, (char *)" hello world", "Collapse leading whitespace output");
    ATT_ASSERT(result.output_size, 12, "Collapse leading whitespace output size");
    mjb_result_free(&result);

    // Test whitespace collapsing with trailing whitespace
    const char *trailing_whitespace = "hello world   ";
    text = mjb_text_make(trailing_whitespace, strlen(trailing_whitespace), enc);
    ATT_ASSERT_STATUS(mjb_filter(&text, MJB_MALFORMED_REPLACE,
        MJB_FILTER_COLLAPSE_SPACES, enc, &result, NULL), MJB_STATUS_OK, "Collapse trailing whitespace");
    ATT_ASSERT(result.output, (char *)"hello world ", "Collapse trailing whitespace output");
    ATT_ASSERT(result.output_size, 12, "Collapse trailing whitespace output size");
    mjb_result_free(&result);

    // Test whitespace collapsing combined with MJB_FILTER_SPACES
    const char
        *unicode_multiple_spaces = "hello"
                                   "\xE2\x80\x80\xE2\x80\x81" // U+2000 EN QUAD, U+2001 EM QUAD
                                   "world";
    text = mjb_text_make(unicode_multiple_spaces, strlen(unicode_multiple_spaces), enc);
    ATT_ASSERT_STATUS(mjb_filter(&text, MJB_MALFORMED_REPLACE, (mjb_filter_flags)(MJB_FILTER_SPACES | MJB_FILTER_COLLAPSE_SPACES), enc, &result, NULL),
        MJB_STATUS_OK, "Collapse Unicode spaces");
    ATT_ASSERT(result.output, (char *)"hello world", "Collapse Unicode spaces output");
    ATT_ASSERT(result.output_size, 11, "Collapse Unicode spaces output size");
    mjb_result_free(&result);

    // Test whitespace collapsing with complex mixed whitespace
    const char *complex_whitespace = "one  \t\n  two\r\n\r\nthree    four";
    text = mjb_text_make(complex_whitespace, strlen(complex_whitespace), enc);
    ATT_ASSERT_STATUS(mjb_filter(&text, MJB_MALFORMED_REPLACE,
        MJB_FILTER_COLLAPSE_SPACES, enc, &result, NULL), MJB_STATUS_OK, "Collapse complex whitespace");
    ATT_ASSERT(result.output, (char *)"one two three four", "Collapse complex whitespace output");
    ATT_ASSERT(result.output_size, 18, "Collapse complex whitespace output size");
    mjb_result_free(&result);

    // Test whitespace collapsing with only whitespace
    const char *only_whitespace = "  \t\n  ";
    text = mjb_text_make(only_whitespace, strlen(only_whitespace), enc);
    ATT_ASSERT_STATUS(mjb_filter(&text, MJB_MALFORMED_REPLACE,
        MJB_FILTER_COLLAPSE_SPACES, enc, &result, NULL), MJB_STATUS_OK, "Collapse only whitespace");
    ATT_ASSERT(result.output, (char *)" ", "Collapse only whitespace output");
    ATT_ASSERT(result.output_size, 1, "Collapse only whitespace output size");
    mjb_result_free(&result);

    // Test no collapsing when there's no consecutive whitespace
    const char *single_spaces = "hello world test";
    text = mjb_text_make(single_spaces, strlen(single_spaces), enc);
    ATT_ASSERT_STATUS(mjb_filter(&text, MJB_MALFORMED_REPLACE,
        MJB_FILTER_COLLAPSE_SPACES, enc, &result, NULL), MJB_STATUS_OK, "No collapse needed");
    ATT_ASSERT(result.output, (char *)"hello world test", "No collapse needed output");
    ATT_ASSERT(result.output_size, 16, "No collapse needed output size");
    mjb_result_free(&result);

    const char *controls = "\x1\x2\t\n\v\f\r\x1f";
    text = mjb_text_make(controls, strlen(controls), enc);
    ATT_ASSERT_STATUS(mjb_filter(&text, MJB_MALFORMED_REPLACE, MJB_FILTER_CONTROLS, enc,
        &result, NULL), MJB_STATUS_OK, "Filter controls");
    ATT_ASSERT(result.output, (char *)"\t\n\v\f\r", "Filter controls output");
    ATT_ASSERT(result.output_size, 5, "Filter controls output size");
    ATT_ASSERT(result.owned, true, "Filter controls owned");
    mjb_result_free(&result);

    const char *numeric = "1234567890";
    text = mjb_text_make(numeric, strlen(numeric), enc);
    ATT_ASSERT_STATUS(mjb_filter(&text, MJB_MALFORMED_REPLACE, MJB_FILTER_NUMERIC, enc,
        &result, NULL), MJB_STATUS_OK, "Filter numeric");
    ATT_ASSERT(result.output, (char *)"1234567890", "Filter numeric output");
    ATT_ASSERT(result.output_size, 10, "Filter numeric output size");
    ATT_ASSERT(result.owned, false, "Filter numeric owned");
    mjb_result_free(&result);

    // U+0661 ARABIC-INDIC DIGIT ONE, U+0662 ARABIC-INDIC DIGIT TWO
    const char *arabic_indic_digit = "\xD9\xA1\xD9\xA2";
    text = mjb_text_make(arabic_indic_digit, strlen(arabic_indic_digit), enc);
    ATT_ASSERT_STATUS(mjb_filter(&text, MJB_MALFORMED_REPLACE,
        MJB_FILTER_NUMERIC, enc, &result, NULL), MJB_STATUS_OK, "Filter arabic indic digit");
    ATT_ASSERT(result.output, (char *)"12", "Filter arabic indic digit output");
    ATT_ASSERT(result.output_size, 2, "Filter arabic indic digit output size");
    ATT_ASSERT(result.owned, true, "Filter arabic indic digit owned");
    mjb_result_free(&result);

    const char *ordinary_combining = "Cafe\xCC\x81"; // Cafe + U+0301 COMBINING ACUTE ACCENT
    text = mjb_text_make(ordinary_combining, strlen(ordinary_combining), enc);
    ATT_ASSERT_STATUS(mjb_filter(&text, MJB_MALFORMED_REPLACE,
        MJB_FILTER_LIMIT_COMBINING, enc, &result, NULL), MJB_STATUS_OK, "Limit combining ordinary accent");
    ATT_ASSERT(result.output, (char *)ordinary_combining, "Limit combining ordinary accent output");
    ATT_ASSERT(result.output_size, strlen(ordinary_combining),
        "Limit combining ordinary accent size");
    ATT_ASSERT(result.owned, false, "Limit combining ordinary accent owned");
    mjb_result_free(&result);

    // clang-format off
    const char *stacked_combining = "a"
        "\xCC\x80" // U+0300 COMBINING GRAVE ACCENT
        "\xCC\x81" // U+0301 COMBINING ACUTE ACCENT
        "\xCC\x82" // U+0302 COMBINING CIRCUMFLEX ACCENT
        "\xCC\x83" // U+0303 COMBINING TILDE
        "\xCC\x84" // U+0304 COMBINING MACRON
        "\xCC\x85" // U+0305 COMBINING OVERLINE
        "b";
    const char *expected_stacked_combining = "a"
        "\xCC\x80" // U+0300 COMBINING GRAVE ACCENT
        "\xCC\x81" // U+0301 COMBINING ACUTE ACCENT
        "\xCC\x82" // U+0302 COMBINING CIRCUMFLEX ACCENT
        "\xCC\x83" // U+0303 COMBINING TILDE
        "b";
    text = mjb_text_make(stacked_combining, strlen(stacked_combining), enc);
    // clang-format on

    ATT_ASSERT_STATUS(mjb_filter(&text, MJB_MALFORMED_REPLACE,
                          MJB_FILTER_LIMIT_COMBINING, enc, &result, NULL),
        MJB_STATUS_OK, "Limit stacked combining marks");
    ATT_ASSERT(strcmp(result.output, expected_stacked_combining), 0,
        "Limit stacked combining marks output");
    ATT_ASSERT(result.output_size, strlen(expected_stacked_combining),
        "Limit stacked combining marks size");
    ATT_ASSERT(result.owned, true, "Limit stacked combining marks owned");
    mjb_result_free(&result);

    // clang-format off
    const char *split_by_removed_control = "a"
        "\xCC\x80" // U+0300 COMBINING GRAVE ACCENT
        "\xCC\x81" // U+0301 COMBINING ACUTE ACCENT
        "\xCC\x82" // U+0302 COMBINING CIRCUMFLEX ACCENT
        "\xCC\x83" // U+0303 COMBINING TILDE
        "\x01"
        "\xCC\x84"; // U+0304 COMBINING MACRON
    const char *expected_split_by_removed_control = "a"
        "\xCC\x80" // U+0300 COMBINING GRAVE ACCENT
        "\xCC\x81" // U+0301 COMBINING ACUTE ACCENT
        "\xCC\x82" // U+0302 COMBINING CIRCUMFLEX ACCENT
        "\xCC\x83"; // U+0303 COMBINING TILDE
    // clang-format on

    text = mjb_text_make(split_by_removed_control, strlen(split_by_removed_control), enc);

    ATT_ASSERT_STATUS(mjb_filter(&text, MJB_MALFORMED_REPLACE, (mjb_filter_flags)(MJB_FILTER_CONTROLS | MJB_FILTER_LIMIT_COMBINING), enc, &result, NULL),
        MJB_STATUS_OK, "Limit combining after removed control");
    ATT_ASSERT(strcmp(result.output, expected_split_by_removed_control), 0,
        "Limit combining after removed control output");
    ATT_ASSERT(result.output_size, strlen(expected_split_by_removed_control),
        "Limit combining after removed control size");
    ATT_ASSERT(result.owned, true, "Limit combining after removed control owned");
    mjb_result_free(&result);

    const char *valid_utf8 = "Hello World";
    text = mjb_text_make(valid_utf8, strlen(valid_utf8), enc);
    ATT_ASSERT_STATUS(mjb_filter(&text, MJB_MALFORMED_REPLACE, MJB_FILTER_NONE, enc,
        &result, NULL), MJB_STATUS_OK, "UTF8: Valid string");
    ATT_ASSERT(result.output, (char *)"Hello World", "UTF8: Valid string output");
    ATT_ASSERT(result.output_size, 11, "UTF8: Valid string size");
    ATT_ASSERT(result.owned, false, "UTF8: Valid string not owned");
    mjb_result_free(&result);

    const char *valid_multibyte = "Héllo Wörld 世界"; // Latin + CJK
    text = mjb_text_make(valid_multibyte, strlen(valid_multibyte), enc);
    ATT_ASSERT_STATUS(mjb_filter(&text, MJB_MALFORMED_REPLACE,
        MJB_FILTER_NONE, enc, &result, NULL), MJB_STATUS_OK, "UTF8: Valid multibyte");
    ATT_ASSERT(strcmp(result.output, valid_multibyte), 0, "UTF8: Valid multibyte unchanged");
    ATT_ASSERT(result.owned, false, "UTF8: Valid multibyte not owned");
    mjb_result_free(&result);

    // clang-format off
    const char *single_invalid = "A\xC0" "B"; // 0xC0 is invalid
    const char *expected_single = "A\xEF\xBF\xBD" "B";
    text = mjb_text_make(single_invalid, strlen(single_invalid), enc);
    // clang-format on

    ATT_ASSERT_STATUS(mjb_filter(&text, MJB_MALFORMED_REPLACE,
        MJB_FILTER_NONE, enc, &result, NULL), MJB_STATUS_OK, "UTF8: Single invalid byte");
    ATT_ASSERT(strcmp(result.output, expected_single), 0, "UTF8: Single invalid byte replaced");
    ATT_ASSERT(result.output_size, 5, "UTF8: Single invalid byte size");
    ATT_ASSERT(result.owned, true, "UTF8: Single invalid byte owned");
    mjb_result_free(&result);

    mjb_diagnostic diagnostic;
    text = mjb_text_make(single_invalid, strlen(single_invalid), enc);
    ATT_ASSERT_STATUS(mjb_filter(&text, MJB_MALFORMED_STOP,
                          MJB_FILTER_NONE, enc, &result, &diagnostic),
        MJB_STATUS_MALFORMED_INPUT, "UTF8: Stop policy rejects malformed input");
    ATT_ASSERT((unsigned int)diagnostic.error,
        (unsigned int)MJB_TEXT_ERROR_OVERLONG_SEQUENCE,
        "UTF8: Stop policy reports the malformed kind");
    ATT_ASSERT(diagnostic.byte_offset, (size_t)1,
        "UTF8: Stop policy reports the malformed byte offset");

    text = mjb_text_make(single_invalid, strlen(single_invalid), enc);
    ATT_ASSERT_STATUS(mjb_filter(&text, MJB_MALFORMED_SKIP,
                          MJB_FILTER_NONE, enc, &result, &diagnostic),
        MJB_STATUS_OK, "UTF8: Skip policy filters malformed input");
    ATT_ASSERT(result.output_size, (size_t)2, "UTF8: Skip policy output size");
    ATT_ASSERT(memcmp(result.output, "AB", 2), 0, "UTF8: Skip policy output");
    ATT_ASSERT(result.owned, true, "UTF8: Skip policy marks the result owned");
    ATT_ASSERT((unsigned int)diagnostic.error,
        (unsigned int)MJB_TEXT_ERROR_OVERLONG_SEQUENCE,
        "UTF8: Skip policy retains the malformed diagnostic");
    mjb_result_free(&result);

    text = mjb_text_make(single_invalid, strlen(single_invalid), enc);
    ATT_ASSERT_STATUS(mjb_filter(&text,
                          MJB_MALFORMED_REPLACE, MJB_FILTER_NORMALIZE, enc, &result,
                          &diagnostic),
        MJB_STATUS_OK, "UTF8: Replacement policy sanitizes before normalization");
    ATT_ASSERT(memcmp(result.output, expected_single, result.output_size), 0,
        "UTF8: Normalization preserves the replacement");
    ATT_ASSERT(result.owned, true,
        "UTF8: Sanitizing before normalization marks the result owned");
    mjb_result_free(&result);

    // clang-format off
    const char *multiple_invalid = "A\xC0" "B\xC1" "C";
    const char *expected_multiple = "A\xEF\xBF\xBD" "B\xEF\xBF\xBD" "C";
    text = mjb_text_make(multiple_invalid, strlen(multiple_invalid), enc);
    // clang-format on

    ATT_ASSERT_STATUS(mjb_filter(&text, MJB_MALFORMED_REPLACE,
        MJB_FILTER_NONE, enc, &result, NULL), MJB_STATUS_OK, "UTF8: Multiple invalid bytes");
    ATT_ASSERT(strcmp(result.output, expected_multiple), 0, "UTF8: Multiple invalid bytes replaced");
    ATT_ASSERT(result.output_size, 9, "UTF8: Multiple invalid bytes size");
    ATT_ASSERT(result.owned, true, "UTF8: Multiple invalid bytes owned");
    mjb_result_free(&result);

    // clang-format off
    const char *invalid_2byte = "A\xC0\xAF" "B"; // Invalid 2-byte sequence
    const char *expected_2byte = "A\xEF\xBF\xBD\xEF\xBF\xBD" "B";
    text = mjb_text_make(invalid_2byte, strlen(invalid_2byte), enc);
    // clang-format on

    ATT_ASSERT_STATUS(mjb_filter(&text, MJB_MALFORMED_REPLACE, MJB_FILTER_NONE,
        enc, &result, NULL), MJB_STATUS_OK, "UTF8: Invalid 2-byte sequence");
    ATT_ASSERT(strcmp(result.output, expected_2byte), 0,
        "UTF8: Invalid 2-byte maximal subparts replaced");
    ATT_ASSERT(result.output_size, 8, "UTF8: Invalid 2-byte sequence size");
    ATT_ASSERT(result.owned, true, "UTF8: Invalid 2-byte owned");
    mjb_result_free(&result);

    // clang-format off
    const char *invalid_3byte = "A\xE0\x80\x80" "B"; // Invalid 3-byte (overlong)
    const char *expected_3byte =
        "A\xEF\xBF\xBD\xEF\xBF\xBD\xEF\xBF\xBD" "B";
    text = mjb_text_make(invalid_3byte, strlen(invalid_3byte), enc);
    // clang-format on

    ATT_ASSERT_STATUS(mjb_filter(&text, MJB_MALFORMED_REPLACE, MJB_FILTER_NONE,
        enc, &result, NULL), MJB_STATUS_OK, "UTF8: Invalid 3-byte sequence");
    ATT_ASSERT(strcmp(result.output, expected_3byte), 0,
        "UTF8: Invalid 3-byte maximal subparts replaced");
    ATT_ASSERT(result.output_size, 11, "UTF8: Invalid 3-byte sequence size");
    ATT_ASSERT(result.owned, true, "UTF8: Invalid 3-byte owned");
    mjb_result_free(&result);

    // clang-format off
    const char *invalid_4byte = "A\xF5\x80\x80\x80" "B"; // 0xF5 is invalid start
    const char *expected_4byte =
        "A\xEF\xBF\xBD\xEF\xBF\xBD\xEF\xBF\xBD\xEF\xBF\xBD" "B";
    text = mjb_text_make(invalid_4byte, strlen(invalid_4byte), enc);
    // clang-format on
    ATT_ASSERT_STATUS(mjb_filter(&text, MJB_MALFORMED_REPLACE, MJB_FILTER_NONE,
        enc, &result, NULL), MJB_STATUS_OK, "UTF8: Invalid 4-byte sequence");
    ATT_ASSERT(strcmp(result.output, expected_4byte), 0,
        "UTF8: Invalid 4-byte maximal subparts replaced");
    ATT_ASSERT(result.output_size, 14, "UTF8: Invalid 4-byte sequence size");
    ATT_ASSERT(result.owned, true, "UTF8: Invalid 4-byte owned");
    mjb_result_free(&result);

    const char *truncated = "Hello\xC2"; // Incomplete 2-byte sequence
    const char *expected_truncated = "Hello\xEF\xBF\xBD";
    text = mjb_text_make(truncated, 6, enc);
    ATT_ASSERT_STATUS(mjb_filter(&text, MJB_MALFORMED_REPLACE, MJB_FILTER_NONE, enc, &result, NULL),
        MJB_STATUS_OK, "UTF8: Truncated sequence");
    ATT_ASSERT(strcmp(result.output, expected_truncated), 0, "UTF8: Truncated sequence replaced");
    ATT_ASSERT(result.output_size, 8, "UTF8: Truncated sequence size");
    ATT_ASSERT(result.owned, true, "UTF8: Truncated sequence owned");
    mjb_result_free(&result);

    // clang-format off
    const char *invalid_continuation = "A\x80" "B"; // 0x80 without lead byte
    const char *expected_continuation = "A\xEF\xBF\xBD" "B";
    text = mjb_text_make(invalid_continuation, strlen(invalid_continuation), enc);
    // clang-format on
    ATT_ASSERT_STATUS(mjb_filter(&text, MJB_MALFORMED_REPLACE,
        MJB_FILTER_NONE, enc, &result, NULL), MJB_STATUS_OK, "UTF8: Invalid continuation");
    ATT_ASSERT(strcmp(result.output, expected_continuation), 0,
        "UTF8: Invalid continuation replaced");
    ATT_ASSERT(result.output_size, 5, "UTF8: Invalid continuation size");
    ATT_ASSERT(result.owned, true, "UTF8: Invalid continuation owned");
    mjb_result_free(&result);

    const char *mixed_valid_invalid = "Hé\xC0llo\xF5\x80\x80\x80 世\xC1界";
    const char *expected_mixed = "Hé\xEF\xBF\xBDllo"
                                 "\xEF\xBF\xBD\xEF\xBF\xBD\xEF\xBF\xBD\xEF\xBF\xBD"
                                 " 世\xEF\xBF\xBD界";
    text = mjb_text_make(mixed_valid_invalid, strlen(mixed_valid_invalid), enc);
    ATT_ASSERT_STATUS(mjb_filter(&text, MJB_MALFORMED_REPLACE,
        MJB_FILTER_NONE, enc, &result, NULL), MJB_STATUS_OK, "UTF8: Mixed valid/invalid");
    ATT_ASSERT(strcmp(result.output, expected_mixed), 0, "UTF8: Mixed valid/invalid replaced");
    ATT_ASSERT(result.owned, true, "UTF8: Mixed valid/invalid owned");
    mjb_result_free(&result);

    // clang-format off
    const char *invalid_start = "\xC0" "Hello";
    const char *expected_start = "\xEF\xBF\xBD" "Hello";
    text = mjb_text_make(invalid_start, strlen(invalid_start), enc);
    // clang-format on
    ATT_ASSERT_STATUS(mjb_filter(&text, MJB_MALFORMED_REPLACE, MJB_FILTER_NONE,
        enc, &result, NULL), MJB_STATUS_OK, "UTF8: Invalid at start");
    ATT_ASSERT(strcmp(result.output, expected_start), 0, "UTF8: Invalid at start replaced");
    ATT_ASSERT(result.output_size, 8, "UTF8: Invalid at start size");
    ATT_ASSERT(result.owned, true, "UTF8: Invalid at start owned");
    mjb_result_free(&result);

    const char *invalid_end = "Hello\xC0";
    const char *expected_end = "Hello\xEF\xBF\xBD";
    text = mjb_text_make(invalid_end, 6, enc);
    ATT_ASSERT_STATUS(mjb_filter(&text, MJB_MALFORMED_REPLACE, MJB_FILTER_NONE, enc, &result, NULL),
        MJB_STATUS_OK, "UTF8: Invalid at end");
    ATT_ASSERT(strcmp(result.output, expected_end), 0, "UTF8: Invalid at end replaced");
    ATT_ASSERT(result.output_size, 8, "UTF8: Invalid at end size");
    ATT_ASSERT(result.owned, true, "UTF8: Invalid at end owned");
    mjb_result_free(&result);

    // clang-format off
    const char *consecutive_invalid = "A\xC0\xC1\xC2" "B"; // Three consecutive invalid bytes
    const char *expected_consecutive =
        "A\xEF\xBF\xBD\xEF\xBF\xBD\xEF\xBF\xBD" "B";
    text = mjb_text_make(consecutive_invalid, strlen(consecutive_invalid), enc);
    // clang-format on
    ATT_ASSERT_STATUS(mjb_filter(&text, MJB_MALFORMED_REPLACE,
        MJB_FILTER_NONE, enc, &result, NULL), MJB_STATUS_OK, "UTF8: Consecutive invalid bytes");
    ATT_ASSERT(strcmp(result.output, expected_consecutive), 0,
        "UTF8: Consecutive invalid maximal subparts replaced");
    ATT_ASSERT(result.output_size, 11, "UTF8: Consecutive invalid size");
    ATT_ASSERT(result.owned, true, "UTF8: Consecutive invalid owned");
    mjb_result_free(&result);

    const char *overlong_slash = "\xC0\xAF"; // Overlong encoding of '/'
    const char *expected_overlong = "\xEF\xBF\xBD\xEF\xBF\xBD";
    text = mjb_text_make(overlong_slash, 2, enc);
    ATT_ASSERT_STATUS(mjb_filter(&text, MJB_MALFORMED_REPLACE, MJB_FILTER_NONE, enc, &result, NULL),
        MJB_STATUS_OK, "UTF8: Overlong encoding");
    ATT_ASSERT(strcmp(result.output, expected_overlong), 0, "UTF8: Overlong encoding replaced");
    ATT_ASSERT(result.output_size, 6, "UTF8: Overlong encoding size");
    ATT_ASSERT(result.owned, true, "UTF8: Overlong encoding owned");
    mjb_result_free(&result);

    const char *only_invalid = "\xC0\xC1\xC2";
    const char *expected_only_invalid = "\xEF\xBF\xBD\xEF\xBF\xBD\xEF\xBF\xBD";
    text = mjb_text_make(only_invalid, 3, enc);
    ATT_ASSERT_STATUS(mjb_filter(&text, MJB_MALFORMED_REPLACE, MJB_FILTER_NONE, enc, &result, NULL),
        MJB_STATUS_OK, "UTF8: Only invalid bytes");
    ATT_ASSERT(strcmp(result.output, expected_only_invalid), 0, "UTF8: Only invalid bytes replaced");
    ATT_ASSERT(result.output_size, 9, "UTF8: Only invalid bytes size");
    ATT_ASSERT(result.owned, true, "UTF8: Only invalid bytes owned");
    mjb_result_free(&result);

    const char *invalid_then_valid = "\xC0\xE4\xB8\x96"; // Invalid + 世
    const char *expected_invalid_valid = "\xEF\xBF\xBD\xE4\xB8\x96";
    text = mjb_text_make(invalid_then_valid, 4, enc);
    ATT_ASSERT_STATUS(mjb_filter(&text, MJB_MALFORMED_REPLACE, MJB_FILTER_NONE, enc, &result, NULL),
        MJB_STATUS_OK, "UTF8: Invalid then valid");
    ATT_ASSERT(strcmp(result.output, expected_invalid_valid), 0,
        "UTF8: Invalid then valid replaced");
    ATT_ASSERT(result.output_size, 6, "UTF8: Invalid then valid size");
    ATT_ASSERT(result.owned, true, "UTF8: Invalid then valid owned");
    mjb_result_free(&result);

    // clang-format off
    const char *missing_continuation_2 = "A\xC2" "B"; // C2 needs continuation
    const char *expected_missing_2 = "A\xEF\xBF\xBD" "B";
    text = mjb_text_make(missing_continuation_2, strlen(missing_continuation_2), enc);
    // clang-format on
    ATT_ASSERT_STATUS(mjb_filter(&text, MJB_MALFORMED_REPLACE,
        MJB_FILTER_NONE, enc, &result, NULL), MJB_STATUS_OK, "UTF8: Missing continuation 2-byte");
    ATT_ASSERT(strcmp(result.output, expected_missing_2), 0,
        "UTF8: Missing continuation 2-byte replaced");
    ATT_ASSERT(result.output_size, 5, "UTF8: Missing continuation 2-byte size");
    ATT_ASSERT(result.owned, true, "UTF8: Missing continuation 2-byte owned");
    mjb_result_free(&result);

    // clang-format off
    const char *missing_continuation_3 = "A\xE0\xA0" "B"; // E0 A0 needs another byte
    const char *expected_missing_3 = "A\xEF\xBF\xBD" "B";
    text = mjb_text_make(missing_continuation_3, strlen(missing_continuation_3), enc);
    // clang-format on
    ATT_ASSERT_STATUS(mjb_filter(&text, MJB_MALFORMED_REPLACE,
        MJB_FILTER_NONE, enc, &result, NULL), MJB_STATUS_OK, "UTF8: Missing continuation 3-byte");
    ATT_ASSERT(strcmp(result.output, expected_missing_3), 0,
        "UTF8: Missing continuation 3-byte replaced");
    ATT_ASSERT(result.output_size, 5, "UTF8: Missing continuation 3-byte size");
    ATT_ASSERT(result.owned, true, "UTF8: Missing continuation 3-byte owned");
    mjb_result_free(&result);

    // clang-format off
    const char *replacement_then_valid = "\xC0" "Hello";
    const char *expected_replacement_valid = "\xEF\xBF\xBD" "Hello";
    text = mjb_text_make(replacement_then_valid, strlen(replacement_then_valid), enc);
    // clang-format on
    ATT_ASSERT_STATUS(mjb_filter(&text, MJB_MALFORMED_REPLACE,
                          MJB_FILTER_NONE, enc, &result, NULL),
        MJB_STATUS_OK, "UTF8: Replacement then valid");
    ATT_ASSERT(strcmp(result.output, expected_replacement_valid), 0,
        "UTF8: Replacement then valid text");
    ATT_ASSERT(result.output_size, 8, "UTF8: Replacement then valid size");
    ATT_ASSERT(result.owned, true, "UTF8: Replacement then valid owned");
    mjb_result_free(&result);

    // clang-format off
    const char *separate_invalid = "A\xC0" "B\xE0\x80" "C\xF5\x80" "D";
    const char *expected_separate = "A\xEF\xBF\xBD"
        "B\xEF\xBF\xBD\xEF\xBF\xBD"
        "C\xEF\xBF\xBD\xEF\xBF\xBD"
        "D";
    text = mjb_text_make(separate_invalid, strlen(separate_invalid), enc);
    // clang-format on
    ATT_ASSERT_STATUS(mjb_filter(&text, MJB_MALFORMED_REPLACE,
        MJB_FILTER_NONE, enc, &result, NULL), MJB_STATUS_OK, "UTF8: Separate invalid sequences");
    ATT_ASSERT(strcmp(result.output, expected_separate), 0,
        "UTF8: Separate invalid sequences replaced");
    ATT_ASSERT(result.output_size, 19, "UTF8: Separate invalid sequences size");
    ATT_ASSERT(result.owned, true, "UTF8: Separate invalid sequences owned");
    mjb_result_free(&result);
}
