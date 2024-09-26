#include <stdio.h>
#include <string.h>

#include "test.h"

void *test_codepoint(void *arg) {
    mjb_character character;

    ATT_ASSERT(mjb_codepoint_character(NULL, MJB_CODEPOINT_MAX), false, "Void character")
    ATT_ASSERT(mjb_codepoint_character(&character, MJB_CODEPOINT_MAX), false, "Not valid codepoint")

    ATT_ASSERT(mjb_codepoint_character(&character, 0), true, "Codepoint: 0")
    /* ATT_ASSERT(strcmp((char*)character.name, "NULL"), 0, "Codepoint: 0")

    ATT_ASSERT(mjb_codepoint_character(&character, '$'), true, "Codepoint: 0")
    ATT_ASSERT(strcmp((char*)character.name, "DOLLAR SIGN"), 0, "Codepoint: $");

    // 0xE0 = à
    ret = mjb_codepoint_character(&character, 0xE0);
    ATT_ASSERT("Codepoint: à", strcmp((char*)character.name, "LATIN SMALL LETTER A WITH GRAVE") == 0);

    // 0x1F642 = 🙂
    ret = mjb_codepoint_character(&character, 0x1F642);
    mjb_print_character(&character, 0x1F642);
    ATT_ASSERT("Codepoint: 🙂", strcmp((char*)character.name, "SLIGHTLY SMILING FACE") == 0);

    // 0x0377 = ͷ, 0x0377 + 1 is not mapped
    ret = mjb_codepoint_character(&character, 0x0377 + 1);
    ATT_ASSERT("Codepoint not mapped: ͷ + 1", !ret);

    bool ret = mjb_codepoint_character(&character, 0);
    ATT_ASSERT("Basic Latin block", character.block == MJB_BLOCK_BASIC_LATIN);

    ret = mjb_codepoint_character(&character, 0x80 - 1);
    ATT_ASSERT("Basic Latin end block", character.block == MJB_BLOCK_BASIC_LATIN);

    ret = mjb_codepoint_character(&character, 0x80 + 1);
    ATT_ASSERT("Latin-1 Supplement block", character.block == MJB_BLOCK_LATIN_1_SUPPLEMENT);

    // 0x0377 = ͷ, 0x0377 + 1 is not mapped
    ret = mjb_codepoint_character(&character, 0x0377 + 1);
    ATT_ASSERT("Greek and Coptic not mapped: ͷ + 1", !ret);

    ret = mjb_codepoint_character(&character, 0xE0000 + 1);
    ATT_ASSERT("Tags block", character.block == MJB_BLOCK_TAGS);

    ret = mjb_codepoint_character(&character, 0xF0000 + 3);
    ATT_ASSERT("Supplementary Private Use Area-A block", !ret);

    ret = mjb_codepoint_character(&character, MJB_CODEPOINT_MAX - 1);
    ATT_ASSERT("Supplementary Private Use Area-B block", !ret);

    bool ret = mjb_codepoint_is(0, MJB_CATEGORY_CC);
    ATT_ASSERT("NULL: category other, control", ret);

    // 0x1F642 = 🙂
    ret = mjb_codepoint_is(0x1F642, MJB_CATEGORY_SO);
    ATT_ASSERT("🙂: category Symbol, Other", ret);

    ret = mjb_codepoint_is(0x1FFFE, MJB_CATEGORY_LU);
    ATT_ASSERT("Not valid codepoint: category invalid", !ret);

    // 0x0377 = ͷ, 0x0377 + 1 is not mapped
    ret = mjb_codepoint_is(0x0377 + 1, MJB_CATEGORY_LL);
    ATT_ASSERT("Not mapped codepoint: category invalid", !ret);

    bool ret = mjb_codepoint_is_graphic(0);
    ATT_ASSERT("NULL: not graphic", !ret);

    ret = mjb_codepoint_is_graphic('#');
    ATT_ASSERT("#: graphic", ret);

    // 0x1F642 = 🙂
    ret = mjb_codepoint_is_graphic(0x1F642);
    ATT_ASSERT("🙂: graphic", ret);

    ret = mjb_codepoint_is_graphic(0x1FFFE);
    ATT_ASSERT("Not valid codepoint: not graphic", !ret);

    // 0x0377 = ͷ, 0x0377 + 1 is not mapped
    ret = mjb_codepoint_is_graphic(0x0377 + 1);
    ATT_ASSERT("Not mapped codepoint: not graphic", !ret);

    bool validity = mjb_codepoint_is_valid(MJB_CODEPOINT_MIN + 1);
    ATT_ASSERT("Valid codepoint", validity);

    validity = mjb_codepoint_is_valid(MJB_CODEPOINT_MIN - 1);
    ATT_ASSERT("Not valid negative codepoint", !validity);

    validity = mjb_codepoint_is_valid(MJB_CODEPOINT_MAX + 1);
    ATT_ASSERT("Not valid exceed codepoint", !validity);

    validity = mjb_codepoint_is_valid(0x1FFFE);
    ATT_ASSERT("Not valid codepoint ending in 0xFFFE", !validity);

    validity = mjb_codepoint_is_valid(0x1FFFF);
    ATT_ASSERT("Not valid codepoint ending in 0xFFFF", !validity);

    validity = mjb_codepoint_is_valid(0xFFFE);
    ATT_ASSERT("Not valid codepoint 0xFFFE", !validity);

    validity = mjb_codepoint_is_valid(0xFFFF);
    ATT_ASSERT("Not valid codepoint 0xFFFF", !validity);

    char buffer[32];

    // 32 noncharacters: U+FDD0–U+FDEF
    for(mjb_codepoint i = 0xFDD0; i <= 0xFDEF; ++i) {
        validity = mjb_codepoint_is_valid(i);

        snprintf(buffer, 32, "Not valid codepoint %#X", i);
        ATT_ASSERT(buffer, !validity);
    }

    mjb_codepoint ret = mjb_codepoint_to_lowercase('#');
    ATT_ASSERT("Lowercase #: #", ret == '#');

    ret = mjb_codepoint_to_uppercase('#');
    ATT_ASSERT("Uppercase #: #", ret == '#');

    ret = mjb_codepoint_to_titlecase('#');
    ATT_ASSERT("Titlecase #: #", ret == '#');

    ret = mjb_codepoint_to_lowercase('A');
    ATT_ASSERT("Lowercase: A > a", ret == 'a');

    ret = mjb_codepoint_to_lowercase('a');
    ATT_ASSERT("Lowercase: a > a", ret == 'a');

    ret = mjb_codepoint_to_uppercase('b');
    ATT_ASSERT("Uppercase: b > B", ret == 'B');

    ret = mjb_codepoint_to_uppercase('B');
    ATT_ASSERT("Uppercase: B > B", ret == 'B');

    ret = mjb_codepoint_to_titlecase('c');
    ATT_ASSERT("Titlecase: c > C", ret == 'C');

    ret = mjb_codepoint_to_titlecase('C');
    ATT_ASSERT("Titlecase: C > C", ret == 'C');*/

    return NULL;
}
