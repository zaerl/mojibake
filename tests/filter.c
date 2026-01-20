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

    ATT_ASSERT(mjb_string_filter(spaces, strlen(spaces), enc, enc, (mjb_filter)(MJB_FILTER_NORMALIZE | MJB_FILTER_SPACES), &result), true, "Filter spaces and normalize");
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
    ATT_ASSERT(mjb_string_filter(unicode_multiple_spaces, strlen(unicode_multiple_spaces), enc, enc, (mjb_filter)(MJB_FILTER_SPACES | MJB_FILTER_COLLAPSE_SPACES), &result), true, "Collapse Unicode spaces");
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

    const char *numeric = "1234567890";
    ATT_ASSERT(mjb_string_filter(numeric, strlen(numeric), enc, enc, MJB_FILTER_NUMERIC, &result), true, "Filter numeric");
    ATT_ASSERT(result.output, (char*)"1234567890", "Filter numeric output");
    ATT_ASSERT(result.output_size, 10, "Filter numeric output size");
    ATT_ASSERT(result.transformed, false, "Filter numeric transformed");
    FREE_RESULT

    const char *arabic_indic_digit = "\xD9\xA1\xD9\xA2"; // U+0661 ARABIC-INDIC DIGIT ONE, U+0662 ARABIC-INDIC DIGIT TWO
    ATT_ASSERT(mjb_string_filter(arabic_indic_digit, strlen(arabic_indic_digit), enc, enc, MJB_FILTER_NUMERIC, &result), true, "Filter arabic indic digit");
    ATT_ASSERT(result.output, (char*)"12", "Filter arabic indic digit output");
    ATT_ASSERT(result.output_size, 2, "Filter arabic indic digit output size");
    ATT_ASSERT(result.transformed, true, "Filter arabic indic digit transformed");
    FREE_RESULT

    const char *valid_utf8 = "Hello World";
    ATT_ASSERT(mjb_string_filter(valid_utf8, strlen(valid_utf8), enc, enc, MJB_FILTER_NONE, &result), true, "UTF8: Valid string");
    ATT_ASSERT(result.output, (char*)"Hello World", "UTF8: Valid string output");
    ATT_ASSERT(result.output_size, 11, "UTF8: Valid string size");
    ATT_ASSERT(result.transformed, false, "UTF8: Valid string not transformed");
    FREE_RESULT

    const char *valid_multibyte = "Héllo Wörld 世界"; // Latin + CJK
    ATT_ASSERT(mjb_string_filter(valid_multibyte, strlen(valid_multibyte), enc, enc, MJB_FILTER_NONE, &result), true, "UTF8: Valid multibyte");
    ATT_ASSERT(strcmp(result.output, valid_multibyte), 0, "UTF8: Valid multibyte unchanged");
    ATT_ASSERT(result.transformed, false, "UTF8: Valid multibyte not transformed");
    FREE_RESULT

    const char *single_invalid = "A\xC0""B"; // 0xC0 is invalid
    const char *expected_single = "A\xEF\xBF\xBD""B";
    ATT_ASSERT(mjb_string_filter(single_invalid, strlen(single_invalid), enc, enc, MJB_FILTER_NONE, &result), true, "UTF8: Single invalid byte");
    ATT_ASSERT(strcmp(result.output, expected_single), 0, "UTF8: Single invalid byte replaced");
    ATT_ASSERT(result.output_size, 5, "UTF8: Single invalid byte size");
    ATT_ASSERT(result.transformed, true, "UTF8: Single invalid byte transformed");
    FREE_RESULT

    const char *multiple_invalid = "A\xC0""B\xC1""C";
    const char *expected_multiple = "A\xEF\xBF\xBD""B\xEF\xBF\xBD""C";
    ATT_ASSERT(mjb_string_filter(multiple_invalid, strlen(multiple_invalid), enc, enc, MJB_FILTER_NONE, &result), true, "UTF8: Multiple invalid bytes");
    ATT_ASSERT(strcmp(result.output, expected_multiple), 0, "UTF8: Multiple invalid bytes replaced");
    ATT_ASSERT(result.output_size, 9, "UTF8: Multiple invalid bytes size");
    ATT_ASSERT(result.transformed, true, "UTF8: Multiple invalid bytes transformed");
    FREE_RESULT

    const char *invalid_2byte = "A\xC0\xAF""B"; // Invalid 2-byte sequence
    const char *expected_2byte = "A\xEF\xBF\xBD""B";
    ATT_ASSERT(mjb_string_filter(invalid_2byte, strlen(invalid_2byte), enc, enc, MJB_FILTER_NONE, &result), true, "UTF8: Invalid 2-byte sequence");
    ATT_ASSERT(strcmp(result.output, expected_2byte), 0, "UTF8: Invalid 2-byte collapsed to one U+FFFD");
    ATT_ASSERT(result.output_size, 5, "UTF8: Invalid 2-byte sequence size");
    ATT_ASSERT(result.transformed, true, "UTF8: Invalid 2-byte transformed");
    FREE_RESULT

    const char *invalid_3byte = "A\xE0\x80\x80""B"; // Invalid 3-byte (overlong)
    const char *expected_3byte = "A\xEF\xBF\xBD""B";
    ATT_ASSERT(mjb_string_filter(invalid_3byte, strlen(invalid_3byte), enc, enc, MJB_FILTER_NONE, &result), true, "UTF8: Invalid 3-byte sequence");
    ATT_ASSERT(strcmp(result.output, expected_3byte), 0, "UTF8: Invalid 3-byte collapsed to one U+FFFD");
    ATT_ASSERT(result.output_size, 5, "UTF8: Invalid 3-byte sequence size");
    ATT_ASSERT(result.transformed, true, "UTF8: Invalid 3-byte transformed");
    FREE_RESULT

    const char *invalid_4byte = "A\xF5\x80\x80\x80""B"; // 0xF5 is invalid start
    const char *expected_4byte = "A\xEF\xBF\xBD""B";
    ATT_ASSERT(mjb_string_filter(invalid_4byte, strlen(invalid_4byte), enc, enc, MJB_FILTER_NONE, &result), true, "UTF8: Invalid 4-byte sequence");
    ATT_ASSERT(strcmp(result.output, expected_4byte), 0, "UTF8: Invalid 4-byte collapsed to one U+FFFD");
    ATT_ASSERT(result.output_size, 5, "UTF8: Invalid 4-byte sequence size");
    ATT_ASSERT(result.transformed, true, "UTF8: Invalid 4-byte transformed");
    FREE_RESULT

    const char *truncated = "Hello\xC2"; // Incomplete 2-byte sequence
    const char *expected_truncated = "Hello\xEF\xBF\xBD";
    ATT_ASSERT(mjb_string_filter(truncated, 6, enc, enc, MJB_FILTER_NONE, &result), true, "UTF8: Truncated sequence");
    ATT_ASSERT(strcmp(result.output, expected_truncated), 0, "UTF8: Truncated sequence replaced");
    ATT_ASSERT(result.output_size, 8, "UTF8: Truncated sequence size");
    ATT_ASSERT(result.transformed, true, "UTF8: Truncated sequence transformed");
    FREE_RESULT

    const char *invalid_continuation = "A\x80""B"; // 0x80 without lead byte
    const char *expected_continuation = "A\xEF\xBF\xBD""B";
    ATT_ASSERT(mjb_string_filter(invalid_continuation, strlen(invalid_continuation), enc, enc, MJB_FILTER_NONE, &result), true, "UTF8: Invalid continuation");
    ATT_ASSERT(strcmp(result.output, expected_continuation), 0, "UTF8: Invalid continuation replaced");
    ATT_ASSERT(result.output_size, 5, "UTF8: Invalid continuation size");
    ATT_ASSERT(result.transformed, true, "UTF8: Invalid continuation transformed");
    FREE_RESULT

    const char *mixed_valid_invalid = "Hé\xC0llo\xF5\x80\x80\x80 世\xC1界";
    const char *expected_mixed = "Hé\xEF\xBF\xBDllo\xEF\xBF\xBD 世\xEF\xBF\xBD界";
    ATT_ASSERT(mjb_string_filter(mixed_valid_invalid, strlen(mixed_valid_invalid), enc, enc, MJB_FILTER_NONE, &result), true, "UTF8: Mixed valid/invalid");
    ATT_ASSERT(strcmp(result.output, expected_mixed), 0, "UTF8: Mixed valid/invalid replaced");
    ATT_ASSERT(result.transformed, true, "UTF8: Mixed valid/invalid transformed");
    FREE_RESULT

    const char *invalid_start = "\xC0""Hello";
    const char *expected_start = "\xEF\xBF\xBD""Hello";
    ATT_ASSERT(mjb_string_filter(invalid_start, strlen(invalid_start), enc, enc, MJB_FILTER_NONE, &result), true, "UTF8: Invalid at start");
    ATT_ASSERT(strcmp(result.output, expected_start), 0, "UTF8: Invalid at start replaced");
    ATT_ASSERT(result.output_size, 8, "UTF8: Invalid at start size");
    ATT_ASSERT(result.transformed, true, "UTF8: Invalid at start transformed");
    FREE_RESULT

    const char *invalid_end = "Hello\xC0";
    const char *expected_end = "Hello\xEF\xBF\xBD";
    ATT_ASSERT(mjb_string_filter(invalid_end, 6, enc, enc, MJB_FILTER_NONE, &result), true, "UTF8: Invalid at end");
    ATT_ASSERT(strcmp(result.output, expected_end), 0, "UTF8: Invalid at end replaced");
    ATT_ASSERT(result.output_size, 8, "UTF8: Invalid at end size");
    ATT_ASSERT(result.transformed, true, "UTF8: Invalid at end transformed");
    FREE_RESULT

    const char *consecutive_invalid = "A\xC0\xC1\xC2""B"; // Three consecutive invalid bytes
    const char *expected_consecutive = "A\xEF\xBF\xBD""B";
    ATT_ASSERT(mjb_string_filter(consecutive_invalid, strlen(consecutive_invalid), enc, enc, MJB_FILTER_NONE, &result), true, "UTF8: Consecutive invalid bytes");
    ATT_ASSERT(strcmp(result.output, expected_consecutive), 0, "UTF8: Consecutive invalid collapsed");
    ATT_ASSERT(result.output_size, 5, "UTF8: Consecutive invalid size");
    ATT_ASSERT(result.transformed, true, "UTF8: Consecutive invalid transformed");
    FREE_RESULT

    const char *overlong_slash = "\xC0\xAF"; // Overlong encoding of '/'
    const char *expected_overlong = "\xEF\xBF\xBD";
    ATT_ASSERT(mjb_string_filter(overlong_slash, 2, enc, enc, MJB_FILTER_NONE, &result), true, "UTF8: Overlong encoding");
    ATT_ASSERT(strcmp(result.output, expected_overlong), 0, "UTF8: Overlong encoding replaced");
    ATT_ASSERT(result.output_size, 3, "UTF8: Overlong encoding size");
    ATT_ASSERT(result.transformed, true, "UTF8: Overlong encoding transformed");
    FREE_RESULT

    const char *only_invalid = "\xC0\xC1\xC2";
    const char *expected_only_invalid = "\xEF\xBF\xBD";
    ATT_ASSERT(mjb_string_filter(only_invalid, 3, enc, enc, MJB_FILTER_NONE, &result), true, "UTF8: Only invalid bytes");
    ATT_ASSERT(strcmp(result.output, expected_only_invalid), 0, "UTF8: Only invalid bytes replaced");
    ATT_ASSERT(result.output_size, 3, "UTF8: Only invalid bytes size");
    ATT_ASSERT(result.transformed, true, "UTF8: Only invalid bytes transformed");
    FREE_RESULT

    const char *invalid_then_valid = "\xC0\xE4\xB8\x96"; // Invalid + 世
    const char *expected_invalid_valid = "\xEF\xBF\xBD\xE4\xB8\x96";
    ATT_ASSERT(mjb_string_filter(invalid_then_valid, 4, enc, enc, MJB_FILTER_NONE, &result), true, "UTF8: Invalid then valid");
    ATT_ASSERT(strcmp(result.output, expected_invalid_valid), 0, "UTF8: Invalid then valid replaced");
    ATT_ASSERT(result.output_size, 6, "UTF8: Invalid then valid size");
    ATT_ASSERT(result.transformed, true, "UTF8: Invalid then valid transformed");
    FREE_RESULT

    const char *missing_continuation_2 = "A\xC2""B"; // C2 needs continuation
    const char *expected_missing_2 = "A\xEF\xBF\xBD""B";
    ATT_ASSERT(mjb_string_filter(missing_continuation_2, strlen(missing_continuation_2), enc, enc, MJB_FILTER_NONE, &result), true, "UTF8: Missing continuation 2-byte");
    ATT_ASSERT(strcmp(result.output, expected_missing_2), 0, "UTF8: Missing continuation 2-byte replaced");
    ATT_ASSERT(result.output_size, 5, "UTF8: Missing continuation 2-byte size");
    ATT_ASSERT(result.transformed, true, "UTF8: Missing continuation 2-byte transformed");
    FREE_RESULT

    const char *missing_continuation_3 = "A\xE0\xA0""B"; // E0 A0 needs another byte
    const char *expected_missing_3 = "A\xEF\xBF\xBD""B";
    ATT_ASSERT(mjb_string_filter(missing_continuation_3, strlen(missing_continuation_3), enc, enc, MJB_FILTER_NONE, &result), true, "UTF8: Missing continuation 3-byte");
    ATT_ASSERT(strcmp(result.output, expected_missing_3), 0, "UTF8: Missing continuation 3-byte replaced");
    ATT_ASSERT(result.output_size, 5, "UTF8: Missing continuation 3-byte size");
    ATT_ASSERT(result.transformed, true, "UTF8: Missing continuation 3-byte transformed");
    FREE_RESULT

    const char *replacement_then_valid = "\xC0""Hello";
    const char *expected_replacement_valid = "\xEF\xBF\xBD""Hello";
    ATT_ASSERT(mjb_string_filter(replacement_then_valid, strlen(replacement_then_valid), enc, enc, MJB_FILTER_NONE, &result), true, "UTF8: Replacement then valid");
    ATT_ASSERT(strcmp(result.output, expected_replacement_valid), 0, "UTF8: Replacement then valid text");
    ATT_ASSERT(result.output_size, 8, "UTF8: Replacement then valid size");
    ATT_ASSERT(result.transformed, true, "UTF8: Replacement then valid transformed");
    FREE_RESULT

    const char *separate_invalid = "A\xC0""B\xE0\x80""C\xF5\x80""D";
    const char *expected_separate = "A\xEF\xBF\xBD""B\xEF\xBF\xBD""C\xEF\xBF\xBD""D";
    ATT_ASSERT(mjb_string_filter(separate_invalid, strlen(separate_invalid), enc, enc, MJB_FILTER_NONE, &result), true, "UTF8: Separate invalid sequences");
    ATT_ASSERT(strcmp(result.output, expected_separate), 0, "UTF8: Separate invalid sequences replaced");
    ATT_ASSERT(result.output_size, 13, "UTF8: Separate invalid sequences size");
    ATT_ASSERT(result.transformed, true, "UTF8: Separate invalid sequences transformed");
    FREE_RESULT

    #undef FREE_RESULT

    return NULL;
 }
