/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <string.h>

#include "test.h"

void *test_string(void *arg) {
    mjb_encoding encoding = MJB_ENCODING_UTF_8;

    ATT_ASSERT(mjb_strnlen("Hello", 5, encoding), 5, "UTF-8 length: Hello")
    ATT_ASSERT(mjb_strnlen("Hello", 4, encoding), 4, "UTF-8 length: Hello")
    ATT_ASSERT(mjb_strnlen("Hello", 3, encoding), 3, "UTF-8 length: Hello")
    ATT_ASSERT(mjb_strnlen("Hello", 2, encoding), 2, "UTF-8 length: Hello")
    ATT_ASSERT(mjb_strnlen("Hello", 1, encoding), 1, "UTF-8 length: Hello")
    ATT_ASSERT(mjb_strnlen("Hello", 0, encoding), 0, "UTF-8 length: Hello")
    ATT_ASSERT(mjb_strnlen(NULL, 0, encoding), 0, "UTF-8 length: NULL")

    ATT_ASSERT(mjb_strnlen("Héllö", 7, encoding), 5, "UTF-8 length: Héllö")
    ATT_ASSERT(mjb_strnlen("Héllö", 4, encoding), 3, "UTF-8 length: Héllö")
    ATT_ASSERT(mjb_strnlen("Héllö", 2, encoding), 1, "UTF-8 length: Héllö")
    ATT_ASSERT(mjb_strnlen("Héllö", 0, encoding), 0, "UTF-8 length: Héllö")
    ATT_ASSERT(mjb_strnlen("Hèllõ ツ", 11, encoding), 7, "UTF-8 length: Hèllõ ツ")
    ATT_ASSERT(mjb_strnlen("Hèllõ ツ", 5, encoding), 4, "UTF-8 length: Hèllõ ツ")
    ATT_ASSERT(mjb_strnlen("こんにちは", 15, encoding), 5, "UTF-8 length: こんにちは")
    ATT_ASSERT(mjb_strnlen("Γειά σου", 15, encoding), 8, "UTF-8 length: Γειά σου")
    ATT_ASSERT(mjb_strnlen("Héllö", 1, encoding), 1, "UTF-8 length: Héllö (1 max value)")

    encoding = MJB_ENCODING_UTF_16_LE;
    const char utf16le_hello[] = "H\0e\0l\0l\0o\0";
    ATT_ASSERT(mjb_strnlen(utf16le_hello, 10, encoding), 5, "UTF-16LE length: Hello")
    ATT_ASSERT(mjb_strnlen(utf16le_hello, 8, encoding), 4, "UTF-16LE length: Hello")
    ATT_ASSERT(mjb_strnlen(utf16le_hello, 6, encoding), 3, "UTF-16LE length: Hello")
    ATT_ASSERT(mjb_strnlen(utf16le_hello, 4, encoding), 2, "UTF-16LE length: Hello")
    ATT_ASSERT(mjb_strnlen(utf16le_hello, 2, encoding), 1, "UTF-16LE length: Hello")
    ATT_ASSERT(mjb_strnlen(utf16le_hello, 0, encoding), 0, "UTF-16LE length: Hello")
    ATT_ASSERT(mjb_strnlen(NULL, 0, encoding), 0, "UTF-16LE length: NULL")

    const char utf16le_hello_accents[] = "H\0\xE9\0l\0l\0\xF6\0";
    ATT_ASSERT(mjb_strnlen(utf16le_hello_accents, 10, encoding), 5, "UTF-16LE length: Héllö")
    ATT_ASSERT(mjb_strnlen(utf16le_hello_accents, 8, encoding), 4, "UTF-16LE length: Héllö")
    ATT_ASSERT(mjb_strnlen(utf16le_hello_accents, 4, encoding), 2, "UTF-16LE length: Héllö")
    ATT_ASSERT(mjb_strnlen(utf16le_hello_accents, 0, encoding), 0, "UTF-16LE length: Héllö")
    const char utf16le_hello_katakana[] = "H\0\xE8\0l\0l\0\xF5\0 \0\x30\x30";
    ATT_ASSERT(mjb_strnlen(utf16le_hello_katakana, 14, encoding), 7, "UTF-16LE length: Hèllõ ツ")
    ATT_ASSERT(mjb_strnlen(utf16le_hello_katakana, 10, encoding), 5, "UTF-16LE length: Hèllõ ツ")
    const char utf16le_konnichiwa[] = "\x93\x30\x93\x30\x6B\x30\x61\x30\x6F\x30";
    ATT_ASSERT(mjb_strnlen(utf16le_konnichiwa, 10, encoding), 5, "UTF-16LE length: こんにちは")
    const char utf16le_geia_sou[] = "\x93\x03\x65\x03\x69\x03\x3F\x03 \0\xC3\x03\x6F\x03\x75\x03";
    ATT_ASSERT(mjb_strnlen(utf16le_geia_sou, 16, encoding), 8, "UTF-16LE length: Γειά σου")
    ATT_ASSERT(mjb_strnlen(utf16le_hello_accents, 2, encoding), 1, "UTF-16LE length: Héllö (1 max value)")

    encoding = MJB_ENCODING_UTF_16_BE;
    const char utf16be_hello[] = "\0H\0e\0l\0l\0o";
    ATT_ASSERT(mjb_strnlen(utf16be_hello, 10, encoding), 5, "UTF-16BE length: Hello")
    ATT_ASSERT(mjb_strnlen(utf16be_hello, 8, encoding), 4, "UTF-16BE length: Hello")
    ATT_ASSERT(mjb_strnlen(utf16be_hello, 6, encoding), 3, "UTF-16BE length: Hello")
    ATT_ASSERT(mjb_strnlen(utf16be_hello, 4, encoding), 2, "UTF-16BE length: Hello")
    ATT_ASSERT(mjb_strnlen(utf16be_hello, 2, encoding), 1, "UTF-16BE length: Hello")
    ATT_ASSERT(mjb_strnlen(utf16be_hello, 0, encoding), 0, "UTF-16BE length: Hello")
    ATT_ASSERT(mjb_strnlen(NULL, 0, encoding), 0, "UTF-16BE length: NULL")

    const char utf16be_hello_accents[] = "\0H\0\xE9\0l\0l\0\xF6";
    ATT_ASSERT(mjb_strnlen(utf16be_hello_accents, 10, encoding), 5, "UTF-16BE length: Héllö")
    ATT_ASSERT(mjb_strnlen(utf16be_hello_accents, 8, encoding), 4, "UTF-16BE length: Héllö")
    ATT_ASSERT(mjb_strnlen(utf16be_hello_accents, 4, encoding), 2, "UTF-16BE length: Héllö")
    ATT_ASSERT(mjb_strnlen(utf16be_hello_accents, 0, encoding), 0, "UTF-16BE length: Héllö")
    const char utf16be_hello_katakana[] = "\0H\0\xE8\0l\0l\0\xF5\0 \x30\x30";
    ATT_ASSERT(mjb_strnlen(utf16be_hello_katakana, 14, encoding), 7, "UTF-16BE length: Hèllõ ツ")
    ATT_ASSERT(mjb_strnlen(utf16be_hello_katakana, 10, encoding), 5, "UTF-16BE length: Hèllõ ツ")
    const char utf16be_konnichiwa[] = "\x30\x93\x30\x93\x30\x6B\x30\x61\x30\x6F";
    ATT_ASSERT(mjb_strnlen(utf16be_konnichiwa, 10, encoding), 5, "UTF-16BE length: こんにちは")
    const char utf16be_geia_sou[] = "\x03\x93\x03\x65\x03\x69\x03\x3F\0 \x03\xC3\x03\x6F\x03\x75";
    ATT_ASSERT(mjb_strnlen(utf16be_geia_sou, 16, encoding), 8, "UTF-16BE length: Γειά σου")
    ATT_ASSERT(mjb_strnlen(utf16be_hello_accents, 2, encoding), 1, "UTF-16BE length: Héllö (1 max value)")

    // Test case conversion functions
    // CURRENT_ASSERT mjb_case
    // CURRENT_COUNT 20
    char *result = NULL;

    // Test uppercase conversion
    result = mjb_case("hello", 5, MJB_CASE_UPPER, MJB_ENCODING_UTF_8);
    ATT_ASSERT(result, "HELLO", "UTF-8 uppercase: hello")
    mjb_free(result);

    result = mjb_case("héllö", 7, MJB_CASE_UPPER, MJB_ENCODING_UTF_8);
    ATT_ASSERT(result, "HÉLLÖ", "UTF-8 uppercase: héllö")
    mjb_free(result);

    // Test lowercase conversion
    result = mjb_case("HELLO", 5, MJB_CASE_LOWER, MJB_ENCODING_UTF_8);
    ATT_ASSERT(result, "hello", "UTF-8 lowercase: HELLO")
    mjb_free(result);

    result = mjb_case("HÉLLÖ", 7, MJB_CASE_LOWER, MJB_ENCODING_UTF_8);
    ATT_ASSERT(result, "héllö", "UTF-8 lowercase: HÉLLÖ")
    mjb_free(result);

    // Test titlecase conversion
    result = mjb_case("hello world", 11, MJB_CASE_TITLE, MJB_ENCODING_UTF_8);
    ATT_ASSERT(result, "Hello World", "UTF-8 titlecase: hello world")
    mjb_free(result);

    result = mjb_case("héllö wörld", 14, MJB_CASE_TITLE, MJB_ENCODING_UTF_8);
    ATT_ASSERT(result, "Héllö Wörld", "UTF-8 titlecase: héllö wörld")
    mjb_free(result);

    result = mjb_case("hello world", 11, MJB_CASE_TITLE, MJB_ENCODING_UTF_8);
    ATT_ASSERT(result, "Hello World", "UTF-8 titlecase: hello world")
    mjb_free(result);

    result = mjb_case("HELLO WORLD", 11, MJB_CASE_TITLE, MJB_ENCODING_UTF_8);
    ATT_ASSERT(result, "Hello World", "UTF-8 titlecase: HELLO WORLD")
    mjb_free(result);

    result = mjb_case("HELLO WORLD", 11, MJB_CASE_TITLE, MJB_ENCODING_UTF_8);
    ATT_ASSERT(result, "Hello World", "UTF-8 titlecase: HELLO WORLD")
    mjb_free(result);

    result = mjb_case("mixed CASE words", 17, MJB_CASE_TITLE, MJB_ENCODING_UTF_8);
    ATT_ASSERT(result, "Mixed Case Words", "UTF-8 titlecase: mixed CASE words")
    mjb_free(result);

    result = mjb_case("  leading space", 15, MJB_CASE_TITLE, MJB_ENCODING_UTF_8);
    ATT_ASSERT(result, "  Leading Space", "UTF-8 titlecase:   leading space")
    mjb_free(result);

    result = mjb_case("élan vital", 11, MJB_CASE_TITLE, MJB_ENCODING_UTF_8);
    ATT_ASSERT(result, "Élan Vital", "UTF-8 titlecase: élan vital")
    mjb_free(result);

    result = mjb_case("straße", 7, MJB_CASE_TITLE, MJB_ENCODING_UTF_8);
    ATT_ASSERT(result, "Straße", "UTF-8 titlecase: straße")
    mjb_free(result);

    result = mjb_case("παράδειγμα", 20, MJB_CASE_TITLE, MJB_ENCODING_UTF_8);
    ATT_ASSERT(result, "Παράδειγμα", "UTF-8 titlecase: παράδειγμα")
    mjb_free(result);

    result = mjb_case("ⅲ times", 10, MJB_CASE_TITLE, MJB_ENCODING_UTF_8);
    ATT_ASSERT(result, "Ⅲ Times", "UTF-8 titlecase: ⅲ times")
    mjb_free(result);

    // TODO: add support for WordBreakProperty.txt
    // See: https://www.unicode.org/reports/tr29/#Word_Boundaries
    // 2019..2019    MidLetter # Po  RIGHT SINGLE QUOTATION MARK
    // WB6: ALetter × MidLetter ALetter
    // WB7: ALetter MidLetter × ALetter
    // result = mjb_case("o’connor", 9, MJB_CASE_TITLE, MJB_ENCODING_UTF_8);
    // ATT_ASSERT(result, "O’Connor", "UTF-8 titlecase: o’connor")
    // mjb_free(result);

    result = mjb_case("İstanbul", 9, MJB_CASE_TITLE, MJB_ENCODING_UTF_8);
    ATT_ASSERT(result, "İstanbul", "UTF-8 titlecase: İstanbul")
    mjb_free(result);

    // TODO: add support for SpecialCasing.txt
    // Modern German orthography sometimes prefers the uppercase form ẞ (U+1E9E) in all-caps or titlecase contexts.
    // Unicode's default case folding still maps ß to SS in titlecase unless locale-specific tailoring is applied.
    result = mjb_case("ßeta", 5, MJB_CASE_UPPER, MJB_ENCODING_UTF_8);
    ATT_ASSERT(result, "SSETA", "UTF-8 titlecase: ßeta")
    mjb_free(result);

    result = mjb_case("coöperate", 10, MJB_CASE_TITLE, MJB_ENCODING_UTF_8);
    ATT_ASSERT(result, "Coöperate", "UTF-8 titlecase: Český Krumlov")
    mjb_free(result);

    result = mjb_case("😀grinning", 12, MJB_CASE_TITLE, MJB_ENCODING_UTF_8);
    ATT_ASSERT(result, "😀Grinning", "UTF-8 titlecase: 😀grinning")
    mjb_free(result);

    result = mjb_case("123abc", 8, MJB_CASE_TITLE, MJB_ENCODING_UTF_8);
    ATT_ASSERT(result, "123Abc", "UTF-8 titlecase: 123abc")
    mjb_free(result);

    return NULL;
 }
