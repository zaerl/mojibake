#include <stdio.h>
#include <string.h>

#include "test.h"

MJB_EXPORT void mjb_codepoint_character_test() {
    mjb_initialize(MJB_DB_PATH);
    mjb_character character;

    bool ret = mjb_codepoint_character(NULL, MJB_CODEPOINT_MAX);
    mjb_assert("Void character", !ret);

    ret = mjb_codepoint_character(&character, MJB_CODEPOINT_MAX);
    mjb_assert("Not valid codepoint", !ret);

    ret = mjb_codepoint_character(&character, 0);
    mjb_assert("Codepoint: 0", strcmp((char*)character.name, "NULL") == 0);

    ret = mjb_codepoint_character(&character, '$');
    mjb_assert("Codepoint: $", strcmp((char*)character.name, "DOLLAR SIGN") == 0);

    /* 0xE0 = à */
    ret = mjb_codepoint_character(&character, 0xE0);
    mjb_assert("Codepoint: à", strcmp((char*)character.name, "LATIN SMALL LETTER A WITH GRAVE") == 0);

    /* 0x1F642 = 🙂 */
    ret = mjb_codepoint_character(&character, 0x1F642);
    mjb_assert("Codepoint: 🙂", strcmp((char*)character.name, "SLIGHTLY SMILING FACE") == 0);

    /* 0x0377 = ͷ, 0x0377 + 1 is not mapped */
    ret = mjb_codepoint_character(&character, 0x0377 + 1);
    mjb_assert("Codepoint not mapped: ͷ + 1", !ret);

    mjb_close();
}

MJB_EXPORT void mjb_codepoint_block_test() {
    mjb_initialize(MJB_DB_PATH);
    mjb_character character;

    bool ret = mjb_codepoint_character(&character, 0);
    mjb_assert("Basic Latin block", character.block == MJB_BLOCK_BASIC_LATIN);

    ret = mjb_codepoint_character(&character, 0x80 - 1);
    mjb_assert("Basic Latin end block", character.block == MJB_BLOCK_BASIC_LATIN);

    ret = mjb_codepoint_character(&character, 0x80 + 1);
    mjb_assert("Latin-1 Supplement block", character.block == MJB_BLOCK_LATIN_1_SUPPLEMENT);

    /* 0x0377 = ͷ, 0x0377 + 1 is not mapped */
    ret = mjb_codepoint_character(&character, 0x0377 + 1);
    mjb_assert("Greek and Coptic not mapped: ͷ + 1", !ret);

    ret = mjb_codepoint_character(&character, 0xE0000 + 1);
    mjb_assert("Tags block", character.block == MJB_BLOCK_TAGS);

    ret = mjb_codepoint_character(&character, 0xF0000 + 3);
    mjb_assert("Supplementary Private Use Area-A block", !ret);

    ret = mjb_codepoint_character(&character, MJB_CODEPOINT_MAX - 1);
    mjb_assert("Supplementary Private Use Area-B block", !ret);

    mjb_close();
}

MJB_EXPORT void mjb_codepoint_is_test() {
    mjb_initialize(MJB_DB_PATH);

    bool ret = mjb_codepoint_is(0, MJB_CATEGORY_CC);
    mjb_assert("NULL: category other, control", ret);

    /* 0x1F642 = 🙂 */
    ret = mjb_codepoint_is(0x1F642, MJB_CATEGORY_SO);
    mjb_assert("🙂: category Symbol, Other", ret);

    ret = mjb_codepoint_is(0x1FFFE, MJB_CATEGORY_LU);
    mjb_assert("Not valid codepoint: category invalid", !ret);

    /* 0x0377 = ͷ, 0x0377 + 1 is not mapped */
    ret = mjb_codepoint_is(0x0377 + 1, MJB_CATEGORY_LL);
    mjb_assert("Not mapped codepoint: category invalid", !ret);

    mjb_close();
}

MJB_EXPORT void mjb_codepoint_is_graphic_test() {
    mjb_initialize(MJB_DB_PATH);

    bool ret = mjb_codepoint_is_graphic(0);
    mjb_assert("NULL: not graphic", !ret);

    ret = mjb_codepoint_is_graphic('#');
    mjb_assert("#: graphic", ret);

    /* 0x1F642 = 🙂 */
    ret = mjb_codepoint_is_graphic(0x1F642);
    mjb_assert("🙂: graphic", ret);

    ret = mjb_codepoint_is_graphic(0x1FFFE);
    mjb_assert("Not valid codepoint: not graphic", !ret);

    /* 0x0377 = ͷ, 0x0377 + 1 is not mapped */
    ret = mjb_codepoint_is_graphic(0x0377 + 1);
    mjb_assert("Not mapped codepoint: not graphic", !ret);

    mjb_close();
}

MJB_EXPORT void mjb_codepoint_is_valid_test() {
    bool validity = mjb_codepoint_is_valid(MJB_CODEPOINT_MIN + 1);
    mjb_assert("Valid codepoint", validity);

    validity = mjb_codepoint_is_valid(MJB_CODEPOINT_MIN - 1);
    mjb_assert("Not valid negative codepoint", !validity);

    validity = mjb_codepoint_is_valid(MJB_CODEPOINT_MAX + 1);
    mjb_assert("Not valid exceed codepoint", !validity);

    validity = mjb_codepoint_is_valid(0x1FFFE);
    mjb_assert("Not valid codepoint ending in 0xFFFE", !validity);

    validity = mjb_codepoint_is_valid(0x1FFFF);
    mjb_assert("Not valid codepoint ending in 0xFFFF", !validity);

    validity = mjb_codepoint_is_valid(0xFFFE);
    mjb_assert("Not valid codepoint 0xFFFE", !validity);

    validity = mjb_codepoint_is_valid(0xFFFF);
    mjb_assert("Not valid codepoint 0xFFFF", !validity);

    char buffer[32];

    /* 32 noncharacters: U+FDD0–U+FDEF */
    for(mjb_codepoint i = 0xFDD0; i <= 0xFDEF; ++i) {
        validity = mjb_codepoint_is_valid(i);

        snprintf(buffer, 32, "Not valid codepoint %#X", i);
        mjb_assert(buffer, !validity);
    }
}

MJB_EXPORT void mjb_codepoint_lc_uc_tc_test() {
    mjb_initialize(MJB_DB_PATH);

    mjb_codepoint ret = mjb_codepoint_to_lowercase('#');
    mjb_assert("Lowercase #: #", ret == '#');

    ret = mjb_codepoint_to_uppercase('#');
    mjb_assert("Uppercase #: #", ret == '#');

    ret = mjb_codepoint_to_titlecase('#');
    mjb_assert("Titlecase #: #", ret == '#');

    ret = mjb_codepoint_to_lowercase('A');
    mjb_assert("Lowercase: A > a", ret == 'a');

    ret = mjb_codepoint_to_lowercase('a');
    mjb_assert("Lowercase: a > a", ret == 'a');

    ret = mjb_codepoint_to_uppercase('b');
    mjb_assert("Uppercase: b > B", ret == 'B');

    ret = mjb_codepoint_to_uppercase('B');
    mjb_assert("Uppercase: B > B", ret == 'B');

    ret = mjb_codepoint_to_titlecase('c');
    mjb_assert("Titlecase: c > C", ret == 'C');

    ret = mjb_codepoint_to_titlecase('C');
    mjb_assert("Titlecase: C > C", ret == 'C');

    mjb_close();
}
