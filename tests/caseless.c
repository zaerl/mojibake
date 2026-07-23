/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "test.h"

static void check_match(const char *s1, size_t s1_byte_length, mjb_encoding s1_encoding,
    const char *s2, size_t s2_byte_length, mjb_encoding s2_encoding, mjb_caseless_mode mode,
    bool expected, const char *name) {
    bool matches = !expected;

    MJB_TEST_COVERAGE(mjb_caseless_match);
    ATT_ASSERT_STATUS(mjb_caseless_match(s1, s1_byte_length, s1_encoding, s2, s2_byte_length,
                          s2_encoding, mode, &matches),
        MJB_STATUS_OK, name)
    ATT_ASSERT(matches, expected, name)
}

int test_caseless(void *arg) {
    bool matches = true;

    ATT_ASSERT_STATUS(mjb_caseless_match("a", 1, MJB_ENC_UTF_8, "A", 1, MJB_ENC_UTF_8,
                          MJB_CASELESS_CANONICAL, NULL),
        MJB_STATUS_INVALID_ARGUMENT, "Caseless match rejects a NULL result")
    ATT_ASSERT_STATUS(mjb_caseless_match(NULL, 1, MJB_ENC_UTF_8, "A", 1, MJB_ENC_UTF_8,
                          MJB_CASELESS_CANONICAL, &matches),
        MJB_STATUS_INVALID_ARGUMENT, "Caseless match rejects a NULL first input")
    ATT_ASSERT(matches, false, "Caseless match clears its result after invalid first input")

    matches = true;
    ATT_ASSERT_STATUS(mjb_caseless_match("a", 1, MJB_ENC_UTF_8, NULL, 1, MJB_ENC_UTF_8,
                          MJB_CASELESS_CANONICAL, &matches),
        MJB_STATUS_INVALID_ARGUMENT, "Caseless match rejects a NULL second input")
    ATT_ASSERT(matches, false, "Caseless match clears its result after invalid second input")

    matches = true;
    ATT_ASSERT_STATUS(mjb_caseless_match("a", 1, MJB_ENC_UTF_8, "A", 1, MJB_ENC_UTF_8,
                          (mjb_caseless_mode)99, &matches),
        MJB_STATUS_INVALID_ARGUMENT, "Caseless match rejects an invalid mode")
    ATT_ASSERT(matches, false, "Caseless match clears its result after an invalid mode")

    matches = true;
    ATT_ASSERT_STATUS(mjb_caseless_match("\x80", 1, MJB_ENC_UTF_8, "a", 1, MJB_ENC_UTF_8,
                          MJB_CASELESS_UNNORMALIZED, &matches),
        MJB_STATUS_MALFORMED_INPUT, "Caseless match rejects malformed first input")
    ATT_ASSERT(matches, false, "Caseless match clears its result after malformed first input")

    matches = true;
    ATT_ASSERT_STATUS(mjb_caseless_match("a", 1, MJB_ENC_UTF_8, "\x80", 1, MJB_ENC_UTF_8,
                          MJB_CASELESS_UNNORMALIZED, &matches),
        MJB_STATUS_MALFORMED_INPUT, "Caseless match rejects malformed second input")
    ATT_ASSERT(matches, false, "Caseless match clears its result after malformed second input")

    const char utf16_without_bom[] = { 'A', '\0' };
    ATT_ASSERT_STATUS(mjb_caseless_match(utf16_without_bom, sizeof(utf16_without_bom),
                          MJB_ENC_UTF_16, "a", 1, MJB_ENC_UTF_8, MJB_CASELESS_CANONICAL, &matches),
        MJB_STATUS_INVALID_ENCODING, "Caseless match rejects generic UTF-16 without a BOM")

    check_match(NULL, 0, MJB_ENC_UTF_8, "", 0, MJB_ENC_UTF_8, MJB_CASELESS_CANONICAL, true,
        "Empty strings are a canonical caseless match");
    check_match("a", MJB_NUL_TERMINATED, MJB_ENC_UTF_8, "A", MJB_NUL_TERMINATED, MJB_ENC_UTF_8,
        MJB_CASELESS_CANONICAL, true, "Caseless match accepts NUL-terminated input");

    check_match("Stra\xC3\x9F"
                "e",
        7, MJB_ENC_UTF_8, "STRASSE", 7, MJB_ENC_UTF_8, MJB_CASELESS_UNNORMALIZED, true,
        "Full case folding matches sharp s with SS");

    check_match("\xC3\x85", 2, MJB_ENC_UTF_8, "A\xCC\x8A", 3, MJB_ENC_UTF_8,
        MJB_CASELESS_UNNORMALIZED, false,
        "Unnormalized caseless matching preserves canonical representation differences");
    check_match("\xC3\x85", 2, MJB_ENC_UTF_8, "A\xCC\x8A", 3, MJB_ENC_UTF_8, MJB_CASELESS_CANONICAL,
        true, "Canonical caseless matching removes canonical representation differences");

    check_match("A\xCC\x81\xCC\xA7", 5, MJB_ENC_UTF_8, "a\xCC\xA7\xCC\x81", 5, MJB_ENC_UTF_8,
        MJB_CASELESS_UNNORMALIZED, false,
        "Unnormalized caseless matching preserves combining-mark order");
    check_match("A\xCC\x81\xCC\xA7", 5, MJB_ENC_UTF_8, "a\xCC\xA7\xCC\x81", 5, MJB_ENC_UTF_8,
        MJB_CASELESS_CANONICAL, true, "Canonical caseless matching reorders combining marks");

    check_match("\xE3\x8E\x92", 3, MJB_ENC_UTF_8, "MHz", 3, MJB_ENC_UTF_8, MJB_CASELESS_CANONICAL,
        false, "Canonical caseless matching preserves compatibility distinctions");
    check_match("\xE3\x8E\x92", 3, MJB_ENC_UTF_8, "MHz", 3, MJB_ENC_UTF_8,
        MJB_CASELESS_COMPATIBILITY, true,
        "Compatibility caseless matching folds letters introduced by decomposition");

    check_match("ab\xC2\xAD", 4, MJB_ENC_UTF_8, "ab", 2, MJB_ENC_UTF_8, MJB_CASELESS_COMPATIBILITY,
        false, "Compatibility caseless matching preserves default-ignorables");
    check_match("ab\xC2\xAD", 4, MJB_ENC_UTF_8, "ab", 2, MJB_ENC_UTF_8, MJB_CASELESS_IDENTIFIER,
        true, "Identifier caseless matching removes default-ignorables");
    check_match("", 0, MJB_ENC_UTF_8, "\xC2\xAD", 2, MJB_ENC_UTF_8, MJB_CASELESS_IDENTIFIER, true,
        "Identifier caseless matching can map non-empty input to empty");

    const char embedded_nul_upper[] = { 'A', '\0', 'B' };
    const char embedded_nul_lower[] = { 'a', '\0', 'b' };
    check_match(embedded_nul_upper, sizeof(embedded_nul_upper), MJB_ENC_UTF_8, embedded_nul_lower,
        sizeof(embedded_nul_lower), MJB_ENC_UTF_8, MJB_CASELESS_CANONICAL, true,
        "Caseless match processes embedded NUL codepoints");

    const char utf16le_strasse[] = { 'S', '\0', 'T', '\0', 'R', '\0', 'A', '\0', 'S', '\0', 'S',
        '\0', 'E', '\0' };
    check_match("Stra\xC3\x9F"
                "e",
        7, MJB_ENC_UTF_8, utf16le_strasse, sizeof(utf16le_strasse), MJB_ENC_UTF_16LE,
        MJB_CASELESS_CANONICAL, true, "Caseless match compares different input encodings");

    ATT_ASSERT_STATUS(mjb_set_locale(MJB_LOCALE_TR), MJB_STATUS_OK,
        "Set Turkish locale for caseless matching")
    check_match("I", 1, MJB_ENC_UTF_8, "i", 1, MJB_ENC_UTF_8, MJB_CASELESS_UNNORMALIZED, true,
        "Caseless matching always uses default non-Turkic folding");
    check_match("I", 1, MJB_ENC_UTF_8, "\xC4\xB1", 2, MJB_ENC_UTF_8, MJB_CASELESS_UNNORMALIZED,
        false, "Caseless matching does not use Turkic folding");
    ATT_ASSERT_STATUS(mjb_set_locale(MJB_LOCALE_EN), MJB_STATUS_OK,
        "Restore default locale after caseless matching")

    return 0;
}
