/**
 * The Mojibake library
 *
 * UTS#39 Unicode Security Mechanisms — confusable detection tests.
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "test.h"

void *test_security(void *arg) {
    // Cyrillic "А" (U+0410, UTF-8: 0xD0 0x90) is confusable with Latin "A"
    // Both have skeleton "A": skeleton("А")="A", skeleton("A")="A"
    ATT_ASSERT(mjb_string_is_confusable("\xD0\x90", 2, "A", 1, MJB_ENCODING_UTF_8), true,
        "Cyrillic A confusable with Latin A")

    // Cyrillic "а" (U+0430) is confusable with Latin "a" (U+0061)
    // skeleton("а")="a", skeleton("a")="a"
    ATT_ASSERT(mjb_string_is_confusable("\xD0\xB0", 2, "a", 1, MJB_ENCODING_UTF_8), true,
        "Cyrillic a confusable with Latin a")

    // "A" (Latin capital) is NOT confusable with "a" (Latin lowercase)
    // skeleton("A")="A", skeleton("a")="a" → different
    ATT_ASSERT(mjb_string_is_confusable("A", 1, "a", 1, MJB_ENCODING_UTF_8), false,
        "A not confusable with a")

    // "a" is not confusable with "b"
    ATT_ASSERT(mjb_string_is_confusable("a", 1, "b", 1, MJB_ENCODING_UTF_8), false,
        "a not confusable with b")

    // A string is confusable with itself
    ATT_ASSERT(mjb_string_is_confusable("hello", 5, "hello", 5, MJB_ENCODING_UTF_8), true,
        "hello confusable with itself")

    // "hello" vs "hеllo" (second 'e' is Cyrillic U+0435, UTF-8: 0xD0 0xB5)
    // skeleton("hello")="hello", skeleton("hеllo")="hello" → confusable
    ATT_ASSERT(mjb_string_is_confusable("hello", 5, "h\xD0\xB5llo", 6, MJB_ENCODING_UTF_8), true,
        "hello confusable with h(Cyrillic e)llo")

    // Empty strings: both have empty skeleton (output_size==0), returns false
    ATT_ASSERT(mjb_string_is_confusable("", 0, "", 0, MJB_ENCODING_UTF_8), false,
        "empty strings not confusable")

    // Different lengths with no possible match
    ATT_ASSERT(mjb_string_is_confusable("a", 1, "ab", 2, MJB_ENCODING_UTF_8), false,
        "a not confusable with ab")

    // Digit '1', capital 'I', and pipe '|' all map to skeleton 'l'
    ATT_ASSERT(mjb_string_is_confusable("1", 1, "l", 1, MJB_ENCODING_UTF_8), true,
        "1 confusable with l")
    ATT_ASSERT(mjb_string_is_confusable("I", 1, "l", 1, MJB_ENCODING_UTF_8), true,
        "I confusable with l")
    ATT_ASSERT(mjb_string_is_confusable("|", 1, "l", 1, MJB_ENCODING_UTF_8), true,
        "| confusable with l")
    ATT_ASSERT(mjb_string_is_confusable("1", 1, "I", 1, MJB_ENCODING_UTF_8), true,
        "1 confusable with I")

    // Digit '0' maps to skeleton 'O' (letter)
    ATT_ASSERT(mjb_string_is_confusable("0", 1, "O", 1, MJB_ENCODING_UTF_8), true,
        "0 confusable with O")
    ATT_ASSERT(mjb_string_is_confusable("0", 1, "o", 1, MJB_ENCODING_UTF_8), false,
        "0 not confusable with o (O != o)")

    // 'm' maps to a two-codepoint skeleton "rn": skeleton("m") == skeleton("rn")
    ATT_ASSERT(mjb_string_is_confusable("m", 1, "rn", 2, MJB_ENCODING_UTF_8), true,
        "m confusable with rn (multi-codepoint skeleton)")
    ATT_ASSERT(mjb_string_is_confusable("mm", 2, "rnrn", 4, MJB_ENCODING_UTF_8), true,
        "mm confusable with rnrn")

    // Cyrillic р (U+0440, UTF-8: 0xD1 0x80) maps to Latin p
    // "рaypal" (with Cyrillic р) is confusable with "paypal"
    ATT_ASSERT(mjb_string_is_confusable("\xD1\x80" "al", 4, "pal", 3, MJB_ENCODING_UTF_8), true,
        "Cyrillic p-aypal confusable with paypal")

    // Cyrillic С (U+0421, UTF-8: 0xD0 0xA1) maps to Latin C
    ATT_ASSERT(mjb_string_is_confusable("\xD0\xA1" "at", 4, "Cat", 3, MJB_ENCODING_UTF_8), true,
        "Cyrillic С + at confusable with Cat")

    // "gооgle" (Cyrillic о U+043E, UTF-8: 0xD0 0xBE) confusable with "google"
    // skeleton(Cyrillic о) = Latin o → both strings have skeleton "google"
    ATT_ASSERT(mjb_string_is_confusable("g\xD0\xBE\xD0\xBE" "d", 6, "good", 4, MJB_ENCODING_UTF_8), true,
        "g(Cyrillic o)(Cyrillic o)gle confusable with google")

    // Confusability is symmetric
    ATT_ASSERT(mjb_string_is_confusable("pal", 3, "\xD1\x80" "al", 4, MJB_ENCODING_UTF_8), true,
        "confusability is symmetric")

    return NULL;
}
