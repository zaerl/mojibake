/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "../src/mojibake.h"
#include "test.h"

void *test_utf8(void *arg) {
    /*#define TEST_UTF8(CHAR, STR, RES, COMMENT) \
        ATT_ASSERT(mjb_codepoint_encode(CHAR, (char*)buffer_utf8, 5, MJB_ENCODING_UTF_8), RES, COMMENT) \
        ATT_ASSERT(strcmp(buffer_utf8, STR), 0, COMMENT)

    // UTF-8 tests
    TEST_UTF8(0x007F, "\x7F", 1, "ASCII limit");
    TEST_UTF8(0x07FF, "\xDF\xBF", 2, "2-bytes limit");
    TEST_UTF8(0x1E0A, "\xE1\xB8\x8A", 3, "LATIN CAPITAL LETTER D WITH DOT ABOVE");
    TEST_UTF8(0xFFFD, "\xEF\xBF\xBD", 3, "3-bytes limit");
    TEST_UTF8(0x10FFFE, "\xF4\x8F\xBF\xBE", 4, "4-bytes limit");
    TEST_UTF8(0x1F642, "\xF0\x9F\x99\x82", 4, "SLIGHTLY SMILING FACE");*/
    uint8_t state = MJB_UTF8_ACCEPT;
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
    }

    return NULL;
}
