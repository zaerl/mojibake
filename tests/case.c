/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <string.h>

#include "test.h"

static int check_fold(const char *source, size_t source_size, const char *target,
    size_t target_size, mjb_case_type type, unsigned int current_line, const char *status) {
    char test_name[128];

    snprintf(test_name, 128, "CaseFolding #%u %s", current_line, status);

    if(source_size == 0 || target_size == 0) {
        return 0;
    }

    char *result = run_mjb_case(source, source_size, type, MJB_ENC_UTF_8);

    MJB_TEST_COVERAGE(mjb_case);
    ATT_ASSERT(result, (char*)target, test_name)

    if(result != NULL) {
        mjb_free(result);
    }

    return 0;
}

// Drive full (C + F), simple (C + S) and Turkic (T) case folding from CaseFolding.txt.
static void test_case_folding_file(void) {
    char line[1024];
    unsigned int current_line = 1;
    FILE *file = fopen("./utils/generate/unicode-data/UCD/CaseFolding.txt", "r");

    if(file == NULL) {
        ATT_ASSERT("Not opened", "Opened file", "Valid case folding test file")

        return;
    }

    char source[64];
    char target[64];

    while(fgets(line, 1024, file)) {
        if(line[0] == '#' || strnlen(line, 512) <= 1) {
            ++current_line;

            continue;
        }

        char *token, *string, *tofree;
        tofree = string = strdup(line);
        unsigned int field = 0;
        char status = 0;
        size_t source_size = 0;
        size_t target_size = 0;

        while((token = strsep(&string, ";")) != NULL) {
            if(field == 0) {
                source_size = get_string_from_codepoints(token, 64, source);
            } else if(field == 1) {
                while(*token == ' ') {
                    ++token;
                }

                status = token[0];
            } else if(field == 2) {
                target_size = get_string_from_codepoints(token, 64, target);
            }

            if(++field == 3) {
                break;
            }
        }

        free(tofree);

        switch(status) {
            case 'C': // Common: applies to both full and simple folding.
                check_fold(source, source_size, target, target_size, MJB_CASE_CASEFOLD,
                    current_line, "C full");
                check_fold(source, source_size, target, target_size, MJB_CASE_CASEFOLD_SIMPLE,
                    current_line, "C simple");
                break;

            case 'F': // Full folding only.
                check_fold(source, source_size, target, target_size, MJB_CASE_CASEFOLD,
                    current_line, "F full");
                break;

            case 'S': // Simple folding only.
                check_fold(source, source_size, target, target_size, MJB_CASE_CASEFOLD_SIMPLE,
                    current_line, "S simple");
                break;

            case 'T': // Turkic: applies to both folding types when the locale is tr or az.
                ATT_ASSERT_STATUS(mjb_locale_set(MJB_LOCALE_TR), MJB_STATUS_OK,
                    "Set locale tr")
                check_fold(source, source_size, target, target_size, MJB_CASE_CASEFOLD,
                    current_line, "T full");
                check_fold(source, source_size, target, target_size, MJB_CASE_CASEFOLD_SIMPLE,
                    current_line, "T simple");
                ATT_ASSERT_STATUS(mjb_locale_set(MJB_LOCALE_EN), MJB_STATUS_OK,
                    "Set locale en")
                break;
        }

        ++current_line;
    }

    fclose(file);
}

