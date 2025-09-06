/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <string.h>

#include "test.h"

void *test_codepoint(void *arg) {
    mjb_character character;

    ATT_ASSERT(mjb_codepoint_character(MJB_CODEPOINT_MAX, &character), false, "Not valid codepoint")

    ATT_ASSERT(mjb_codepoint_character(0, &character), true, "Codepoint: 0")
    ATT_ASSERT(strcmp((char*)character.name, "NULL"), 0, "Codepoint: 0")

    ATT_ASSERT(mjb_codepoint_character('$', &character), true, "Codepoint: 0")
    ATT_ASSERT(strcmp((char*)character.name, "DOLLAR SIGN"), 0, "Codepoint: $")

    // U+E0 = à
    ATT_ASSERT(mjb_codepoint_character(0xE0, &character), true, "Codepoint: à")
    ATT_ASSERT(strcmp((char*)character.name, "LATIN SMALL LETTER A WITH GRAVE"), 0, "Codepoint: à")

    // U+1F642 = 🙂
    ATT_ASSERT(mjb_codepoint_character(0x1F642, &character), true, "Codepoint: 🙂")
    ATT_ASSERT(strcmp((char*)character.name, "SLIGHTLY SMILING FACE"), 0, "Codepoint: 🙂")

    // U+0377 = ͷ, U+0377 + 1 is not mapped
    ATT_ASSERT(mjb_codepoint_character(0x0377 + 1, &character), false, "Codepoint not mapped: ͷ + 1")

    // U+AC00 = First hangul syllable
    ATT_ASSERT(mjb_codepoint_character(MJB_CP_HANGUL_S_BASE, &character), true, "First hangul syllable")
    ATT_ASSERT(strcmp((char*)character.name, "HANGUL SYLLABLE GA"), 0, "First hangul syllable")

    // U+D7A3 = Last hangul syllable
    mjb_codepoint last_syllable = MJB_CP_HANGUL_S_BASE + MJB_CP_HANGUL_S_COUNT - 1;
    ATT_ASSERT(mjb_codepoint_character(last_syllable, &character), true, "Last hangul syllable")
    ATT_ASSERT(strcmp((char*)character.name, "HANGUL SYLLABLE HIH"), 0, "Last hangul syllable")

    mjb_codepoint_block block = {};

    ATT_ASSERT(mjb_character_block(MJB_CODEPOINT_MAX, &block), false, "Not valid codepoint")
    ATT_ASSERT(mjb_character_block(0xE0080, &block), false, "Not mapped codepoint")

    ATT_ASSERT(mjb_character_block(0, &block), true, "Valid basic Latin block 1")
    ATT_ASSERT((unsigned int)block.id, (unsigned int)MJB_BLOCK_BASIC_LATIN, "Basic Latin block 2")

    ATT_ASSERT(mjb_character_block(0x80 - 1, &block), true, "Valid basic final Latin block")
    ATT_ASSERT((unsigned int)block.id, (unsigned int)MJB_BLOCK_BASIC_LATIN, "Basic Latin block 3")

    ATT_ASSERT(mjb_character_block(0x80 + 1, &block), true, "Valid latin-1 Supplement block")
    ATT_ASSERT((unsigned int)block.id, (unsigned int)MJB_BLOCK_LATIN_1_SUPPLEMENT, "Latin-1 Supplement block")

    ATT_ASSERT(mjb_character_block(0xE0000 + 1, &block), true, "Valid tags block")
    ATT_ASSERT((unsigned int)block.id, (unsigned int)MJB_BLOCK_TAGS, "Tags block")

    ATT_ASSERT(mjb_character_block(0xC0C0, &block), true, "Hangul Syllables block")
    ATT_ASSERT((unsigned int)block.id, (unsigned int)MJB_BLOCK_HANGUL_SYLLABLES, "Hangul block")

    ATT_ASSERT(mjb_character_block(0x10C0, &block), true, "Georgian block")
    ATT_ASSERT((unsigned int)block.id, (unsigned int)MJB_BLOCK_GEORGIAN, "Georgian block")

    ATT_ASSERT(mjb_codepoint_character(0xF0000 + 3, &character), false, "Supplementary Private Use Area-A block")

    ATT_ASSERT(mjb_codepoint_character(MJB_CODEPOINT_MAX - 1, &character), false, "Supplementary Private Use Area-B block")

    ATT_ASSERT(mjb_codepoint_category_is(0, MJB_CATEGORY_CC), true, "NULL: category other, control")

    // U+1F642 = 🙂
    ATT_ASSERT(mjb_codepoint_category_is(0x1F642, MJB_CATEGORY_SO), true, "🙂: category Symbol, Other")

    ATT_ASSERT(mjb_codepoint_category_is(0x1FFFE, MJB_CATEGORY_LU), false, "Not valid codepoint: category invalid")

    // U+0377 = ͷ, 0x0377 + 1 is not mapped
    ATT_ASSERT(mjb_codepoint_category_is(0x0377 + 1, MJB_CATEGORY_LL), false, "Not mapped codepoint: category invalid")

    ATT_ASSERT(mjb_codepoint_is_graphic(0), false, "NULL: not graphic")

    ATT_ASSERT(mjb_codepoint_is_graphic('#'), true, "#: graphic")

    // U+1F642 = 🙂
    ATT_ASSERT(mjb_codepoint_is_graphic(0x1F642), true, "🙂: graphic")

    ATT_ASSERT(mjb_codepoint_is_graphic(0x1FFFE), false, "Not valid codepoint: not graphic")

    // U+0377 = ͷ, U+0377 + 1 is not mapped
    ATT_ASSERT(mjb_codepoint_is_graphic(0x0377 + 1), false, "Not mapped codepoint: not graphic")

    ATT_ASSERT(mjb_codepoint_is_valid(MJB_CODEPOINT_MIN + 1), true, "Valid codepoint")
    ATT_ASSERT(mjb_codepoint_is_valid(MJB_CODEPOINT_MIN - 1), false, "Not valid negative codepoint")
    ATT_ASSERT(mjb_codepoint_is_valid(MJB_CODEPOINT_MAX + 1), false, "Not valid exceed codepoint")
    ATT_ASSERT(mjb_codepoint_is_valid(0x1FFFE), false, "Not valid codepoint ending in 0xFFFE")
    ATT_ASSERT(mjb_codepoint_is_valid(0x1FFFF), false, "Not valid codepoint ending in 0xFFFF")
    ATT_ASSERT(mjb_codepoint_is_valid(0xFFFE), false, "Not valid codepoint 0xFFFE")
    ATT_ASSERT(mjb_codepoint_is_valid(0xFFFF), false, "Not valid codepoint 0xFFFF")

    char buffer[32];

    // 32 noncharacters: U+FDD0 - U+FDEF
    for(mjb_codepoint i = 0xFDD0; i <= 0xFDEF; ++i) {
        snprintf(buffer, 32, "Not valid codepoint %#X", i);
        // CURRENT_COUNT 32
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

    // U+00BD = ½
    ATT_ASSERT(mjb_codepoint_character(0x00BD, &character), true, "Codepoint: ½ 1")
    ATT_ASSERT(character.decimal == MJB_NUMBER_NOT_VALID, true, "Codepoint: ½ 2")
    ATT_ASSERT(character.digit == MJB_NUMBER_NOT_VALID, true, "Codepoint: ½ 3")
    ATT_ASSERT((const char*)character.numeric, (const char*)"1/2", "Codepoint: ½ 4")

    // U+0030 = 1
    ATT_ASSERT(mjb_codepoint_character(0x0031, &character), true, "Codepoint: 1")
    ATT_ASSERT(character.decimal == 1, true, "Codepoint: 1 2")
    ATT_ASSERT(character.digit == 1, true, "Codepoint: 1 3")
    ATT_ASSERT((const char*)character.numeric, (const char*)"1", "Codepoint: 1 4")

    // EGYPTIAN HIEROGLYPH
    ATT_ASSERT(mjb_codepoint_character(0x13000, &character), true, "HIEROGLYPH 1")
    ATT_ASSERT(strcmp((char*)character.name, "EGYPTIAN HIEROGLYPH A001"), (int)0, "HIEROGLYPH 1")

    ATT_ASSERT(mjb_codepoint_character(0x13455, &character), true, "HIEROGLYPH 2")
    ATT_ASSERT(strcmp((char*)character.name, "EGYPTIAN HIEROGLYPH MODIFIER DAMAGED"), 0, "HIEROGLYPH 2")

    ATT_ASSERT(mjb_codepoint_character(0x13460, &character), true, "EGYPTIAN HIEROGLYPH 3")
    ATT_ASSERT(strcmp((char*)character.name, "EGYPTIAN HIEROGLYPH-13460"), 0, "HIEROGLYPH 3")

    ATT_ASSERT(mjb_codepoint_character(0x143FA, &character), true, "EGYPTIAN HIEROGLYPH 4")
    ATT_ASSERT(strcmp((char*)character.name, "EGYPTIAN HIEROGLYPH-143FA"), 0, "HIEROGLYPH 4")

    ATT_ASSERT(mjb_codepoint_character(0x13460, &character), true, "EGYPTIAN HIEROGLYPH 3")
    ATT_ASSERT(strcmp((char*)character.name, "EGYPTIAN HIEROGLYPH-13460"), 0, "HIEROGLYPH 3")

    ATT_ASSERT(mjb_codepoint_character(0xF900, &character), true, "CJK CI 1")
    ATT_ASSERT(strcmp((char*)character.name, "CJK COMPATIBILITY IDEOGRAPH-F900"), 0, "CJK CI 1")

    ATT_ASSERT(mjb_codepoint_character(0xFAD9, &character), true, "CJK CI 2")
    ATT_ASSERT(strcmp((char*)character.name, "CJK COMPATIBILITY IDEOGRAPH-FAD9"), 0, "CJK CI 2")

    ATT_ASSERT(mjb_codepoint_character(0x14400, &character), true, "ANATOLIAN HIEROGLYPH 1")
    ATT_ASSERT(strcmp((char*)character.name, "ANATOLIAN HIEROGLYPH A001"), (int)0, "ANATOLIAN HIEROGLYPH 1")

    ATT_ASSERT(mjb_codepoint_character(0x14646, &character), true, "ANATOLIAN HIEROGLYPH 2")
    ATT_ASSERT((char*)character.name, (char*)"ANATOLIAN HIEROGLYPH A530", "ANATOLIAN HIEROGLYPH 2")

    return NULL;
}
