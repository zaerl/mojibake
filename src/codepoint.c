#include <string.h>

#include "mojibake.h"
#include "db.h"

static const mjb_character empty_character;

/* Return true if the codepoint is valid */
MJB_EXPORT bool mjb_codepoint_is_valid(mjb_codepoint codepoint) {
    if(codepoint < MJB_CODEPOINT_MIN || codepoint > MJB_CODEPOINT_MAX ||
        (codepoint >= 0xFDD0 && codepoint <= 0xFDEF) || /* Noncharacter */
        (codepoint & 0xFFFE) == 0xFFFE || (codepoint & 0xFFFF) == 0xFFFF) { /* Noncharacter */
        return false;
    }

    return true;
}

/* Return the codepoint character */
MJB_EXPORT bool mjb_codepoint_character(mjb_character *character, mjb_codepoint codepoint) {
    if(character == NULL || !mjb_codepoint_is_valid(codepoint) || !mjb_ready()) {
        return false;
    }

    /* Reset character */
    *character = empty_character;

    int ret = sqlite3_bind_int(mjb.char_stmt, 1, codepoint);
    DB_CHECK(ret, false)

    ret = sqlite3_step(mjb.char_stmt);
    bool found = ret == SQLITE_ROW;

    if(found) {
        DB_COLUMN_INT(mjb.char_stmt, character->codepoint, 0);
        DB_COLUMN_TEXT(mjb.char_stmt, character->name, 1)
        DB_COLUMN_INT(mjb.char_stmt, character->block, 2);
        DB_COLUMN_INT(mjb.char_stmt, character->category, 3);
        DB_COLUMN_INT(mjb.char_stmt, character->combining, 4);
        DB_COLUMN_INT(mjb.char_stmt, character->bidirectional, 5);
        DB_COLUMN_TEXT(mjb.char_stmt, character->decimal, 6)
        DB_COLUMN_TEXT(mjb.char_stmt, character->digit, 7)
        DB_COLUMN_TEXT(mjb.char_stmt, character->numeric, 8)
        DB_COLUMN_INT(mjb.char_stmt, character->mirrored, 9);
        DB_COLUMN_INT(mjb.char_stmt, character->uppercase, 10);
        DB_COLUMN_INT(mjb.char_stmt, character->lowercase, 11);
        DB_COLUMN_INT(mjb.char_stmt, character->titlecase, 12);
    }

    ret = sqlite3_clear_bindings(mjb.char_stmt);
    DB_CHECK(ret, false)

    ret = sqlite3_reset(mjb.char_stmt);
    DB_CHECK(ret, false)

    return found;
}

/* Return true if the codepoint has the category */
MJB_EXPORT bool mjb_codepoint_is(mjb_codepoint codepoint, mjb_category category) {
    mjb_character character;

    if(!mjb_codepoint_character(&character, codepoint)) {
        return false;
    }

    return character.category == category;
}

/* Return true if the codepoint is graphic */
MJB_EXPORT bool mjb_codepoint_is_graphic(mjb_codepoint codepoint) {
    mjb_character character;

    if(!mjb_codepoint_character(&character, codepoint)) {
        return false;
    }

    /* All C categories can be printed */
    switch(character.category) {
        case MJB_CATEGORY_CC:
        case MJB_CATEGORY_CF:
        case MJB_CATEGORY_CS:
        case MJB_CATEGORY_CO:
        case MJB_CATEGORY_CN:
            return false;
        default:
            return true;
    }
}

/* Return the codepoint lowercase codepoint */
MJB_EXPORT mjb_codepoint mjb_codepoint_to_lowercase(mjb_codepoint codepoint) {
    mjb_character character;

    if(!mjb_codepoint_character(&character, codepoint)) {
        return codepoint;
    }

    return character.lowercase == 0 ? codepoint : character.lowercase;
}

/* Return the codepoint uppercase codepoint */
MJB_EXPORT mjb_codepoint mjb_codepoint_to_uppercase(mjb_codepoint codepoint) {
    mjb_character character;

    if(!mjb_codepoint_character(&character, codepoint)) {
        return codepoint;
    }

    return character.uppercase == 0 ? codepoint : character.uppercase;
}

/* Return the codepoint titlecase codepoint */
MJB_EXPORT mjb_codepoint mjb_codepoint_to_titlecase(mjb_codepoint codepoint) {
    mjb_character character;

    if(!mjb_codepoint_character(&character, codepoint)) {
        return codepoint;
    }

    return character.titlecase == 0 ? codepoint : character.titlecase;
}
