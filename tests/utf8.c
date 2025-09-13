/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "test.h"
#include "../src/utf8.h"
#include <stdio.h>

void *test_utf8(void *arg) {
    char buffer_utf8[5];

    #define TEST_ENC(CHAR, STR, RES, COMMENT, ENCODING, BUFFER, BUFFER_SIZE) \
        ATT_ASSERT(mjb_codepoint_encode(CHAR, (char*)BUFFER, BUFFER_SIZE, ENCODING), RES, COMMENT) \
        ATT_ASSERT((const char*)BUFFER, STR, COMMENT)

    #define TEST_UTF8(CHAR, STR, RES, COMMENT) \
        TEST_ENC(CHAR, STR, RES, COMMENT, MJB_ENCODING_UTF_8, buffer_utf8, 5)

    // UTF-8 tests
    TEST_UTF8(0x007F, "\x7F", 1, "ASCII limit");
    TEST_UTF8(0x07FF, "\xDF\xBF", 2, "2-bytes limit");
    TEST_UTF8(0x1E0A, "\xE1\xB8\x8A", 3, "LATIN CAPITAL LETTER D WITH DOT ABOVE");
    TEST_UTF8(0xFFFD, "\xEF\xBF\xBD", 3, "3-bytes limit");
    TEST_UTF8(0x10FFFE, "\xF4\x8F\xBF\xBE", 4, "4-bytes limit");
    TEST_UTF8(0x1F642, "\xF0\x9F\x99\x82", 4, "SLIGHTLY SMILING FACE");

    char buffer_utf16[5];

    #define TEST_UTF16_LE(CHAR, STR, RES, COMMENT) \
        TEST_ENC(CHAR, STR, RES, COMMENT, MJB_ENCODING_UTF_16_LE, buffer_utf16, 5)

    TEST_UTF16_LE(0x007F, "\x7F\x00", 2, "ASCII limit UTF-16LE");
    TEST_UTF16_LE(0x07FF, "\xFF\x07", 2, "2-bytes limit UTF-16LE");
    TEST_UTF16_LE(0x1E0A, "\x0A\x1E", 2, "LATIN CAPITAL LETTER D WITH DOT ABOVE UTF-16LE");
    TEST_UTF16_LE(0xFFFD, "\xFD\xFF", 2, "3-bytes limit UTF-16LE");
    TEST_UTF16_LE(0x10FFFE, "\xFF\xDB\xFE\xDF", 4, "4-bytes limit UTF-16LE");
    TEST_UTF16_LE(0x1F642, "\x3D\xD8\x42\xDE", 4, "SLIGHTLY SMILING FACE UTF-16LE");

    #define TEST_UTF16_BE(CHAR, STR, RES, COMMENT) \
        TEST_ENC(CHAR, STR, RES, COMMENT, MJB_ENCODING_UTF_16_BE, buffer_utf16, 5)

    TEST_UTF16_BE(0x007F, "\x00\x7F", 2, "ASCII limit UTF-16BE");
    TEST_UTF16_BE(0x07FF, "\x07\xFF", 2, "2-bytes limit UTF-16BE");
    TEST_UTF16_BE(0x1E0A, "\x1E\x0A", 2, "LATIN CAPITAL LETTER D WITH DOT ABOVE UTF-16BE");
    TEST_UTF16_BE(0xFFFD, "\xFF\xFD", 2, "3-bytes limit UTF-16BE");
    TEST_UTF16_BE(0x10FFFE, "\xDB\xFF\xDF\xFE", 4, "4-bytes limit UTF-16BE");
    TEST_UTF16_BE(0x1F642, "\xD8\x3D\xDE\x42", 4, "SLIGHTLY SMILING FACE UTF-16BE");

    /*uint8_t state = MJB_UTF8_ACCEPT;
    mjb_codepoint codepoint;
    const char *hello_world = "Hello, World \xF0\x9F\x99\x82!";
    const char *index = hello_world;

    for(; *index; ++index) {
        state = mjb_utf8_decode_step(state, *index, &codepoint);
        // printf("State %d\n", state);

        if(state == MJB_UTF8_ACCEPT) {
            mjb_character character;
            bool valid = mjb_codepoint_character(codepoint, &character);
            printf("U+%04X %s [%lu]\n", codepoint, valid ? character.name : "Unknown", index - hello_world + 1);
        } else if(state == MJB_UTF8_REJECT) {
            // printf("The string is not well-formed\n");
            printf("Bad UTF-8 sequence\n");
            state = MJB_UTF8_ACCEPT;
        }
    }

    if(state != MJB_UTF8_ACCEPT) {
        printf("The string is not well-formed\n");
    }*/

    #undef TEST_ENC
    #undef TEST_UTF8
    #undef TEST_UTF16_LE
    #undef TEST_UTF16_BE

    return NULL;
}
