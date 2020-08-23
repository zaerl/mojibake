#include <stdio.h>
#include <string.h>

#include "test.h"

MB_EXPORT void mb_codepoint_character_test() {
    mb_initialize(MB_DB_PATH);
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

MB_EXPORT void mb_codepoint_block_test() {
    mb_initialize(MB_DB_PATH);
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

MB_EXPORT void mb_codepoint_is_test() {
    mb_initialize(MB_DB_PATH);

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

MB_EXPORT void mb_codepoint_is_graphic_test() {
    mb_initialize(MB_DB_PATH);

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

MB_EXPORT void mb_codepoint_is_valid_test() {
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

MB_EXPORT void mb_codepoint_lc_uc_tc_test() {
    mb_initialize(MB_DB_PATH);

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

MB_EXPORT void mb_codepoint_normalize_test() {
    mb_initialize(MB_DB_PATH);

    mb_normalize("\xE2\x84\xAB", 1, MB_NORMALIZATION_NFD);
    mb_assert("ANGSTROM SIGN (U+212B) NFD", true);

    mb_close();
}
