/**
 * The mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <string.h>

#include "test.h"

void *test_codepoint(void *arg) {
    mjb_character character;

    ATT_ASSERT(mjb_codepoint_character(NULL, MJB_CODEPOINT_MAX), false, "Void character")
    ATT_ASSERT(mjb_codepoint_character(&character, MJB_CODEPOINT_MAX), false, "Not valid codepoint")

    ATT_ASSERT(mjb_codepoint_character(&character, 0), true, "Codepoint: 0")
    ATT_ASSERT(strcmp((char*)character.name, "NULL"), 0, "Codepoint: 0")

    ATT_ASSERT(mjb_codepoint_character(&character, '$'), true, "Codepoint: 0")
    ATT_ASSERT(strcmp((char*)character.name, "DOLLAR SIGN"), 0, "Codepoint: $")

    // 0xE0 = à
    ATT_ASSERT(mjb_codepoint_character(&character, 0xE0), true, "Codepoint: à")
    ATT_ASSERT(strcmp((char*)character.name, "LATIN SMALL LETTER A WITH GRAVE"), 0, "Codepoint: à")

    // 0x1F642 = 🙂
    ATT_ASSERT(mjb_codepoint_character(&character, 0x1F642), true, "Codepoint: 🙂")
    ATT_ASSERT(strcmp((char*)character.name, "SLIGHTLY SMILING FACE"), 0, "Codepoint: 🙂")

    // 0x0377 = ͷ, 0x0377 + 1 is not mapped
    ATT_ASSERT(mjb_codepoint_character(&character, 0x0377 + 1), false, "Codepoint not mapped: ͷ + 1")

    // 0xAC00 = First hangul syllable
    ATT_ASSERT(mjb_codepoint_character(&character, MJB_CODEPOINT_HANGUL_START), true, "First hangul syllable")
    ATT_ASSERT(strcmp((char*)character.name, "HANGUL SYLLABLE GA"), 0, "First hangul syllable")

    // 0xD7A3 = Last hangul syllable
    ATT_ASSERT(mjb_codepoint_character(&character, MJB_CODEPOINT_HANGUL_END), true, "Last hangul syllable")
    ATT_ASSERT(strcmp((char*)character.name, "HANGUL SYLLABLE HIH"), 0, "Last hangul syllable")

    ATT_ASSERT(mjb_codepoint_block_is(MJB_CODEPOINT_MAX, MJB_BLOCK_BASIC_LATIN), false, "Not valid codepoint")
    ATT_ASSERT(mjb_codepoint_block_is(0, MJB_BLOCK_BASIC_LATIN), true, "Basic Latin block")
    ATT_ASSERT(mjb_codepoint_block_is(0x80 - 1, MJB_BLOCK_BASIC_LATIN), true, "Basic Latin block")
    ATT_ASSERT(mjb_codepoint_block_is(0x80 + 1, MJB_BLOCK_LATIN_1_SUPPLEMENT), true, "Latin-1 Supplement block")
    ATT_ASSERT(mjb_codepoint_block_is(0x80 + 1, MJB_BLOCK_LATIN_1_SUPPLEMENT), true, "Latin-1 Supplement block")
    // 0x0377 = ͷ, 0x0377 + 1 is not mapped
    ATT_ASSERT(mjb_codepoint_block_is(0x0377 + 1, MJB_BLOCK_LATIN_1_SUPPLEMENT), false, "Greek and Coptic not mapped: ͷ + 1")
    ATT_ASSERT(mjb_codepoint_block_is(0xE0000 + 1, MJB_BLOCK_TAGS), true, "Tags block")

    ATT_ASSERT(mjb_codepoint_character(&character, 0xF0000 + 3), false, "Supplementary Private Use Area-A block")

    ATT_ASSERT(mjb_codepoint_character(&character, MJB_CODEPOINT_MAX - 1), false, "Supplementary Private Use Area-B block")

    ATT_ASSERT(mjb_codepoint_category_is(0, MJB_CATEGORY_CC), true, "NULL: category other, control")

    // 0x1F642 = 🙂
    ATT_ASSERT(mjb_codepoint_category_is(0x1F642, MJB_CATEGORY_SO), true, "🙂: category Symbol, Other")

    ATT_ASSERT(mjb_codepoint_category_is(0x1FFFE, MJB_CATEGORY_LU), false, "Not valid codepoint: category invalid")

    // 0x0377 = ͷ, 0x0377 + 1 is not mapped
    ATT_ASSERT(mjb_codepoint_category_is(0x0377 + 1, MJB_CATEGORY_LL), false, "Not mapped codepoint: category invalid")

    ATT_ASSERT(mjb_codepoint_is_graphic(0), false, "NULL: not graphic")

    ATT_ASSERT(mjb_codepoint_is_graphic('#'), true, "#: graphic")

    // 0x1F642 = 🙂
    ATT_ASSERT(mjb_codepoint_is_graphic(0x1F642), true, "🙂: graphic")

    ATT_ASSERT(mjb_codepoint_is_graphic(0x1FFFE), false, "Not valid codepoint: not graphic")

    // 0x0377 = ͷ, 0x0377 + 1 is not mapped
    ATT_ASSERT(mjb_codepoint_is_graphic(0x0377 + 1), false, "Not mapped codepoint: not graphic")

    ATT_ASSERT(mjb_codepoint_is_valid(MJB_CODEPOINT_MIN + 1), true, "Valid codepoint")
    ATT_ASSERT(mjb_codepoint_is_valid(MJB_CODEPOINT_MIN - 1), false, "Not valid negative codepoint")
    ATT_ASSERT(mjb_codepoint_is_valid(MJB_CODEPOINT_MAX + 1), false, "Not valid exceed codepoint")
    ATT_ASSERT(mjb_codepoint_is_valid(0x1FFFE), false, "Not valid codepoint ending in 0xFFFE")
    ATT_ASSERT(mjb_codepoint_is_valid(0x1FFFF), false, "Not valid codepoint ending in 0xFFFF")
    ATT_ASSERT(mjb_codepoint_is_valid(0xFFFE), false, "Not valid codepoint 0xFFFE")
    ATT_ASSERT(mjb_codepoint_is_valid(0xFFFF), false, "Not valid codepoint 0xFFFF")

    char buffer[32];

    // 32 noncharacters: U+FDD0–U+FDEF
    for(mjb_codepoint i = 0xFDD0; i <= 0xFDEF; ++i) {
        snprintf(buffer, 32, "Not valid codepoint %#X", i);
        ATT_ASSERT(mjb_codepoint_is_valid(i), false, buffer)
    }

    ATT_ASSERT(mjb_codepoint_to_lowercase('#'), '#', "Lowercase #: #")
    ATT_ASSERT(mjb_codepoint_to_uppercase('#'), '#', "Uppercase #: #")
    ATT_ASSERT(mjb_codepoint_to_titlecase('#'), '#', "Titlecase #: #")
    ATT_ASSERT(mjb_codepoint_to_lowercase('A'), 'a', "Lowercase: A > a")
    ATT_ASSERT(mjb_codepoint_to_lowercase('a'), 'a', "Lowercase: a > a")
    ATT_ASSERT(mjb_codepoint_to_uppercase('b'), 'B', "Uppercase: b > B")
    ATT_ASSERT(mjb_codepoint_to_uppercase('B'), 'B', "Uppercase: B > B")
    ATT_ASSERT(mjb_codepoint_to_titlecase('c'), 'C', "Titlecase: c > C")
    ATT_ASSERT(mjb_codepoint_to_titlecase('C'), 'C', "Titlecase: C > C")

    ATT_ASSERT(mjb_codepoint_is_combining(0), false, "NULL")
    ATT_ASSERT(mjb_codepoint_is_combining(0x30), false, "DIGIT ZERO")
    ATT_ASSERT(mjb_codepoint_is_combining(0x0300), true, "COMBINING GRAVE ACCENT")
    ATT_ASSERT(mjb_codepoint_is_combining(0x1F1E6), false, "REGIONAL INDICATOR SYMBOL LETTER A")
    ATT_ASSERT(mjb_codepoint_is_combining(0x488), true, "COMBINING CYRILLIC HUNDRED THOUSANDS SIGN")

    return NULL;
}
