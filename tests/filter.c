/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

 #include "test.h"

 void *test_filter(void *arg) {
    mjb_result result;
    mjb_encoding enc = MJB_ENCODING_UTF_8;

    #define FREE_RESULT if(result.output != NULL && result.transformed) { mjb_free(result.output); }

    ATT_ASSERT(mjb_string_filter("", 0, enc, enc, MJB_FILTER_NONE, &result), true, "Filter empty string");
    ATT_ASSERT(result.output, (char*)"", "Filter empty string output");
    ATT_ASSERT(result.output_size, 0, "Filter empty string size");
    ATT_ASSERT(result.transformed, false, "Filter empty string transformed");
    FREE_RESULT

    ATT_ASSERT(mjb_string_filter("", 0, enc, enc, MJB_FILTER_NORMALIZE, &result), true, "Filter normalize empty string");
    ATT_ASSERT(result.output, (char*)"", "Filter normalize empty string output");
    ATT_ASSERT(result.output_size, 0, "Filter normalize empty string size");
    ATT_ASSERT(result.transformed, false, "Filter normalize empty string transformed");
    FREE_RESULT

    ATT_ASSERT(mjb_string_filter("   ", 0, enc, enc, MJB_FILTER_SPACES, &result), true, "Filter spaces");
    ATT_ASSERT(result.output, (char*)"   ", "Filter spaces output");
    FREE_RESULT

    const char *spaces =
        "\x20"          // U+0020 SPACE
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

    ATT_ASSERT(mjb_string_filter(spaces, strlen(spaces), enc, enc, MJB_FILTER_SPACES, &result), true, "Filter spaces");
    ATT_ASSERT(result.output, (char*)"                 ", "Filter spaces output");
    ATT_ASSERT(result.output_size, 17, "Filter spaces output size");
    FREE_RESULT

    ATT_ASSERT(mjb_string_filter(spaces, strlen(spaces), enc, enc, MJB_FILTER_NORMALIZE | MJB_FILTER_SPACES, &result), true, "Filter spaces and normalize");
    ATT_ASSERT(result.output, (char*)"                 ", "Filter spaces and normalize output");
    ATT_ASSERT(result.output_size, 17, "Filter spaces and normalize output size");
    FREE_RESULT

    // Test whitespace collapsing with consecutive spaces
    const char *multiple_spaces = "hello    world";
    ATT_ASSERT(mjb_string_filter(multiple_spaces, strlen(multiple_spaces), enc, enc, MJB_FILTER_COLLAPSE_SPACES, &result), true, "Collapse multiple spaces");
    ATT_ASSERT(result.output, (char*)"hello world", "Collapse multiple spaces output");
    ATT_ASSERT(result.output_size, 11, "Collapse multiple spaces output size");
    FREE_RESULT

    // Test whitespace collapsing with tabs and newlines
    const char *mixed_whitespace = "hello\t\t\n\nworld";
    ATT_ASSERT(mjb_string_filter(mixed_whitespace, strlen(mixed_whitespace), enc, enc, MJB_FILTER_COLLAPSE_SPACES, &result), true, "Collapse mixed whitespace");
    ATT_ASSERT(result.output, (char*)"hello world", "Collapse mixed whitespace output");
    ATT_ASSERT(result.output_size, 11, "Collapse mixed whitespace output size");
    FREE_RESULT

    // Test whitespace collapsing with leading whitespace
    const char *leading_whitespace = "   hello world";
    ATT_ASSERT(mjb_string_filter(leading_whitespace, strlen(leading_whitespace), enc, enc, MJB_FILTER_COLLAPSE_SPACES, &result), true, "Collapse leading whitespace");
    ATT_ASSERT(result.output, (char*)" hello world", "Collapse leading whitespace output");
    ATT_ASSERT(result.output_size, 12, "Collapse leading whitespace output size");
    FREE_RESULT

    // Test whitespace collapsing with trailing whitespace
    const char *trailing_whitespace = "hello world   ";
    ATT_ASSERT(mjb_string_filter(trailing_whitespace, strlen(trailing_whitespace), enc, enc, MJB_FILTER_COLLAPSE_SPACES, &result), true, "Collapse trailing whitespace");
    ATT_ASSERT(result.output, (char*)"hello world ", "Collapse trailing whitespace output");
    ATT_ASSERT(result.output_size, 12, "Collapse trailing whitespace output size");
    FREE_RESULT

    // Test whitespace collapsing combined with MJB_FILTER_SPACES
    const char *unicode_multiple_spaces =
        "hello"
        "\xE2\x80\x80\xE2\x80\x81"  // U+2000 EN QUAD, U+2001 EM QUAD
        "world";
    ATT_ASSERT(mjb_string_filter(unicode_multiple_spaces, strlen(unicode_multiple_spaces), enc, enc, MJB_FILTER_SPACES | MJB_FILTER_COLLAPSE_SPACES, &result), true, "Collapse Unicode spaces");
    ATT_ASSERT(result.output, (char*)"hello world", "Collapse Unicode spaces output");
    ATT_ASSERT(result.output_size, 11, "Collapse Unicode spaces output size");
    FREE_RESULT

    // Test whitespace collapsing with complex mixed whitespace
    const char *complex_whitespace = "one  \t\n  two\r\n\r\nthree    four";
    ATT_ASSERT(mjb_string_filter(complex_whitespace, strlen(complex_whitespace), enc, enc, MJB_FILTER_COLLAPSE_SPACES, &result), true, "Collapse complex whitespace");
    ATT_ASSERT(result.output, (char*)"one two three four", "Collapse complex whitespace output");
    ATT_ASSERT(result.output_size, 18, "Collapse complex whitespace output size");
    FREE_RESULT

    // Test whitespace collapsing with only whitespace
    const char *only_whitespace = "  \t\n  ";
    ATT_ASSERT(mjb_string_filter(only_whitespace, strlen(only_whitespace), enc, enc, MJB_FILTER_COLLAPSE_SPACES, &result), true, "Collapse only whitespace");
    ATT_ASSERT(result.output, (char*)" ", "Collapse only whitespace output");
    ATT_ASSERT(result.output_size, 1, "Collapse only whitespace output size");
    FREE_RESULT

    // Test no collapsing when there's no consecutive whitespace
    const char *single_spaces = "hello world test";
    ATT_ASSERT(mjb_string_filter(single_spaces, strlen(single_spaces), enc, enc, MJB_FILTER_COLLAPSE_SPACES, &result), true, "No collapse needed");
    ATT_ASSERT(result.output, (char*)"hello world test", "No collapse needed output");
    ATT_ASSERT(result.output_size, 16, "No collapse needed output size");
    FREE_RESULT

    const char *controls = "\x1\x2\t\n\v\f\r\x1f";
    ATT_ASSERT(mjb_string_filter(controls, strlen(controls), enc, enc, MJB_FILTER_CONTROLS, &result), true, "Filter controls");
    ATT_ASSERT(result.output, (char*)"\t\n\v\f\r", "Filter controls output");
    ATT_ASSERT(result.output_size, 5, "Filter controls output size");
    ATT_ASSERT(result.transformed, true, "Filter controls transformed");
    FREE_RESULT

    #undef FREE_RESULT

    return NULL;
 }
