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
static const char* db_name = "../src/mojibake.db";

static void mb_assert(char *message, bool test) {
    ++tests_run;

    if(test) {
        printf("Test: %s \x1B[32mOK\x1B[0m\n", message);
        ++tests_valid;
    } else {
        printf("\x1B[31mTest: %s FAIL\x1B[0m\n", message);
    }
}

static void print_character(mb_character* character, mb_codepoint codepoint) {
    if(!character) {
        return;
    }

    const char* format = "Character %u\n"
        "codepoint: %u\n"
        "name: '%s'\n"
        "block: %u\n"
        "category: %u\n"
        "combining: %u\n"
        "bidirectional: %u\n"
        "decomposition: %u\n"
        "decimal: '%s'\n"
        "digit: '%s'\n"
        "numeric: '%s'\n"
        "mirrored: %s\n"
        "uppercase: %u\n"
        "lowercase: %u\n"
        "titlecase: %u\n";

    printf(format,
        codepoint,
        character->codepoint,
        character->name,
        character->block,
        character->category,
        character->combining,
        character->bidirectional,
        character->decomposition,
        character->decimal,
        character->digit,
        character->numeric,
        character->mirrored ? "true" : "false",
        character->uppercase,
        character->lowercase,
        character->titlecase);
}

static void mb_run_test(char *name, mb_test test) {
    printf("\x1b[36m%s\x1B[0m\n", name);
    test();
    printf("\n");
}

static void mb_version_test() {
    char *version = mb_version();
    size_t size = sizeof(MB_VERSION);
    int result = strncmp(version, MB_VERSION, size);

    mb_assert("Valid version", result == 0);
}

static void mb_version_number_test() {
    unsigned int version_number = mb_version_number();

    mb_assert("Valid version number", version_number == MB_VERSION_NUMBER);
}

static void mb_unicode_version_test() {
    char *version = mb_unicode_version();
    size_t size = sizeof(MB_UNICODE_VERSION);
    int result = strncmp(version, MB_UNICODE_VERSION, size);

    mb_assert("Valid unicode version", result == 0);
}

static void mb_codepoint_is_valid_test() {
    bool validity = mb_codepoint_is_valid(MB_CODEPOINT_MIN + 1);
    mb_assert("Valid codepoint", validity);

    validity = mb_codepoint_is_valid(MB_CODEPOINT_MIN - 1);
    mb_assert("Not valid negative codepoint", !validity);

    validity = mb_codepoint_is_valid(MB_CODEPOINT_MAX + 1);
    mb_assert("Not valid exceed codepoint", !validity);

    validity = mb_codepoint_is_valid(0x1FFFE);
    mb_assert("Not valid codepoint ending in 0xFFFE", !validity);

    validity = mb_codepoint_is_valid(0x1FFFF);
    mb_assert("Not valid codepoint ending in 0xFFFF", !validity);

    validity = mb_codepoint_is_valid(0xFFFE);
    mb_assert("Not valid codepoint 0xFFFE", !validity);

    validity = mb_codepoint_is_valid(0xFFFF);
    mb_assert("Not valid codepoint 0xFFFF", !validity);

    char buffer[32];

    /* 32 noncharacters: U+FDD0–U+FDEF */
    for(mb_codepoint i = 0xFDD0; i <= 0xFDEF; ++i) {
        validity = mb_codepoint_is_valid(i);

        snprintf(buffer, 32, "Not valid codepoint %#X", i);
        mb_assert(buffer, !validity);
    }
}

static void mb_plane_is_valid_test() {
    bool validity = mb_plane_is_valid(1);
    mb_assert("Valid codespace plane", validity);

    validity = mb_plane_is_valid(-1);
    mb_assert("Not valid negative codespace plane", !validity);

    validity = mb_plane_is_valid(MB_PLANE_NUM);
    mb_assert("Not valid exceed codespace plane", !validity);
}

