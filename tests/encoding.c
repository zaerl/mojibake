#include "test.h"

MB_EXPORT void mb_string_encoding_test() {
    mb_encoding encoding = mb_string_encoding(0, 10);
    mb_assert("Void string", encoding == MB_ENCODING_UNKNOWN);

    encoding = mb_string_encoding("", 0);
    mb_assert("Void length", encoding == MB_ENCODING_UNKNOWN);

    encoding = mb_string_encoding(0, 0);
    mb_assert("Void string and length", encoding == MB_ENCODING_UNKNOWN);

    const char *test1 = "The quick brown fox jumps over the lazy dog";
    encoding = mb_string_encoding(test1, 43);
    mb_assert("Plain ASCII (and UTF-8)", encoding == (MB_ENCODING_ASCII |
        MB_ENCODING_UTF_8));

    test1 = "\xEF\xBB\xBFThe quick brown fox jumps over the lazy dog";
    encoding = mb_string_encoding(test1, 43 + 3);
    mb_assert("UTF-8 BOM", encoding == MB_ENCODING_UTF_8);

    test1 = "\xFE\xFFThe quick brown fox jumps over the lazy dog";
    encoding = mb_string_encoding(test1, 43 + 2);
    mb_assert("UTF-16-BE BOM", encoding == MB_ENCODING_UTF_16_BE);

    test1 = "\xFF\xFEThe quick brown fox jumps over the lazy dog";
    encoding = mb_string_encoding(test1, 43 + 2);
    mb_assert("UTF-16-LE BOM", encoding == MB_ENCODING_UTF_16_LE);

    test1 = "\x00\x00\xFE\xFFThe quick brown fox jumps over the lazy dog";

    encoding = mb_string_encoding(test1, 43 + 4);
    mb_assert("UTF-32-BE BOM", encoding == MB_ENCODING_UTF_32_BE);

    test1 = "\xFF\xFE\x00\x00The quick brown fox jumps over the lazy dog";
    encoding = mb_string_encoding(test1, 43 + 4);
    mb_assert("UTF-32-LE BOM", encoding == (MB_ENCODING_UTF_32_LE |
        MB_ENCODING_UTF_16_LE));
}

MB_EXPORT void mb_string_is_ascii_test() {
    bool is_ascii = mb_string_is_ascii("", 0);
    mb_assert("Void string", !is_ascii);

    is_ascii = mb_string_is_ascii("", 0);
    mb_assert("Void length", !is_ascii);

    is_ascii = mb_string_is_ascii(0, 0);
    mb_assert("Void string and length", !is_ascii);

    const char *test = "The quick brown fox jumps over the lazy dog";
    is_ascii = mb_string_is_ascii(test, 43);
    mb_assert("Valid string and length", is_ascii);

    /* \xF0\x9F\x99\x82 = 🙂 */
    test = "\xF0\x9F\x99\x82";
    is_ascii = mb_string_is_ascii(test, 5);
    mb_assert("String with emoji", !is_ascii);

    test = "\x80";
    is_ascii = mb_string_is_ascii(test, 2);
    mb_assert("Lone continuation byte", !is_ascii);

    test = "\xC0";
    is_ascii = mb_string_is_ascii(test, 2);
    mb_assert("Lone first 2-bytes sequence", !is_ascii);

    test = "\xE0";
    is_ascii = mb_string_is_ascii(test, 2);
    mb_assert("Lone first 3-bytes sequence", !is_ascii);

    test = "\xF0";
    is_ascii = mb_string_is_ascii(test, 2);
    mb_assert("Lone first 4-bytes sequence", !is_ascii);
}

MB_EXPORT void mb_string_is_utf8_test() {
    bool is_utf8 = mb_string_is_utf8("", 0);
    mb_assert("Void string", !is_utf8);

    is_utf8 = mb_string_is_utf8("", 0);
    mb_assert("Void length", !is_utf8);

    is_utf8 = mb_string_is_utf8(0, 0);
    mb_assert("Void string and length", !is_utf8);

    const char *test = "The quick brown fox jumps over the lazy dog";
    is_utf8 = mb_string_is_utf8(test, 43);
    mb_assert("Valid string and length", is_utf8);

    /* \xF0\x9F\x99\x82 = 🙂 */
    test = "The quick brown fox jumps over the lazy dog \xF0\x9F\x99\x82";
    is_utf8 = mb_string_is_utf8(test, 48);
    mb_assert("String with emoji", is_utf8);

    test = "The quick brown fox jumps over the lazy dog \xF0\x9F\x99\x82";
    is_utf8 = mb_string_is_utf8(test, 48);
    mb_assert("Not valid continuation byte", is_utf8);
}