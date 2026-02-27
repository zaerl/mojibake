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
    // CURRENT_COUNT 32
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

    // Test casefold: ASCII uppercase via unicode_data.lowercase fallback
    result = mjb_case("ABC", 3, MJB_CASE_CASEFOLD, encoding);
    ATT_ASSERT(result, (char*)"abc", "Casefold: ABC -> abc")
    mjb_free(result);

    result = mjb_case("Hello World", 11, MJB_CASE_CASEFOLD, encoding);
    ATT_ASSERT(result, (char*)"hello world", "Casefold: Hello World -> hello world")
    mjb_free(result);

    // Test casefold: ß (U+00DF) -> ss, F entry (multi-char expansion)
    result = mjb_case("ß", 2, MJB_CASE_CASEFOLD, encoding);
    ATT_ASSERT(result, (char*)"ss", "Casefold: ß -> ss")
    mjb_free(result);

    result = mjb_case("straße", 7, MJB_CASE_CASEFOLD, encoding);
    ATT_ASSERT(result, (char*)"strasse", "Casefold: straße -> strasse")
    mjb_free(result);

    // Test casefold: µ (U+00B5 MICRO SIGN) -> μ (U+03BC), C exception entry
    result = mjb_case("µ", 2, MJB_CASE_CASEFOLD, encoding);
    ATT_ASSERT(result, (char*)"μ", "Casefold: µ (U+00B5) -> μ (U+03BC)")
    mjb_free(result);

    // Test casefold: ﬃ (U+FB03) -> ffi, F entry with 3-codepoint expansion
    result = mjb_case("ﬃ", 3, MJB_CASE_CASEFOLD, encoding);
    ATT_ASSERT(result, (char*)"ffi", "Casefold: ﬃ -> ffi")
    mjb_free(result);

    // Test casefold: Greek uppercase via unicode_data.lowercase fallback
    result = mjb_case("Σ", 2, MJB_CASE_CASEFOLD, encoding);
    ATT_ASSERT(result, (char*)"σ", "Casefold: Σ -> σ")
    mjb_free(result);

    // Test casefold: digits/symbols pass through unchanged (identity path)
    result = mjb_case("123", 3, MJB_CASE_CASEFOLD, encoding);
    ATT_ASSERT(result, (char*)"123", "Casefold: 123 -> 123")
    mjb_free(result);

    // Test Final_Sigma rule: word-final Σ → ς, non-final Σ → σ
    result = mjb_case("ΣΕΙΣ", 8, MJB_CASE_LOWER, encoding);
    ATT_ASSERT(result, (char*)"σεις", "UTF-8 lowercase Final_Sigma: ΣΕΙΣ -> σεις")
    mjb_free(result);

    result = mjb_case("ΑΣΑ", 6, MJB_CASE_LOWER, encoding);
    ATT_ASSERT(result, (char*)"ασα", "UTF-8 lowercase non-final sigma: ΑΣΑ -> ασα")
    mjb_free(result);

    result = mjb_case("ΣΕΙΣ", 8, MJB_CASE_TITLE, encoding);
    ATT_ASSERT(result, (char*)"Σεις", "UTF-8 titlecase Final_Sigma: ΣΕΙΣ -> Σεις")
    mjb_free(result);

    // Test that titlecase uses original codepoint for special-casing lookup (Fix 2):
    // In-word İ (U+0130) must lower to "i + U+0307 (combining dot above)", not bare "i".
    result = mjb_case("AİB", 4, MJB_CASE_TITLE, encoding);
    ATT_ASSERT(result, (char*)"Ai\xCC\x87""b", "UTF-8 titlecase special casing: AİB -> Ai\u0307b")
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
