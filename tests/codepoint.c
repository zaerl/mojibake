/**
 * The mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <string.h>

#include "test.h"

MJB_EXPORT void mjb_codepoint_character_test() {
    mojibake *mjb;
    mjb_initialize(MJB_DB_PATH, &mjb);
    mjb_character character;

    bool ret = mjb_codepoint_character(mjb, NULL, MJB_CODEPOINT_MAX);
    mjb_assert("Void character", !ret);

    ret = mjb_codepoint_character(mjb, &character, MJB_CODEPOINT_MAX);
    mjb_assert("Not valid codepoint", !ret);

    ret = mjb_codepoint_character(mjb, &character, 0);
    mjb_assert("Codepoint: 0", strcmp((char*)character.name, "NULL") == 0);

    ret = mjb_codepoint_character(mjb, &character, '$');
    mjb_assert("Codepoint: $", strcmp((char*)character.name, "DOLLAR SIGN") == 0);

    /* 0xE0 = à */
    ret = mjb_codepoint_character(mjb, &character, 0xE0);
    mjb_assert("Codepoint: à", strcmp((char*)character.name, "LATIN SMALL LETTER A WITH GRAVE") == 0);

    /* 0x1F642 = 🙂 */
    ret = mjb_codepoint_character(mjb, &character, 0x1F642);
    mjb_assert("Codepoint: 🙂", strcmp((char*)character.name, "SLIGHTLY SMILING FACE") == 0);

    /* 0x0377 = ͷ, 0x0377 + 1 is not mapped */
    ret = mjb_codepoint_character(mjb, &character, 0x0377 + 1);
    mjb_assert("Codepoint not mapped: ͷ + 1", !ret);

    mjb_close(mjb);
}

MJB_EXPORT void mjb_codepoint_block_test() {
    mojibake *mjb;
    mjb_initialize(MJB_DB_PATH, &mjb);

    mjb_character character;

    bool ret = mjb_codepoint_character(mjb, &character, 0);
    mjb_assert("Basic Latin block", character.block == MJB_BLOCK_BASIC_LATIN);

    ret = mjb_codepoint_character(mjb, &character, 0x80 - 1);
    mjb_assert("Basic Latin end block", character.block == MJB_BLOCK_BASIC_LATIN);

    ret = mjb_codepoint_character(mjb, &character, 0x80 + 1);
    mjb_assert("Latin-1 Supplement block", character.block == MJB_BLOCK_LATIN_1_SUPPLEMENT);

    /* 0x0377 = ͷ, 0x0377 + 1 is not mapped */
    ret = mjb_codepoint_character(mjb, &character, 0x0377 + 1);
    mjb_assert("Greek and Coptic not mapped: ͷ + 1", !ret);

    ret = mjb_codepoint_character(mjb, &character, 0xE0000 + 1);
    mjb_assert("Tags block", character.block == MJB_BLOCK_TAGS);

    ret = mjb_codepoint_character(mjb, &character, 0xF0000 + 3);
    mjb_assert("Supplementary Private Use Area-A block", !ret);

    ret = mjb_codepoint_character(mjb, &character, MJB_CODEPOINT_MAX - 1);
    mjb_assert("Supplementary Private Use Area-B block", !ret);

    mjb_close(mjb);
}

MJB_EXPORT void mjb_codepoint_is_test() {
    mojibake *mjb;
    mjb_initialize(MJB_DB_PATH, &mjb);

    bool ret = mjb_codepoint_is(mjb, 0, MJB_CATEGORY_CC);
    mjb_assert("NULL: category other, control", ret);

    /* 0x1F642 = 🙂 */
    ret = mjb_codepoint_is(mjb, 0x1F642, MJB_CATEGORY_SO);
    mjb_assert("🙂: category Symbol, Other", ret);

    ret = mjb_codepoint_is(mjb, 0x1FFFE, MJB_CATEGORY_LU);
    mjb_assert("Not valid codepoint: category invalid", !ret);

    /* 0x0377 = ͷ, 0x0377 + 1 is not mapped */
    ret = mjb_codepoint_is(mjb, 0x0377 + 1, MJB_CATEGORY_LL);
    mjb_assert("Not mapped codepoint: category invalid", !ret);

    mjb_close(mjb);
}

