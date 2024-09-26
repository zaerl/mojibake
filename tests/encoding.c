/**
 * The mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "../src/mojibake.h"
#include "test.h"

void *test_encoding(void *arg) {
    ATT_ASSERT(mjb_string_encoding(0, 10), MJB_ENCODING_UNKNOWN, "Void string")
    ATT_ASSERT(mjb_string_encoding("", 0), MJB_ENCODING_UNKNOWN, "Void length");
    ATT_ASSERT(mjb_string_encoding(0, 0), MJB_ENCODING_UNKNOWN, "Void string and length");

    const char *test1 = "The quick brown fox jumps over the lazy dog";
    ATT_ASSERT(mjb_string_encoding(test1, 43), (MJB_ENCODING_ASCII | MJB_ENCODING_UTF_8), "Plain ASCII (and UTF-8)");

    const char *test2 = "\xEF\xBB\xBFThe quick brown fox jumps over the lazy dog";
    ATT_ASSERT(mjb_string_encoding(test2, 43 + 3), MJB_ENCODING_UTF_8, "UTF-8 BOM");

    const char *test3 = "\xFE\xFFThe quick brown fox jumps over the lazy dog";
    ATT_ASSERT(mjb_string_encoding(test3, 43 + 2), MJB_ENCODING_UTF_16_BE, "UTF-16-BE BOM");

    const char *test4 = "\xFF\xFEThe quick brown fox jumps over the lazy dog";
    ATT_ASSERT(mjb_string_encoding(test4, 43 + 2), MJB_ENCODING_UTF_16_LE, "UTF-16-LE BOM");

    const char *test5 = "\x00\x00\xFE\xFFThe quick brown fox jumps over the lazy dog";
    ATT_ASSERT(mjb_string_encoding(test5, 43 + 4), MJB_ENCODING_UTF_32_BE, "UTF-32-BE BOM");

    const char *test6 = "\xFF\xFE\x00\x00The quick brown fox jumps over the lazy dog";
    ATT_ASSERT(mjb_string_encoding(test6, 43 + 4), (MJB_ENCODING_UTF_32_LE | MJB_ENCODING_UTF_16_LE), "UTF-32-LE BOM");

    ATT_ASSERT(mjb_string_is_utf8("", 0), false, "Void string");
    ATT_ASSERT(mjb_string_is_utf8("", 0), false, "Void length");
    ATT_ASSERT(mjb_string_is_utf8(0, 0), false, "Void string and length");

    const char *test7 = "The quick brown fox jumps over the lazy dog";
    ATT_ASSERT(mjb_string_is_utf8(test7, 43), true, "Valid string and length");

    // \xF0\x9F\x99\x82 = 🙂
    const char *test8 = "The quick brown fox jumps over the lazy dog \xF0\x9F\x99\x82";
    ATT_ASSERT(mjb_string_is_utf8(test8, 48), true, "String with emoji");

    const char *test9 = "The quick brown fox jumps over the lazy dog \xF0\x9F\x99\x82";
    ATT_ASSERT(mjb_string_is_utf8(test9, 48), true, "Not valid continuation byte");

    ATT_ASSERT(mjb_string_is_ascii("", 0), false, "Void string");
    ATT_ASSERT(mjb_string_is_ascii("", 0), false, "Void length");
    ATT_ASSERT(mjb_string_is_ascii(0, 0), false, "Void string and length");

    const char *test10 = "The quick brown fox jumps over the lazy dog";
    ATT_ASSERT(mjb_string_is_ascii(test10, 43), true, "Valid string and length");

    // \xF0\x9F\x99\x82 = 🙂
    const char *test11 = "\xF0\x9F\x99\x82";
    ATT_ASSERT(mjb_string_is_ascii(test11, 5), false, "String with emoji");

    const char *test12 = "\x80";
    ATT_ASSERT(mjb_string_is_ascii(test12, 2), false, "Lone continuation byte");

    const char *test13 = "\xC0";
    ATT_ASSERT(mjb_string_is_ascii(test13, 2), false, "Lone first 2-bytes sequence");

    const char *test14 = "\xE0";
    ATT_ASSERT(mjb_string_is_ascii(test14, 2), false, "Lone first 3-bytes sequence");

    const char *test15 = "\xF0";
    ATT_ASSERT(mjb_string_is_ascii(test15, 2), false, "Lone first 4-bytes sequence");

    return NULL;
}
