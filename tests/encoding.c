/**
 * The mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "test.h"

MJB_EXPORT void mjb_string_encoding_test(void) {
    mjb_encoding encoding = mjb_string_encoding(0, 10);
    mjb_assert("Void string", encoding == MJB_ENCODING_UNKNOWN);

    encoding = mjb_string_encoding("", 0);
    mjb_assert("Void length", encoding == MJB_ENCODING_UNKNOWN);

    encoding = mjb_string_encoding(0, 0);
    mjb_assert("Void string and length", encoding == MJB_ENCODING_UNKNOWN);

    const char *test1 = "The quick brown fox jumps over the lazy dog";
    encoding = mjb_string_encoding(test1, 43);
    mjb_assert("Plain ASCII (and UTF-8)", encoding == (MJB_ENCODING_ASCII |
        MJB_ENCODING_UTF_8));

    test1 = "\xEF\xBB\xBFThe quick brown fox jumps over the lazy dog";
    encoding = mjb_string_encoding(test1, 43 + 3);
    mjb_assert("UTF-8 BOM", encoding == MJB_ENCODING_UTF_8);

    test1 = "\xFE\xFFThe quick brown fox jumps over the lazy dog";
    encoding = mjb_string_encoding(test1, 43 + 2);
    mjb_assert("UTF-16-BE BOM", encoding == MJB_ENCODING_UTF_16_BE);

    test1 = "\xFF\xFEThe quick brown fox jumps over the lazy dog";
    encoding = mjb_string_encoding(test1, 43 + 2);
    mjb_assert("UTF-16-LE BOM", encoding == MJB_ENCODING_UTF_16_LE);

    test1 = "\x00\x00\xFE\xFFThe quick brown fox jumps over the lazy dog";

    encoding = mjb_string_encoding(test1, 43 + 4);
    mjb_assert("UTF-32-BE BOM", encoding == MJB_ENCODING_UTF_32_BE);

    test1 = "\xFF\xFE\x00\x00The quick brown fox jumps over the lazy dog";
    encoding = mjb_string_encoding(test1, 43 + 4);
    mjb_assert("UTF-32-LE BOM", encoding == (MJB_ENCODING_UTF_32_LE |
        MJB_ENCODING_UTF_16_LE));
}

MJB_EXPORT void mjb_string_is_utf8_test(void) {
    bool is_utf8 = mjb_string_is_utf8("", 0);
    mjb_assert("Void string", !is_utf8);

    is_utf8 = mjb_string_is_utf8("", 0);
    mjb_assert("Void length", !is_utf8);

    is_utf8 = mjb_string_is_utf8(0, 0);
    mjb_assert("Void string and length", !is_utf8);

    const char *test = "The quick brown fox jumps over the lazy dog";
    is_utf8 = mjb_string_is_utf8(test, 43);
    mjb_assert("Valid string and length", is_utf8);

    /* \xF0\x9F\x99\x82 = 🙂 */
    test = "The quick brown fox jumps over the lazy dog \xF0\x9F\x99\x82";
    is_utf8 = mjb_string_is_utf8(test, 48);
    mjb_assert("String with emoji", is_utf8);

    test = "The quick brown fox jumps over the lazy dog \xF0\x9F\x99\x82";
    is_utf8 = mjb_string_is_utf8(test, 48);
    mjb_assert("Not valid continuation byte", is_utf8);
}

MJB_EXPORT void mjb_string_is_ascii_test(void) {
    bool is_ascii = mjb_string_is_ascii("", 0);
    mjb_assert("Void string", !is_ascii);

    is_ascii = mjb_string_is_ascii("", 0);
    mjb_assert("Void length", !is_ascii);

    is_ascii = mjb_string_is_ascii(0, 0);
    mjb_assert("Void string and length", !is_ascii);

    const char *test = "The quick brown fox jumps over the lazy dog";
    is_ascii = mjb_string_is_ascii(test, 43);
    mjb_assert("Valid string and length", is_ascii);

    /* \xF0\x9F\x99\x82 = 🙂 */
    test = "\xF0\x9F\x99\x82";
    is_ascii = mjb_string_is_ascii(test, 5);
    mjb_assert("String with emoji", !is_ascii);

    test = "\x80";
    is_ascii = mjb_string_is_ascii(test, 2);
    mjb_assert("Lone continuation byte", !is_ascii);

    test = "\xC0";
    is_ascii = mjb_string_is_ascii(test, 2);
    mjb_assert("Lone first 2-bytes sequence", !is_ascii);

    test = "\xE0";
    is_ascii = mjb_string_is_ascii(test, 2);
    mjb_assert("Lone first 3-bytes sequence", !is_ascii);

    test = "\xF0";
    is_ascii = mjb_string_is_ascii(test, 2);
    mjb_assert("Lone first 4-bytes sequence", !is_ascii);
}
