/**
 * The mojibake library tests
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <string.h>

#include "../src/mojibake.h"
#include "../src/version.h"

typedef void (*mb_test)(void);

static int tests_run;
static int tests_valid;

void mb_assert(char *message, bool test) {
    ++tests_run;

    if(test) {
        printf("Test: %s \x1B[32mOK\x1B[0m\n", message);
        ++tests_valid;
    } else {
        printf("\x1B[31mTest: %s FAIL\x1B[0m\n", message);
    }
}

static void mb_run_test(char *name, mb_test test) {
    printf("%s\n", name);
    test();
    printf("\n");
}

static void mb_version_test() {
    char *version = mb_version();
    size_t size = sizeof(MB_VERSION);
    int result = strncmp(version, MB_VERSION, size);

    mb_assert("valid version", result == 0);
}

static void mb_version_number_test() {
    unsigned int version_number = mb_version_number();

    mb_assert("valid version number", version_number == MB_VERSION_NUMBER);
}

static void mb_unicode_version_test() {
    char *version = mb_unicode_version();
    size_t size = sizeof(MB_UNICODE_VERSION);
    int result = strncmp(version, MB_UNICODE_VERSION, size);

    mb_assert("valid unicode version", result == 0);
}

static void mb_codepoint_is_valid_test() {
    bool validity = mb_codepoint_is_valid(MB_CODEPOINT_MIN + 1);
    mb_assert("valid codepoint", validity);

    validity = mb_codepoint_is_valid(MB_CODEPOINT_MIN - 1);
    mb_assert("not valid negative codepoint", !validity);

    validity = mb_codepoint_is_valid(MB_CODEPOINT_MAX + 1);
    mb_assert("not valid exceed codepoint", !validity);

    validity = mb_codepoint_is_valid(0x1FFFE);
    mb_assert("not valid codepoint ending in 0xFFFE", !validity);

    validity = mb_codepoint_is_valid(0x1FFFF);
    mb_assert("not valid codepoint ending in 0xFFFF", !validity);

    validity = mb_codepoint_is_valid(0xFFFE);
    mb_assert("not valid codepoint 0xFFFE", !validity);

    validity = mb_codepoint_is_valid(0xFFFF);
    mb_assert("not valid codepoint 0xFFFF", !validity);

    char buffer[32];

    /* 32 noncharacters: U+FDD0–U+FDEF */
    for(mb_codepoint i = 0xFDD0; i <= 0xFDEF; ++i) {
        validity = mb_codepoint_is_valid(i);

        snprintf(buffer, 32, "invalid codepoint %#X", i);
        mb_assert(buffer, !validity);
    }
}

static void mb_plane_is_valid_test() {
    bool validity = mb_plane_is_valid(1);
    mb_assert("valid codespace plane", validity);

    validity = mb_plane_is_valid(-1);
    mb_assert("not valid negative codespace plane", !validity);

    validity = mb_plane_is_valid(MB_PLANE_NUM);
    mb_assert("not valid exceed codespace plane", !validity);
}

static void mb_plane_name_test() {
    bool validity = strcmp(mb_plane_name(0, true), "BMP") == 0;
    mb_assert("valid codespace plane name abbreviation", validity);

    validity = strcmp(mb_plane_name(0, false), "Basic Multilingual Plane") == 0;
    mb_assert("valid codespace plane name full", validity);

    validity = mb_plane_name(-1, false) == NULL;
    mb_assert("invalid codespace plane low", validity);

    validity = mb_plane_name(MB_PLANE_NUM, false) == NULL;
    mb_assert("invalid codespace plane high", validity);

    validity = strcmp(mb_plane_name(4, false), "Unassigned") == 0;
    mb_assert("unassigned codespace plane abbreviation", validity);

    validity = strcmp(mb_plane_name(4, true), "Unassigned") == 0;
    mb_assert("unassigned codespace plane full", validity);
}

