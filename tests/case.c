/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <string.h>

#include "test.h"

void *test_case(void *arg) {
    mjb_encoding encoding = MJB_ENCODING_UTF_8;

    // Test case conversion functions
    // CURRENT_ASSERT mjb_case
    // CURRENT_COUNT 20
    char *result = NULL;

    // Test uppercase conversion
    result = mjb_case("hello", 5, MJB_CASE_UPPER, encoding);
    ATT_ASSERT(result, (char*)"HELLO", "UTF-8 uppercase: hello")
    mjb_free(result);

    result = mjb_case("héllö", 7, MJB_CASE_UPPER, encoding);
    ATT_ASSERT(result, (char*)"HÉLLÖ", "UTF-8 uppercase: héllö")
    mjb_free(result);

    // Test lowercase conversion
    result = mjb_case("HELLO", 5, MJB_CASE_LOWER, encoding);
    ATT_ASSERT(result, (char*)"hello", "UTF-8 lowercase: HELLO")
    mjb_free(result);

    result = mjb_case("HÉLLÖ", 7, MJB_CASE_LOWER, encoding);
    ATT_ASSERT(result, (char*)"héllö", "UTF-8 lowercase: HÉLLÖ")
    mjb_free(result);

    // Test titlecase conversion
    result = mjb_case("hello world", 11, MJB_CASE_TITLE, encoding);
    ATT_ASSERT(result, (char*)"Hello World", "UTF-8 titlecase: hello world")
    mjb_free(result);

    result = mjb_case("héllö wörld", 14, MJB_CASE_TITLE, encoding);
    ATT_ASSERT(result, (char*)"Héllö Wörld", "UTF-8 titlecase: héllö wörld")
    mjb_free(result);

    result = mjb_case("hello world", 11, MJB_CASE_TITLE, encoding);
    ATT_ASSERT(result, (char*)"Hello World", "UTF-8 titlecase: hello world")
    mjb_free(result);

    result = mjb_case("HELLO WORLD", 11, MJB_CASE_TITLE, encoding);
    ATT_ASSERT(result, (char*)"Hello World", "UTF-8 titlecase: HELLO WORLD")
    mjb_free(result);

    result = mjb_case("HELLO WORLD", 11, MJB_CASE_TITLE, encoding);
    ATT_ASSERT(result, (char*)"Hello World", "UTF-8 titlecase: HELLO WORLD")
    mjb_free(result);

    result = mjb_case("mixed CASE words", 17, MJB_CASE_TITLE, encoding);
    ATT_ASSERT(result, (char*)"Mixed Case Words", "UTF-8 titlecase: mixed CASE words")
    mjb_free(result);

    result = mjb_case("  leading space", 15, MJB_CASE_TITLE, encoding);
    ATT_ASSERT(result, (char*)"  Leading Space", "UTF-8 titlecase:   leading space")
    mjb_free(result);

    result = mjb_case("élan vital", 11, MJB_CASE_TITLE, encoding);
    ATT_ASSERT(result, (char*)"Élan Vital", "UTF-8 titlecase: élan vital")
    mjb_free(result);

    result = mjb_case("straße", 7, MJB_CASE_TITLE, encoding);
    ATT_ASSERT(result, (char*)"Straße", "UTF-8 titlecase: straße")
    mjb_free(result);

    result = mjb_case("παράδειγμα", 20, MJB_CASE_TITLE, encoding);
    ATT_ASSERT(result, (char*)"Παράδειγμα", "UTF-8 titlecase: παράδειγμα")
    mjb_free(result);

    result = mjb_case("ⅲ times", 10, MJB_CASE_TITLE, encoding);
    ATT_ASSERT(result, (char*)"Ⅲ Times", "UTF-8 titlecase: ⅲ times")
    mjb_free(result);

    // TODO: add support for WordBreakProperty.txt
    // See: https://www.unicode.org/reports/tr29/#Word_Boundaries
    // 2019..2019    MidLetter # Po  RIGHT SINGLE QUOTATION MARK
    // WB6: ALetter × MidLetter ALetter
    // WB7: ALetter MidLetter × ALetter
    // result = mjb_case("o’connor", 9, MJB_CASE_TITLE, encoding);
    // ATT_ASSERT(result, "O’Connor", "UTF-8 titlecase: o’connor")
    // mjb_free(result);

    result = mjb_case("İstanbul", 9, MJB_CASE_TITLE, encoding);
    ATT_ASSERT(result, (char*)"İstanbul", "UTF-8 titlecase: İstanbul")
    mjb_free(result);

    // Modern German orthography sometimes prefers the uppercase form ẞ (U+1E9E) in all-caps or
    // titlecase contexts. Unicode's default case folding still maps ß to SS in titlecase unless
    // locale-specific tailoring is applied.
    result = mjb_case("ßeta", 5, MJB_CASE_UPPER, encoding);
    ATT_ASSERT(result, (char*)"SSETA", "UTF-8 titlecase: ßeta")
    mjb_free(result);

    result = mjb_case("coöperate", 10, MJB_CASE_TITLE, encoding);
    ATT_ASSERT(result, (char*)"Coöperate", "UTF-8 titlecase: Český Krumlov")
    mjb_free(result);

    result = mjb_case("😀grinning", 12, MJB_CASE_TITLE, encoding);
    ATT_ASSERT(result, (char*)"😀Grinning", "UTF-8 titlecase: 😀grinning")
    mjb_free(result);

    result = mjb_case("123abc", 8, MJB_CASE_TITLE, encoding);
    ATT_ASSERT(result, (char*)"123Abc", "UTF-8 titlecase: 123abc")
    mjb_free(result);

    ATT_ASSERT(mjb_codepoint_to_lowercase('#'), '#', "Lowercase #: #")
    ATT_ASSERT(mjb_codepoint_to_uppercase('#'), '#', "Uppercase #: #")
    ATT_ASSERT(mjb_codepoint_to_titlecase('#'), '#', "Titlecase #: #")
    ATT_ASSERT(mjb_codepoint_to_lowercase('A'), 'a', "Lowercase: A > a")
    ATT_ASSERT(mjb_codepoint_to_lowercase('a'), 'a', "Lowercase: a > a")
    ATT_ASSERT(mjb_codepoint_to_uppercase('b'), 'B', "Uppercase: b > B")
    ATT_ASSERT(mjb_codepoint_to_uppercase('B'), 'B', "Uppercase: B > B")
    ATT_ASSERT(mjb_codepoint_to_titlecase('c'), 'C', "Titlecase: c > C")
    ATT_ASSERT(mjb_codepoint_to_titlecase('C'), 'C', "Titlecase: C > C")

    return NULL;
}