MJB_EXPORT void mjb_codepoint_is_graphic_test() {
    mojibake *mjb;
    mjb_initialize(MJB_DB_PATH, &mjb);

    bool ret = mjb_codepoint_is_graphic(mjb, 0);
    mjb_assert("NULL: not graphic", !ret);

    ret = mjb_codepoint_is_graphic(mjb, '#');
    mjb_assert("#: graphic", ret);

    /* 0x1F642 = 🙂 */
    ret = mjb_codepoint_is_graphic(mjb, 0x1F642);
    mjb_assert("🙂: graphic", ret);

    ret = mjb_codepoint_is_graphic(mjb, 0x1FFFE);
    mjb_assert("Not valid codepoint: not graphic", !ret);

    /* 0x0377 = ͷ, 0x0377 + 1 is not mapped */
    ret = mjb_codepoint_is_graphic(mjb, 0x0377 + 1);
    mjb_assert("Not mapped codepoint: not graphic", !ret);

    mjb_close(mjb);
}

MJB_EXPORT void mjb_codepoint_is_valid_test() {
    bool validity = mjb_codepoint_is_valid(NULL, MJB_CODEPOINT_MIN + 1);
    mjb_assert("Valid codepoint", validity);

    validity = mjb_codepoint_is_valid(NULL, MJB_CODEPOINT_MIN - 1);
    mjb_assert("Not valid negative codepoint", !validity);

    validity = mjb_codepoint_is_valid(NULL, MJB_CODEPOINT_MAX + 1);
    mjb_assert("Not valid exceed codepoint", !validity);

    validity = mjb_codepoint_is_valid(NULL, 0x1FFFE);
    mjb_assert("Not valid codepoint ending in 0xFFFE", !validity);

    validity = mjb_codepoint_is_valid(NULL, 0x1FFFF);
    mjb_assert("Not valid codepoint ending in 0xFFFF", !validity);

    validity = mjb_codepoint_is_valid(NULL, 0xFFFE);
    mjb_assert("Not valid codepoint 0xFFFE", !validity);

    validity = mjb_codepoint_is_valid(NULL, 0xFFFF);
    mjb_assert("Not valid codepoint 0xFFFF", !validity);

    char buffer[32];

    /* 32 noncharacters: U+FDD0–U+FDEF */
    for(mjb_codepoint i = 0xFDD0; i <= 0xFDEF; ++i) {
        validity = mjb_codepoint_is_valid(NULL, i);

        snprintf(buffer, 32, "Not valid codepoint %#X", i);
        mjb_assert(buffer, !validity);
    }
}

MJB_EXPORT void mjb_codepoint_lc_uc_tc_test() {
    mojibake *mjb;
    mjb_initialize(MJB_DB_PATH, &mjb);

    mjb_codepoint ret = mjb_codepoint_to_lowercase(mjb, '#');
    mjb_assert("Lowercase #: #", ret == '#');

    ret = mjb_codepoint_to_uppercase(mjb, '#');
    mjb_assert("Uppercase #: #", ret == '#');

    ret = mjb_codepoint_to_titlecase(mjb, '#');
    mjb_assert("Titlecase #: #", ret == '#');

    ret = mjb_codepoint_to_lowercase(mjb, 'A');
    mjb_assert("Lowercase: A > a", ret == 'a');

    ret = mjb_codepoint_to_lowercase(mjb, 'a');
    mjb_assert("Lowercase: a > a", ret == 'a');

    ret = mjb_codepoint_to_uppercase(mjb, 'b');
    mjb_assert("Uppercase: b > B", ret == 'B');

    ret = mjb_codepoint_to_uppercase(mjb, 'B');
    mjb_assert("Uppercase: B > B", ret == 'B');

    ret = mjb_codepoint_to_titlecase(mjb, 'c');
    mjb_assert("Titlecase: c > C", ret == 'C');

    ret = mjb_codepoint_to_titlecase(mjb, 'C');
    mjb_assert("Titlecase: C > C", ret == 'C');

    mjb_close(mjb);
}