static void mb_plane_name_test() {
    bool validity = strcmp(mb_plane_name(0, true), "BMP") == 0;
    mb_assert("Valid codespace plane name abbreviation", validity);

    validity = strcmp(mb_plane_name(0, false), "Basic Multilingual Plane") == 0;
    mb_assert("Valid codespace plane name full", validity);

    validity = mb_plane_name(-1, false) == NULL;
    mb_assert("Not valid codespace plane low", validity);

    validity = mb_plane_name(MB_PLANE_NUM, false) == NULL;
    mb_assert("Not valid codespace plane high", validity);

    validity = strcmp(mb_plane_name(4, false), "Unassigned") == 0;
    mb_assert("Unassigned codespace plane abbreviation", validity);

    validity = strcmp(mb_plane_name(4, true), "Unassigned") == 0;
    mb_assert("Unassigned codespace plane full", validity);
}

static void mb_string_encoding_test() {
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

static void mb_string_is_ascii_test() {
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

static void mb_string_is_utf8_test() {
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

static void mb_ready_test() {
    mb_assert("Not ready", !mb_ready());

    bool result = mb_initialize("null.db");
    mb_assert("Not valid DB call", !result);
    mb_assert("Not valid DB", !mb_ready());

    result = mb_close();
    mb_assert("Not valid DB close call", !result);
    mb_assert("DB closed", !mb_ready());

    result = mb_initialize(db_name);
    mb_assert("Valid DB call", result);
    mb_assert("Valid DB", mb_ready());

    result = mb_close();
    mb_assert("Valid DB close call", result);
    mb_assert("DB closed", !mb_ready());
}

static void mb_codepoint_character_test() {
    mb_initialize(db_name);
    mb_character character;

    bool ret = mb_codepoint_character(NULL, MB_CODEPOINT_MAX);
    mb_assert("Void character", !ret);

    ret = mb_codepoint_character(&character, MB_CODEPOINT_MAX);
    mb_assert("Not valid codepoint", !ret);

    ret = mb_codepoint_character(&character, 0);
    mb_assert("Codepoint: 0", strcmp((char*)character.name, "NULL") == 0);

    ret = mb_codepoint_character(&character, '$');
    mb_assert("Codepoint: $", strcmp((char*)character.name, "DOLLAR SIGN") == 0);

    /* 0xE0 = à */
    ret = mb_codepoint_character(&character, 0xE0);
    mb_assert("Codepoint: à", strcmp((char*)character.name, "LATIN SMALL LETTER A WITH GRAVE") == 0);

    /* 0x1F642 = 🙂 */
    ret = mb_codepoint_character(&character, 0x1F642);
    mb_assert("Codepoint: 🙂", strcmp((char*)character.name, "SLIGHTLY SMILING FACE") == 0);

    /* 0x0377 = ͷ, 0x0377 + 1 is not mapped */
    ret = mb_codepoint_character(&character, 0x0377 + 1);
    mb_assert("Codepoint not mapped: ͷ + 1", !ret);

    mb_close();
}

static void mb_codepoint_block_test() {
    mb_initialize(db_name);
    mb_character character;

    bool ret = mb_codepoint_character(&character, 0);
    mb_assert("Basic Latin block", character.block == MB_BLOCK_BASIC_LATIN);

    ret = mb_codepoint_character(&character, 0x80 - 1);
    mb_assert("Basic Latin end block", character.block == MB_BLOCK_BASIC_LATIN);

    ret = mb_codepoint_character(&character, 0x80 + 1);
    mb_assert("Latin-1 Supplement block", character.block == MB_BLOCK_LATIN_1_SUPPLEMENT);

    /* 0x0377 = ͷ, 0x0377 + 1 is not mapped */
    ret = mb_codepoint_character(&character, 0x0377 + 1);
    mb_assert("Greek and Coptic not mapped: ͷ + 1", !ret);

    ret = mb_codepoint_character(&character, 0xE0000 + 1);
    mb_assert("Tags block", character.block == MB_BLOCK_TAGS);

    ret = mb_codepoint_character(&character, 0xF0000 + 3);
    mb_assert("Supplementary Private Use Area-A block", !ret);

    ret = mb_codepoint_character(&character, MB_CODEPOINT_MAX - 1);
    mb_assert("Supplementary Private Use Area-B block", !ret);

    mb_close();
}

static void mb_codepoint_is_test() {
    mb_initialize(db_name);

    bool ret = mb_codepoint_is(0, MB_CATEGORY_CC);
    mb_assert("NULL: category other, control", ret);

    /* 0x1F642 = 🙂 */
    ret = mb_codepoint_is(0x1F642, MB_CATEGORY_SO);
    mb_assert("🙂: category Symbol, Other", ret);

    ret = mb_codepoint_is(0x1FFFE, MB_CATEGORY_LU);
    mb_assert("Not valid codepoint: category invalid", !ret);

    /* 0x0377 = ͷ, 0x0377 + 1 is not mapped */
    ret = mb_codepoint_is(0x0377 + 1, MB_CATEGORY_LL);
    mb_assert("Not mapped codepoint: category invalid", !ret);

    mb_close();
}

static void mb_codepoint_is_graphic_test() {
    mb_initialize(db_name);

    bool ret = mb_codepoint_is_graphic(0);
    mb_assert("NULL: not graphic", !ret);

    ret = mb_codepoint_is_graphic('#');
    mb_assert("#: graphic", ret);

    /* 0x1F642 = 🙂 */
    ret = mb_codepoint_is_graphic(0x1F642);
    mb_assert("🙂: graphic", ret);

    ret = mb_codepoint_is_graphic(0x1FFFE);
    mb_assert("Not valid codepoint: not graphic", !ret);

    /* 0x0377 = ͷ, 0x0377 + 1 is not mapped */
    ret = mb_codepoint_is_graphic(0x0377 + 1);
    mb_assert("Not mapped codepoint: not graphic", !ret);

    mb_close();
}

static void mb_codepoint_lc_uc_tc_test() {
    mb_initialize(db_name);

    mb_codepoint ret = mb_codepoint_to_lowercase('#');
    mb_assert("Lowercase #: #", ret == '#');

    ret = mb_codepoint_to_uppercase('#');
    mb_assert("Uppercase #: #", ret == '#');

    ret = mb_codepoint_to_titlecase('#');
    mb_assert("Titlecase #: #", ret == '#');

    ret = mb_codepoint_to_lowercase('A');
    mb_assert("Lowercase: A > a", ret == 'a');

    ret = mb_codepoint_to_lowercase('a');
    mb_assert("Lowercase: a > a", ret == 'a');

    ret = mb_codepoint_to_uppercase('b');
    mb_assert("Uppercase: b > B", ret == 'B');

    ret = mb_codepoint_to_uppercase('B');
    mb_assert("Uppercase: B > B", ret == 'B');

    ret = mb_codepoint_to_titlecase('c');
    mb_assert("Titlecase: c > C", ret == 'C');

    ret = mb_codepoint_to_titlecase('C');
    mb_assert("Titlecase: C > C", ret == 'C');

    mb_close();
}

int main(int argc, const char * argv[]) {
    printf("\x1b[36mMojibake %s test\x1B[0m\n\n", mb_version());

    mb_run_test("Get version", mb_version_test);
    mb_run_test("Get version number", mb_version_number_test);
    mb_run_test("Get unicode version", mb_unicode_version_test);
    mb_run_test("Codepoint is valid", mb_codepoint_is_valid_test);
    mb_run_test("Codespace plane is valid", mb_plane_is_valid_test);
    mb_run_test("Codespace plane name", mb_plane_name_test);
    mb_run_test("String encoding", mb_string_encoding_test);
    mb_run_test("String is ASCII", mb_string_is_ascii_test);
    mb_run_test("String is UTF-8", mb_string_is_utf8_test);

    /* Init tests */
    mb_run_test("Ready", mb_ready_test);
    mb_run_test("Codepoint character", mb_codepoint_character_test);
    mb_run_test("Codepoint block", mb_codepoint_block_test);
    mb_run_test("Codepoint is", mb_codepoint_is_test);
    mb_run_test("Codepoint is graphic", mb_codepoint_is_graphic_test);
    mb_run_test("Codepoint is LC/UC/TC", mb_codepoint_lc_uc_tc_test);

    /* Green if valid and red if not */
    const char* colorCode = tests_valid == tests_run ? "\x1B[32m" : "\x1B[31m";

    printf("%sTests valid/run: %d/%d\n\x1B[0m", colorCode, tests_valid, tests_run);

    return tests_run == tests_valid ? 0 : -1;
}