int test_case(void *arg) {
    mjb_encoding encoding = MJB_ENC_UTF_8;

    // Test case conversion functions
    MJB_TEST_COVERAGE(mjb_case);
    char *result = NULL;

    mjb_result guard_result = { NULL, 0, false };

    ATT_ASSERT_STATUS(mjb_case(NULL, 1, MJB_CASE_UPPER, encoding, encoding, &guard_result),
        MJB_STATUS_INVALID_ARGUMENT, "Case conversion rejects NULL buffer")
    ATT_ASSERT_STATUS(mjb_case("a", 1, MJB_CASE_UPPER, encoding, encoding, NULL),
        MJB_STATUS_INVALID_ARGUMENT, "Case conversion rejects NULL result")
    ATT_ASSERT_STATUS(mjb_case("a", 1, MJB_CASE_NONE, encoding, encoding, &guard_result),
        MJB_STATUS_INVALID_ARGUMENT, "Case conversion rejects MJB_CASE_NONE")

    ATT_ASSERT_STATUS(mjb_case("", 0, MJB_CASE_UPPER, encoding, encoding, &guard_result),
        MJB_STATUS_OK, "Case conversion accepts empty string")
    ATT_ASSERT(guard_result.transformed, false, "Case conversion empty string not transformed")
    ATT_ASSERT(guard_result.output_size, (size_t)0, "Case conversion empty string size")

    ATT_ASSERT_STATUS(mjb_case("a", 1, MJB_CASE_UPPER, encoding, MJB_ENC_UTF_16LE, &guard_result),
        MJB_STATUS_OK, "Case conversion converts output encoding")
    ATT_ASSERT(guard_result.transformed, true,
        "Case conversion converted output encoding transformed")
    ATT_ASSERT(guard_result.output_size, (size_t)2,
        "Case conversion converted output encoding size")
    ATT_ASSERT((int)memcmp(guard_result.output, "A\0", 2), 0,
        "Case conversion converted output encoding bytes")

    if(guard_result.transformed) {
        mjb_free(guard_result.output);
    }

    // Test uppercase conversion
    result = run_mjb_case("hello", 5, MJB_CASE_UPPER, encoding);
    ATT_ASSERT(result, (char*)"HELLO", "UTF-8 uppercase: hello")
    mjb_free(result);

    result = run_mjb_case("héllö", 7, MJB_CASE_UPPER, encoding);
    ATT_ASSERT(result, (char*)"HÉLLÖ", "UTF-8 uppercase: héllö")
    mjb_free(result);

    result = run_mjb_case("a𠀀b", 6, MJB_CASE_UPPER, encoding);
    ATT_ASSERT(result, (char*)"A𠀀B", "UTF-8 uppercase preserves uncased CJK")
    mjb_free(result);

    // Test lowercase conversion
    result = run_mjb_case("HELLO", 5, MJB_CASE_LOWER, encoding);
    ATT_ASSERT(result, (char*)"hello", "UTF-8 lowercase: HELLO")
    mjb_free(result);

    result = run_mjb_case("HÉLLÖ", 7, MJB_CASE_LOWER, encoding);
    ATT_ASSERT(result, (char*)"héllö", "UTF-8 lowercase: HÉLLÖ")
    mjb_free(result);

    result = run_mjb_case("A𠀀B", 6, MJB_CASE_LOWER, encoding);
    ATT_ASSERT(result, (char*)"a𠀀b", "UTF-8 lowercase preserves uncased CJK")
    mjb_free(result);

    // Test titlecase conversion
    result = run_mjb_case("hello world", 11, MJB_CASE_TITLE, encoding);
    ATT_ASSERT(result, (char*)"Hello World", "UTF-8 titlecase: hello world")
    mjb_free(result);

    result = run_mjb_case("héllö wörld", 14, MJB_CASE_TITLE, encoding);
    ATT_ASSERT(result, (char*)"Héllö Wörld", "UTF-8 titlecase: héllö wörld")
    mjb_free(result);

    result = run_mjb_case("a 𠀀 b", 8, MJB_CASE_TITLE, encoding);
    ATT_ASSERT(result, (char*)"A 𠀀 B", "UTF-8 titlecase preserves uncased CJK")
    mjb_free(result);

    result = run_mjb_case("hello world", 11, MJB_CASE_TITLE, encoding);
    ATT_ASSERT(result, (char*)"Hello World", "UTF-8 titlecase: hello world")
    mjb_free(result);

    result = run_mjb_case("HELLO WORLD", 11, MJB_CASE_TITLE, encoding);
    ATT_ASSERT(result, (char*)"Hello World", "UTF-8 titlecase: HELLO WORLD")
    mjb_free(result);

    result = run_mjb_case("HELLO WORLD", 11, MJB_CASE_TITLE, encoding);
    ATT_ASSERT(result, (char*)"Hello World", "UTF-8 titlecase: HELLO WORLD")
    mjb_free(result);

    result = run_mjb_case("mixed CASE words", 17, MJB_CASE_TITLE, encoding);
    ATT_ASSERT(result, (char*)"Mixed Case Words", "UTF-8 titlecase: mixed CASE words")
    mjb_free(result);

    result = run_mjb_case("  leading space", 15, MJB_CASE_TITLE, encoding);
    ATT_ASSERT(result, (char*)"  Leading Space", "UTF-8 titlecase:   leading space")
    mjb_free(result);

    result = run_mjb_case("élan vital", 11, MJB_CASE_TITLE, encoding);
    ATT_ASSERT(result, (char*)"Élan Vital", "UTF-8 titlecase: élan vital")
    mjb_free(result);

    result = run_mjb_case("straße", 7, MJB_CASE_TITLE, encoding);
    ATT_ASSERT(result, (char*)"Straße", "UTF-8 titlecase: straße")
    mjb_free(result);

    result = run_mjb_case("παράδειγμα", 20, MJB_CASE_TITLE, encoding);
    ATT_ASSERT(result, (char*)"Παράδειγμα", "UTF-8 titlecase: παράδειγμα")
    mjb_free(result);

    result = run_mjb_case("ⅲ times", 10, MJB_CASE_TITLE, encoding);
    ATT_ASSERT(result, (char*)"Ⅲ Times", "UTF-8 titlecase: ⅲ times")
    mjb_free(result);

    // TODO: add support for WordBreakProperty.txt
    // See: https://www.unicode.org/reports/tr29/#Word_Boundaries
    // 2019..2019    MidLetter # Po  RIGHT SINGLE QUOTATION MARK
    // WB6: ALetter × MidLetter ALetter
    // WB7: ALetter MidLetter × ALetter
    // result = run_mjb_case("o’connor", 9, MJB_CASE_TITLE, encoding);
    // ATT_ASSERT(result, "O’Connor", "UTF-8 titlecase: o’connor")
    // mjb_free(result);

    result = run_mjb_case("İstanbul", 9, MJB_CASE_TITLE, encoding);
    ATT_ASSERT(result, (char*)"İstanbul", "UTF-8 titlecase: İstanbul")
    mjb_free(result);

    // Modern German orthography sometimes prefers the uppercase form ẞ (U+1E9E) in all-caps or
    // titlecase contexts. Unicode's default case folding still maps ß to SS in titlecase unless
    // locale-specific tailoring is applied.
    result = run_mjb_case("ßeta", 5, MJB_CASE_UPPER, encoding);
    ATT_ASSERT(result, (char*)"SSETA", "UTF-8 titlecase: ßeta")
    mjb_free(result);

    result = run_mjb_case("coöperate", 10, MJB_CASE_TITLE, encoding);
    ATT_ASSERT(result, (char*)"Coöperate", "UTF-8 titlecase: Český Krumlov")
    mjb_free(result);

    result = run_mjb_case("😀grinning", 12, MJB_CASE_TITLE, encoding);
    ATT_ASSERT(result, (char*)"😀Grinning", "UTF-8 titlecase: 😀grinning")
    mjb_free(result);

    result = run_mjb_case("123abc", 8, MJB_CASE_TITLE, encoding);
    ATT_ASSERT(result, (char*)"123Abc", "UTF-8 titlecase: 123abc")
    mjb_free(result);

    // Test casefold: ASCII uppercase via unicode_data.lowercase fallback
    result = run_mjb_case("ABC", 3, MJB_CASE_CASEFOLD, encoding);
    ATT_ASSERT(result, (char*)"abc", "Casefold: ABC -> abc")
    mjb_free(result);

    result = run_mjb_case("Hello World", 11, MJB_CASE_CASEFOLD, encoding);
    ATT_ASSERT(result, (char*)"hello world", "Casefold: Hello World -> hello world")
    mjb_free(result);

    // Test casefold: ß (U+00DF) -> ss, F entry (multi-char expansion)
    result = run_mjb_case("ß", 2, MJB_CASE_CASEFOLD, encoding);
    ATT_ASSERT(result, (char*)"ss", "Casefold: ß -> ss")
    mjb_free(result);

    result = run_mjb_case("straße", 7, MJB_CASE_CASEFOLD, encoding);
    ATT_ASSERT(result, (char*)"strasse", "Casefold: straße -> strasse")
    mjb_free(result);

    // Test casefold: µ (U+00B5 MICRO SIGN) -> μ (U+03BC), C exception entry
    result = run_mjb_case("µ", 2, MJB_CASE_CASEFOLD, encoding);
    ATT_ASSERT(result, (char*)"μ", "Casefold: µ (U+00B5) -> μ (U+03BC)")
    mjb_free(result);

    // Test casefold: ﬃ (U+FB03) -> ffi, F entry with 3-codepoint expansion
    result = run_mjb_case("ﬃ", 3, MJB_CASE_CASEFOLD, encoding);
    ATT_ASSERT(result, (char*)"ffi", "Casefold: ﬃ -> ffi")
    mjb_free(result);

    // Test casefold: Greek uppercase via unicode_data.lowercase fallback
    result = run_mjb_case("Σ", 2, MJB_CASE_CASEFOLD, encoding);
    ATT_ASSERT(result, (char*)"σ", "Casefold: Σ -> σ")
    mjb_free(result);

    // Test casefold: digits/symbols pass through unchanged (identity path)
    result = run_mjb_case("123", 3, MJB_CASE_CASEFOLD, encoding);
    ATT_ASSERT(result, (char*)"123", "Casefold: 123 -> 123")
    mjb_free(result);

    // Simple case folding (C + S statuses): multi-char full folds are not applied.
    result = run_mjb_case("straße", 7, MJB_CASE_CASEFOLD_SIMPLE, encoding);
    ATT_ASSERT(result, (char*)"straße", "Simple casefold: ß folds to itself")
    mjb_free(result);

    result = run_mjb_case("ẞ", 3, MJB_CASE_CASEFOLD_SIMPLE, encoding);
    ATT_ASSERT(result, (char*)"ß", "Simple casefold: ẞ -> ß")
    mjb_free(result);

    result = run_mjb_case("ﬃ", 3, MJB_CASE_CASEFOLD_SIMPLE, encoding);
    ATT_ASSERT(result, (char*)"ﬃ", "Simple casefold: ﬃ folds to itself")
    mjb_free(result);

    result = run_mjb_case("İ", 2, MJB_CASE_CASEFOLD_SIMPLE, encoding);
    ATT_ASSERT(result, (char*)"İ", "Simple casefold: İ folds to itself")
    mjb_free(result);

    // Turkic (T) case folding, active for the tr and az locales.
    ATT_ASSERT_STATUS(mjb_locale_set(MJB_LOCALE_TR), MJB_STATUS_OK, "Set locale tr")

    result = run_mjb_case("KIRMIZI", 7, MJB_CASE_CASEFOLD, encoding);
    ATT_ASSERT(result, (char*)"kırmızı", "Turkic casefold: KIRMIZI -> kırmızı")
    mjb_free(result);

    result = run_mjb_case("İZMİR", 7, MJB_CASE_CASEFOLD, encoding);
    ATT_ASSERT(result, (char*)"izmir", "Turkic casefold: İZMİR -> izmir")
    mjb_free(result);

    result = run_mjb_case("I", 1, MJB_CASE_CASEFOLD_SIMPLE, encoding);
    ATT_ASSERT(result, (char*)"ı", "Turkic simple casefold: I -> ı")
    mjb_free(result);

    ATT_ASSERT_STATUS(mjb_locale_set(MJB_LOCALE_EN), MJB_STATUS_OK, "Set locale en")

    // Test Final_Sigma rule: word-final Σ → ς, non-final Σ → σ
    result = run_mjb_case("ΣΕΙΣ", 8, MJB_CASE_LOWER, encoding);
    ATT_ASSERT(result, (char*)"σεις", "UTF-8 lowercase Final_Sigma: ΣΕΙΣ -> σεις")
    mjb_free(result);

    result = run_mjb_case("ΑΣΑ", 6, MJB_CASE_LOWER, encoding);
    ATT_ASSERT(result, (char*)"ασα", "UTF-8 lowercase non-final sigma: ΑΣΑ -> ασα")
    mjb_free(result);

    result = run_mjb_case("ΣΕΙΣ", 8, MJB_CASE_TITLE, encoding);
    ATT_ASSERT(result, (char*)"Σεις", "UTF-8 titlecase Final_Sigma: ΣΕΙΣ -> Σεις")
    mjb_free(result);

    // Test that titlecase uses original codepoint for special-casing lookup (Fix 2):
    // In-word İ (U+0130) must lower to "i + U+0307 (combining dot above)", not bare "i".
    result = run_mjb_case("A\xC4\xB0""B", 4, MJB_CASE_TITLE, encoding);
    ATT_ASSERT(result, (char*)"Ai\xCC\x87""b",
        "UTF-8 titlecase special casing: A\\xC4\\xB0B -> Ai\\xCC\\x87b")
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

    test_case_folding_file();

    return 0;
}
