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

    #undef FREE_RESULT

    return NULL;
 }
