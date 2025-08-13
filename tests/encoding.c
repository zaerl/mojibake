/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "test.h"

void *test_encoding(void *arg) {
    ATT_ASSERT(mjb_string_encoding(0, 10), MJB_ENCODING_UNKNOWN, "Void unknown string")
    ATT_ASSERT(mjb_string_encoding("", 0), MJB_ENCODING_UNKNOWN, "Void unknown length");

    ATT_ASSERT(mjb_string_encoding(0, 0), MJB_ENCODING_UNKNOWN, "Void unknown string and length");
    const char *test1 = "The quick brown fox jumps over the lazy dog";

    ATT_ASSERT(mjb_string_encoding(test1, 43), (MJB_ENCODING_ASCII | MJB_ENCODING_UTF_8), "Plain ASCII (and UTF-8)");
    const char *test2 = "\xEF\xBB\xBFThe quick brown fox jumps over the lazy dog";

    ATT_ASSERT(mjb_string_encoding(test2, 43 + 3), MJB_ENCODING_UTF_8, "UTF-8 BOM");
    const char *test3 = "\xFE\xFFThe quick brown fox jumps over the lazy dog";

    ATT_ASSERT(mjb_string_encoding(test3, 43 + 2), MJB_ENCODING_UTF_16 | MJB_ENCODING_UTF_16_BE, "UTF-16-BE BOM");

    const char *test4 = "\xFF\xFEThe quick brown fox jumps over the lazy dog";
    ATT_ASSERT(mjb_string_encoding(test4, 43 + 2), MJB_ENCODING_UTF_16 | MJB_ENCODING_UTF_16_LE, "UTF-16-LE BOM");

    const char *test5 = "\x00\x00\xFE\xFFThe quick brown fox jumps over the lazy dog";
    ATT_ASSERT(mjb_string_encoding(test5, 43 + 4), MJB_ENCODING_UTF_32 | MJB_ENCODING_UTF_32_BE, "UTF-32-BE BOM");

    const char *test6 = "\xFF\xFE\x00\x00The quick brown fox jumps over the lazy dog";
    ATT_ASSERT(mjb_string_encoding(test6, 43 + 4), MJB_ENCODING_UTF_32 | MJB_ENCODING_UTF_32_LE | MJB_ENCODING_UTF_16 | MJB_ENCODING_UTF_16_LE, "UTF-32-LE BOM");

    ATT_ASSERT(mjb_string_is_ascii("", 0), false, "Void ASCII string");
    ATT_ASSERT(mjb_string_is_ascii("", 0), false, "Void ASCII length");
    ATT_ASSERT(mjb_string_is_ascii(0, 0), false, "Void ASCII string and length");

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

    const char *utf8_test = "";

    ATT_ASSERT(mjb_string_is_utf8(NULL, 0), false, "Void UTF-8 string");
    ATT_ASSERT(mjb_string_is_utf8("", 0), false, "Empty UTF-8 \"\" string");
    ATT_ASSERT(mjb_string_is_utf8(utf8_test, strlen(utf8_test)), false, "Empty UTF-8 string");

    utf8_test = "Hello, world!";
    ATT_ASSERT(mjb_string_is_utf8(utf8_test, strlen(utf8_test)), true, "Simple ASCII");

    utf8_test = "Hell\xC3\xB6 w\xC3\xB6rld";
    ATT_ASSERT(mjb_string_is_utf8(utf8_test, strlen(utf8_test)), true, "Hell[o] wörld");

    utf8_test = "\xE3\x81\x93\xE3\x82\x93\xE3\x81\xAB\xE3\x81\xA1\xE3\x81\xAF\xE4\xB8\x96\xE7\x95\x8C"; //こんにちは世界
    ATT_ASSERT(mjb_string_is_utf8(utf8_test, strlen(utf8_test)), true, "Japanese");

    utf8_test = "Hello \xF0\x9F\x8C\x8D";
    ATT_ASSERT(mjb_string_is_utf8(utf8_test, strlen(utf8_test)), true, "Hello (world)");

    utf8_test = "a\xC2\xA2\xE2\x82\xAC\xF0\x90\x8D\x88";
    ATT_ASSERT(mjb_string_is_utf8(utf8_test, strlen(utf8_test)), true, "1-byte, 2-byte, 3-byte, and 4-byte characters");

    utf8_test = "\xF4\x8F\xBF\xBF";
    ATT_ASSERT(mjb_string_is_utf8(utf8_test, strlen(utf8_test)), true, "U+10FFFF, maximum code point");

    utf8_test = "\xEF\xBB\xBF""Hello";
    ATT_ASSERT(mjb_string_is_utf8(utf8_test, strlen(utf8_test)), true, "BOM (Byte Order Mark) followed by \"Hello\"");

    utf8_test = "Hello\xC2\xA0World\xE2\x80\x83Test\xE2\x80\x8B";
    ATT_ASSERT(mjb_string_is_utf8(utf8_test, strlen(utf8_test)), true, "Various Unicode spaces and invisible characters");

    utf8_test = "n\xCC\x83";
    ATT_ASSERT(mjb_string_is_utf8(utf8_test, strlen(utf8_test)), true, "N combined with tilde");

    utf8_test = "A\xCE\x91\xE2\x98\x83\xF0\x9D\x84\x9E";
    ATT_ASSERT(mjb_string_is_utf8(utf8_test, strlen(utf8_test)), true, "Characters from various Unicode planes");

    utf8_test = "Hello\0World";
    ATT_ASSERT(mjb_string_is_utf8(utf8_test, 11), true, "String with NULL character");

    utf8_test = "\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F\x7F"; //
    ATT_ASSERT(mjb_string_is_utf8(utf8_test, strlen(utf8_test)), true, "All ASCII control characters");

    utf8_test = "Hello\xE2\x80\x94World\xE2\x80\xA2Test\xE2\x99\xA5Unicode\xE2\x98\xAE";
    ATT_ASSERT(mjb_string_is_utf8(utf8_test, strlen(utf8_test)), true, "Various Unicode punctuation and symbols");

    // UTF-16 tests
    ATT_ASSERT(mjb_string_is_utf16(NULL, 0), false, "Void UTF-16 string");
    ATT_ASSERT(mjb_string_is_utf16("", 0), false, "Empty UTF-16 string");
    ATT_ASSERT(mjb_string_is_utf16("A", 1), false, "Odd-length UTF-16 string");
    ATT_ASSERT(mjb_string_is_utf16("", 1), false, "Odd-length empty UTF-16 string");

    // UTF-16BE tests (Big Endian)
    const char *utf16be_hello = "\x00H\x00e\x00l\x00l\x00o";  // "Hello" in UTF-16BE
    ATT_ASSERT(mjb_string_is_utf16(utf16be_hello, 10), true, "UTF-16BE Hello");

    const char *utf16be_with_bmp = "\x00A\x03\x91\x00Z";  // "AΑZ" (A, Greek Alpha, Z) in UTF-16BE
    ATT_ASSERT(mjb_string_is_utf16(utf16be_with_bmp, 6), true, "UTF-16BE with BMP characters");

    // UTF-16LE tests (Little Endian)
    const char *utf16le_hello = "H\x00e\x00l\x00l\x00o\x00";  // "Hello" in UTF-16LE
    ATT_ASSERT(mjb_string_is_utf16(utf16le_hello, 10), true, "UTF-16LE Hello");

    const char *utf16le_with_bmp = "A\x00\x91\x03Z\x00";  // "AΑZ" (A, Greek Alpha, Z) in UTF-16LE
    ATT_ASSERT(mjb_string_is_utf16(utf16le_with_bmp, 6), true, "UTF-16LE with BMP characters");

    // UTF-16BE with surrogate pairs (emoji: 🙂 U+1F642)
    const char *utf16be_emoji = "\xD8\x3D\xDE\x42";  // 🙂 in UTF-16BE surrogate pair
    ATT_ASSERT(mjb_string_is_utf16(utf16be_emoji, 4), true, "UTF-16BE with surrogate pair");

    // UTF-16LE with surrogate pairs (emoji: 🙂 U+1F642)
    const char *utf16le_emoji = "\x3D\xD8\x42\xDE";  // 🙂 in UTF-16LE surrogate pair
    ATT_ASSERT(mjb_string_is_utf16(utf16le_emoji, 4), true, "UTF-16LE with surrogate pair");

    // UTF-16 with BOM markers
    const char *utf16be_bom = "\xFE\xFF\x00H\x00i";  // BOM + "Hi" in UTF-16BE
    ATT_ASSERT(mjb_string_is_utf16(utf16be_bom, 6), true, "UTF-16BE with BOM");

    const char *utf16le_bom = "\xFF\xFEH\x00i\x00";  // BOM + "Hi" in UTF-16LE
    ATT_ASSERT(mjb_string_is_utf16(utf16le_bom, 6), true, "UTF-16LE with BOM");

    // Edge cases - maximum valid codepoints
    const char *utf16be_max = "\xDB\xFF\xDF\xFF";  // U+10FFFF in UTF-16BE
    ATT_ASSERT(mjb_string_is_utf16(utf16be_max, 4), true, "UTF-16BE maximum codepoint");

    const char *utf16le_max = "\xFF\xDB\xFF\xDF";  // U+10FFFF in UTF-16LE
    ATT_ASSERT(mjb_string_is_utf16(utf16le_max, 4), true, "UTF-16LE maximum codepoint");

    ATT_ASSERT(mjb_codepoint_encode(0, (char*)0, 0, MJB_ENCODING_UTF_8), 0, "Void buffer")
    ATT_ASSERT(mjb_codepoint_encode(0, (char*)1, 1, MJB_ENCODING_UTF_8), 0, "Wrong size")

    ATT_ASSERT(mjb_codepoint_encode(0, (char*)0, 0, MJB_ENCODING_UTF_16_LE), 0, "Void buffer UTF-16LE")
    ATT_ASSERT(mjb_codepoint_encode(0, (char*)1, 1, MJB_ENCODING_UTF_16_LE), 0, "Wrong size UTF-16LE")

    ATT_ASSERT(mjb_codepoint_encode(0, (char*)0, 0, MJB_ENCODING_UTF_16_BE), 0, "Void buffer UTF-16BE")
    ATT_ASSERT(mjb_codepoint_encode(0, (char*)1, 1, MJB_ENCODING_UTF_16_BE), 0, "Wrong size UTF-16BE")

    ATT_ASSERT(mjb_codepoint_encode(0, (char*)1, 4, MJB_ENCODING_UTF_32), 0, "Invalid encoding")

    char buffer_utf8[5];
    ATT_ASSERT(mjb_codepoint_encode(MJB_CODEPOINT_MAX + 1, (char*)buffer_utf8, 5, MJB_ENCODING_UTF_8), 0, "Noncharacter max")
    ATT_ASSERT(mjb_codepoint_encode(MJB_CODEPOINT_MIN - 1, (char*)buffer_utf8, 5, MJB_ENCODING_UTF_8), 0, "Noncharacter min")

    ATT_ASSERT(mjb_codepoint_encode(MJB_CODEPOINT_MAX + 1, (char*)buffer_utf8, 5, MJB_ENCODING_UTF_16_LE), 0, "Noncharacter max UTF-16LE")
    ATT_ASSERT(mjb_codepoint_encode(MJB_CODEPOINT_MIN - 1, (char*)buffer_utf8, 5, MJB_ENCODING_UTF_16_LE), 0, "Noncharacter min UTF-16LE")

    ATT_ASSERT(mjb_codepoint_encode(MJB_CODEPOINT_MAX + 1, (char*)buffer_utf8, 5, MJB_ENCODING_UTF_16_BE), 0, "Noncharacter max UTF-16BE")
    ATT_ASSERT(mjb_codepoint_encode(MJB_CODEPOINT_MIN - 1, (char*)buffer_utf8, 5, MJB_ENCODING_UTF_16_BE), 0, "Noncharacter min UTF-16BE")

    ATT_ASSERT(mjb_codepoint_encode(0x0000, (char*)buffer_utf8, 5, MJB_ENCODING_UTF_8), 1, "0x0000 UTF-8")
    ATT_ASSERT(buffer_utf8[0], 0, "0x0000 UTF-8")

    ATT_ASSERT(mjb_codepoint_encode(0x0000, (char*)buffer_utf8, 5, MJB_ENCODING_UTF_16_LE), 2, "0x0000 UTF-16LE")
    ATT_ASSERT(buffer_utf8[0], 0, "0x0000 UTF-16LE")

    ATT_ASSERT(mjb_codepoint_encode(0x0000, (char*)buffer_utf8, 5, MJB_ENCODING_UTF_16_BE), 2, "0x0000 UTF-16BE")
    ATT_ASSERT(buffer_utf8[0], 0, "0x0000 UTF-16BE")

    // CURRENT_COUNT 12
    #define TEST_UTF8(CHAR, STR, RES, COMMENT) \
        ATT_ASSERT(mjb_codepoint_encode(CHAR, (char*)buffer_utf8, 5, MJB_ENCODING_UTF_8), RES, COMMENT) \
        ATT_ASSERT(strcmp(buffer_utf8, STR), 0, COMMENT)

    // UTF-8 tests
    TEST_UTF8(0x007F, "\x7F", 1, "ASCII limit");
    TEST_UTF8(0x07FF, "\xDF\xBF", 2, "2-bytes limit");
    TEST_UTF8(0x1E0A, "\xE1\xB8\x8A", 3, "LATIN CAPITAL LETTER D WITH DOT ABOVE");
    TEST_UTF8(0xFFFD, "\xEF\xBF\xBD", 3, "3-bytes limit");
    TEST_UTF8(0x10FFFE, "\xF4\x8F\xBF\xBE", 4, "4-bytes limit");
    TEST_UTF8(0x1F642, "\xF0\x9F\x99\x82", 4, "SLIGHTLY SMILING FACE");

    // CURRENT_COUNT 12
    #define TEST_UTF16LE(CHAR, STR, RES, COMMENT) \
        ATT_ASSERT(mjb_codepoint_encode(CHAR, (char*)buffer_utf8, 5, MJB_ENCODING_UTF_16_LE), RES, COMMENT) \
        ATT_ASSERT(strcmp(buffer_utf8, STR), 0, COMMENT)

    TEST_UTF16LE(0x007F, "\x7F\x00", 2, "ASCII limit UTF-16LE");
    TEST_UTF16LE(0x07FF, "\xFF\x07", 2, "2-bytes limit UTF-16LE");
    TEST_UTF16LE(0x1E0A, "\x0A\x1E", 2, "LATIN CAPITAL LETTER D WITH DOT ABOVE UTF-16LE");
    TEST_UTF16LE(0xFFFD, "\xFD\xFF", 2, "3-bytes limit UTF-16LE");
    TEST_UTF16LE(0x10FFFE, "\xFF\xDB\xFE\xDF", 4, "4-bytes limit UTF-16LE");
    TEST_UTF16LE(0x1F642, "\x3D\xD8\x42\xDE", 4, "SLIGHTLY SMILING FACE UTF-16LE");

    // CURRENT_COUNT 12
    #define TEST_UTF16BE(CHAR, STR, RES, COMMENT) \
        ATT_ASSERT(mjb_codepoint_encode(CHAR, (char*)buffer_utf8, 5, MJB_ENCODING_UTF_16_BE), RES, COMMENT) \
        ATT_ASSERT(strcmp(buffer_utf8, STR), 0, COMMENT)

    TEST_UTF16BE(0x007F, "\x00\x7F", 2, "ASCII limit UTF-16BE");
    TEST_UTF16BE(0x07FF, "\x07\xFF", 2, "2-bytes limit UTF-16BE");
    TEST_UTF16BE(0x1E0A, "\x1E\x0A", 2, "LATIN CAPITAL LETTER D WITH DOT ABOVE UTF-16BE");
    TEST_UTF16BE(0xFFFD, "\xFF\xFD", 2, "3-bytes limit UTF-16BE");
    TEST_UTF16BE(0x10FFFE, "\xDB\xFF\xDF\xFE", 4, "4-bytes limit UTF-16BE");
    TEST_UTF16BE(0x1F642, "\xD8\x3D\xDE\x42", 4, "SLIGHTLY SMILING FACE UTF-16BE");

    #undef TEST_UTF8
    #undef TEST_UTF16LE
    #undef TEST_UTF16BE

    return NULL;
}