static void mb_string_encoding_test() {
    mb_encoding encoding = mb_string_encoding(0, 10);
    mb_assert("void string", encoding == MB_ENCODING_UNKNOWN);

    encoding = mb_string_encoding("", 0);
    mb_assert("void length", encoding == MB_ENCODING_UNKNOWN);

    encoding = mb_string_encoding(0, 0);
    mb_assert("void string and length", encoding == MB_ENCODING_UNKNOWN);

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

static void mb_string_is_ascii_test() {
    bool is_ascii = mb_string_is_ascii("", 0);
    mb_assert("void string", !is_ascii);

    is_ascii = mb_string_is_ascii("", 0);
    mb_assert("void length", !is_ascii);

    is_ascii = mb_string_is_ascii(0, 0);
    mb_assert("void string and length", !is_ascii);

    const char *test = "The quick brown fox jumps over the lazy dog";
    is_ascii = mb_string_is_ascii(test, 43);
    mb_assert("void string and length", is_ascii);

    /* \xF0\x9F\x99\x82 = 🙂 */
    test = "\xF0\x9F\x99\x82";
    is_ascii = mb_string_is_ascii(test, 5);
    mb_assert("string with emoji", !is_ascii);

    test = "\x80";
    is_ascii = mb_string_is_ascii(test, 2);
    mb_assert("lone continuation byte", !is_ascii);

    test = "\xC0";
    is_ascii = mb_string_is_ascii(test, 2);
    mb_assert("lone first 2-bytes sequence", !is_ascii);

    test = "\xE0";
    is_ascii = mb_string_is_ascii(test, 2);
    mb_assert("lone first 3-bytes sequence", !is_ascii);

    test = "\xF0";
    is_ascii = mb_string_is_ascii(test, 2);
    mb_assert("lone first 4-bytes sequence", !is_ascii);
}

static void mb_string_is_utf8_test() {
    bool is_utf8 = mb_string_is_utf8("", 0);
    mb_assert("void string", !is_utf8);

    is_utf8 = mb_string_is_utf8("", 0);
    mb_assert("void length", !is_utf8);

    is_utf8 = mb_string_is_utf8(0, 0);
    mb_assert("void string and length", !is_utf8);

    const char *test = "The quick brown fox jumps over the lazy dog";
    is_utf8 = mb_string_is_utf8(test, 43);
    mb_assert("void string and length", is_utf8);

    /* \xF0\x9F\x99\x82 = 🙂 */
    test = "The quick brown fox jumps over the lazy dog \xF0\x9F\x99\x82";
    is_utf8 = mb_string_is_utf8(test, 48);
    mb_assert("string with emoji", is_utf8);

    test = "The quick brown fox jumps over the lazy dog \xF0\x9F\x99\x82";
    is_utf8 = mb_string_is_utf8(test, 48);
    mb_assert("Invalid continuation byte", is_utf8);
}

static void mb_codepoint_character_test() {
    const mb_character* character = mb_codepoint_character(MB_CODEPOINT_MAX);
    mb_assert("invalid codepoint", character == NULL);

    character = mb_codepoint_character('$');
    mb_assert("Codepoint $", character && strcmp(character->name, "DOLLAR SIGN") == 0);

    /* 0xE0 = à */
    character = mb_codepoint_character(0xE0);
    mb_assert("Codepoint à", character && strcmp(character->name, "LATIN SMALL LETTER A WITH GRAVE") == 0);

    /* 0x1F642 = 🙂 */
    character = mb_codepoint_character(0x1F642);
    mb_assert("Codepoint 🙂", character && strcmp(character->name, "SLIGHTLY SMILING FACE") == 0);
}

int main(int argc, const char * argv[]) {
    printf("Mojibake %s test\n\n", mb_version());

    mb_run_test("Get version", mb_version_test);
    mb_run_test("Get version number", mb_version_number_test);
    mb_run_test("Get unicode version", mb_unicode_version_test);
    mb_run_test("Codepoint is valid", mb_codepoint_is_valid_test);
    mb_run_test("Codespace plane is valid", mb_plane_is_valid_test);
    mb_run_test("Codespace plane name", mb_plane_name_test);
    mb_run_test("String get encoding", mb_string_encoding_test);
    mb_run_test("String is ASCII", mb_string_is_ascii_test);
    mb_run_test("String is UTF-8", mb_string_is_utf8_test);
    mb_run_test("Codepoint get character", mb_codepoint_character_test);

    /* Green if valid and red if not */
    const char* colorCode = tests_valid == tests_run ? "\x1B[32m" : "\x1B[31m";

    printf("%sTests valid/run: %d/%d\n\x1B[0m", colorCode, tests_valid, tests_run);

    return tests_run == tests_valid ? 0 : -1;
}
