/**
 * The mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mojibake.h"
#include "sqlite/sqlite3.h"

#define DB_CHECK(db_result, ret) if(db_result != SQLITE_OK) { mjb_db_error(); return ret; }
#define DB_CHECK_CLOSE(db_result, ret) if(db_result != SQLITE_OK) { mjb_db_error(); mjb_close(); return ret; }
#define DB_COLUMN_INT(stmt, name, col) name = sqlite3_column_int(stmt, col);
#define DB_COLUMN_TEXT(stmt, name, col) strncpy((char*)&name, (const char*)sqlite3_column_text(stmt, col), sqlite3_column_bytes(stmt, col));

#define MJB_ENCODING_UTF_8_BOM "\xEF\xBB\xBF"
#define MJB_ENCODING_UTF_16_BE_BOM "\xFE\xFF"
#define MJB_ENCODING_UTF_16_LE_BOM "\xFF\xFE"
#define MJB_ENCODING_UTF_32_BE_BOM "\x00\x00\xFE\xFF"
#define MJB_ENCODING_UTF_32_LE_BOM "\xFF\xFE\x00\x00"

typedef struct mjb_connection {
    sqlite3* db;
    sqlite3_stmt* char_stmt;
    bool ok;
    mjb_alloc memory_alloc;
    mjb_realloc memory_realloc;
    mjb_free memory_free;
} mjb_connection;

static mjb_connection mjb_internal = { NULL, NULL, false };
static const mjb_character empty_character;

static mjb_encoding mjb_encoding_from_bom(const char* buffer, size_t length) {
    if(length < 2) {
        /* BOM are at least 2 characters */
        return MJB_ENCODING_UNKNOWN;
    }

    if(length >= 3) {
        if(memcmp(buffer, MJB_ENCODING_UTF_8_BOM, 3) == 0) {
            return MJB_ENCODING_UTF_8;
        }
    }

    mjb_encoding bom_encoding = MJB_ENCODING_UNKNOWN;

    if(length >= 4) {
        if(memcmp(buffer, MJB_ENCODING_UTF_32_BE_BOM, 4) == 0) {
            bom_encoding = MJB_ENCODING_UTF_32_BE;
        } else if(memcmp(buffer, MJB_ENCODING_UTF_32_LE_BOM, 4) == 0) {
            bom_encoding = MJB_ENCODING_UTF_32_LE;
        }
    }

    if(length >= 2) {
        if(memcmp(buffer, MJB_ENCODING_UTF_16_BE_BOM, 2) == 0) {
            bom_encoding = MJB_ENCODING_UTF_16_BE;
        } else if(memcmp(buffer, MJB_ENCODING_UTF_16_LE_BOM, 2) == 0) {
            /* A UTF-32-LE document is also valid UTF-16-LE */
            bom_encoding |= MJB_ENCODING_UTF_16_LE;
        }
    }

    return bom_encoding;
}

static void mjb_db_error() {
    fprintf(stderr, "Error: %s\n", sqlite3_errmsg(mjb_internal.db));
}

/* Initialize the library */
MJB_EXPORT bool mjb_initialize(const char* filename) {
    if(mjb_ready()) {
        return true;
    }

    int ret = sqlite3_open_v2(filename, &mjb_internal.db, SQLITE_OPEN_READONLY, NULL);
    DB_CHECK_CLOSE(ret, false)

    ret = sqlite3_prepare_v3(mjb_internal.db, "SELECT * FROM characters WHERE codepoint = ?", -1,
        SQLITE_PREPARE_PERSISTENT, &mjb_internal.char_stmt, NULL);
    DB_CHECK_CLOSE(ret, false)

    mjb_internal.ok = true;

    mjb_internal.memory_alloc = &malloc;
    mjb_internal.memory_realloc = &realloc;
    mjb_internal.memory_free = &free;

    return true;
}

/* The library is ready */
MJB_EXPORT bool mjb_ready() {
    return mjb_internal.ok;
}

/* Close the library */
MJB_EXPORT bool mjb_close() {
    if(!mjb_ready()) {
        mjb_db_error();

        return false;
    }

    int ret = SQLITE_ERROR;

    if(mjb_internal.char_stmt) {
        ret = sqlite3_finalize(mjb_internal.char_stmt);
        mjb_internal.char_stmt = NULL;
    }

    if(mjb_internal.db) {
        ret = sqlite3_close(mjb_internal.db);
        mjb_internal.db = NULL;
    }

    mjb_internal.ok = false;

    return ret == SQLITE_OK;
}

/* Output the current library version (MJB_VERSION) */
MJB_EXPORT char* mjb_version() {
    return MJB_VERSION;
}

/* Output the current library version number (MJB_VERSION_NUMBER) */
MJB_EXPORT unsigned int mjb_version_number() {
    return MJB_VERSION_NUMBER;
}

/* Output the current supported unicode version (MJB_UNICODE_VERSION) */
MJB_EXPORT char* mjb_unicode_version() {
    return MJB_UNICODE_VERSION;
}

/* Return true if the codepoint is valid */
MJB_EXPORT bool mjb_codepoint_is_valid(mjb_codepoint codepoint) {
    if(codepoint < MJB_CODEPOINT_MIN || codepoint > MJB_CODEPOINT_MAX ||
        (codepoint >= 0xFDD0 && codepoint <= 0xFDEF) || /* Noncharacter */
        (codepoint & 0xFFFE) == 0xFFFE || (codepoint & 0xFFFF) == 0xFFFF) { /* Noncharacter */
        return false;
    }

    return true;
}

/* Return true if the plane is valid */
MJB_EXPORT bool mjb_plane_is_valid(mjb_plane plane) {
    return plane >= 0 && plane < MJB_PLANE_NUM;
}

/* Return the name of a plane, NULL if the place specified is not valid */
MJB_EXPORT const char* mjb_plane_name(mjb_plane plane, bool abbreviation) {
    if(!mjb_plane_is_valid(plane)) {
        return NULL;
    }

    switch(plane) {
        case 0:
            return abbreviation ? "BMP" : "Basic Multilingual Plane";

        case 1:
            return abbreviation ? "SMP" : "Supplementary Multilingual Plane";

        case 2:
            return abbreviation ? "SIP" : "Supplementary Ideographic Plane";

        case 3:
            return abbreviation ? "TIP" : "Tertiary Ideographic Plane";

        case 14:
            return abbreviation ? "SSP" : "Supplementary Special-purpose Plane";

        case 15:
            return abbreviation ? "PUA-A" : "Supplementary Private Use Area-A";

        case 16:
            return abbreviation ? "PUA-B" : "Supplementary Private Use Area-B";

        default:
            return "Unassigned";
    }
}

/* Return the string encoding (the most probable) */
MJB_EXPORT mjb_encoding mjb_string_encoding(const char *buffer, size_t size) {
    if(buffer == 0 || size == 0) {
        return MJB_ENCODING_UNKNOWN;
    }

    mjb_encoding bom_encoding = mjb_encoding_from_bom(buffer, size);

    if(bom_encoding != MJB_ENCODING_UNKNOWN) {
        return bom_encoding;
    }

    /* No BOM, let's try UTF-8 */
    if(mjb_string_is_utf8(buffer, size)) {
        bom_encoding |= MJB_ENCODING_UTF_8;
    }

    /* No BOM, let's try ASCII */
    if(mjb_string_is_ascii(buffer, size)) {
        bom_encoding |= MJB_ENCODING_ASCII;
    }

    return bom_encoding;
}

/* Return true if the string is encoded in UTF-8 */
MJB_EXPORT bool mjb_string_is_utf8(const char* buffer, size_t size) {
    const char* end = buffer + size;
    unsigned char byte;
    unsigned int code_length, i;
    uint32_t ch;

    if(buffer == 0 || size == 0) {
        return false;
    }

    while(buffer != end) {
        byte = *buffer;

        if(byte <= 0x7F) {
            /* 0b0xxxxxxx, 1 byte sequence */
            buffer += 1;
            continue;
        }

        if(0xC2 <= byte && byte <= 0xDF) {
            /* 0b110xxxxx: 2 bytes sequence */
            code_length = 2;
        } else if(0xE0 <= byte && byte <= 0xEF) {
            /* 0b1110xxxx: 3 bytes sequence */
            code_length = 3;
        } else if(0xF0 <= byte && byte <= 0xF4) {
            /* 0b11110xxx: 4 bytes sequence */
            code_length = 4;
        } else {
            /* invalid first byte of a multibyte character */
            return false;
        }

        if(buffer + (code_length - 1) >= end) {
            /* truncated string or invalid byte sequence */
            return false;
        }

        /* Check continuation bytes: bit 7 should be set, bit 6 should be
         * unset (b10xxxxxx). */
        for(i = 1; i < code_length; ++i) {
            if((buffer[i] & 0xC0) != 0x80) {
                return false;
            }
        }

        if(code_length == 2) {
            /* 2 bytes sequence: U+0080..U+07FF */
            ch = ((buffer[0] & 0x1f) << 6) + (buffer[1] & 0x3f);
            /* buffer[0] >= 0xC2, so ch >= 0x0080.
             buffer[0] <= 0xDF, (buffer[1] & 0x3f) <= 0x3f, so ch <= 0x07ff */
        } else if(code_length == 3) {
            /* 3 bytes sequence: U+0800..U+FFFF */
            ch = ((buffer[0] & 0x0f) << 12) + ((buffer[1] & 0x3f) << 6) +
            (buffer[2] & 0x3f);
            /* (0xff & 0x0f) << 12 | (0xff & 0x3f) << 6 | (0xff & 0x3f) = 0xffff,
             so ch <= 0xffff */
            if(ch < 0x0800) {
                return false;
            }

            /* surrogates (U+D800-U+DFFF) are invalid in UTF-8:
             test if (0xD800 <= ch && ch <= 0xDFFF) */
            if((ch >> 11) == 0x1b) {
                return false;
            }
        } else if(code_length == 4) {
            /* 4 bytes sequence: U+10000..U+10FFFF */
            ch = ((buffer[0] & 0x07) << 18) + ((buffer[1] & 0x3f) << 12) +
            ((buffer[2] & 0x3f) << 6) + (buffer[3] & 0x3f);

            if((ch < 0x10000) || (0x10FFFF < ch)) {
                return false;
            }
        }

        buffer += code_length;
    }

    return true;
}

/* Return true if the string is encoded in ASCII */
MJB_EXPORT bool mjb_string_is_ascii(const char* buffer, size_t size) {
    const char* end = buffer + size;

    if(buffer == 0 || size == 0) {
        return false;
    }

    for(; buffer != end; ++buffer) {
        /* every character must have leading bit at zero */
        if(*buffer & 0x80) {
            return false;
        }
    }

    return 1;
}

/* Return the codepoint character */
MJB_EXPORT bool mjb_codepoint_character(mjb_character* character, mjb_codepoint codepoint) {
    if(character == NULL || !mjb_codepoint_is_valid(codepoint) || !mjb_ready()) {
        return false;
    }

    /* Reset character */
    *character = empty_character;

    int ret = sqlite3_bind_int(mjb_internal.char_stmt, 1, codepoint);
    DB_CHECK(ret, false)

    ret = sqlite3_step(mjb_internal.char_stmt);
    bool found = ret == SQLITE_ROW;

    if(found) {
        DB_COLUMN_INT(mjb_internal.char_stmt, character->codepoint, 0);
        DB_COLUMN_TEXT(mjb_internal.char_stmt, character->name, 1)
        DB_COLUMN_INT(mjb_internal.char_stmt, character->block, 2);
        DB_COLUMN_INT(mjb_internal.char_stmt, character->category, 3);
        DB_COLUMN_INT(mjb_internal.char_stmt, character->combining, 4);
        DB_COLUMN_INT(mjb_internal.char_stmt, character->bidirectional, 5);
        DB_COLUMN_INT(mjb_internal.char_stmt, character->decomposition, 6);
        DB_COLUMN_TEXT(mjb_internal.char_stmt, character->decimal, 7)
        DB_COLUMN_TEXT(mjb_internal.char_stmt, character->digit, 8)
        DB_COLUMN_TEXT(mjb_internal.char_stmt, character->numeric, 9)
        DB_COLUMN_INT(mjb_internal.char_stmt, character->mirrored, 10);
        DB_COLUMN_INT(mjb_internal.char_stmt, character->uppercase, 11);
        DB_COLUMN_INT(mjb_internal.char_stmt, character->lowercase, 12);
        DB_COLUMN_INT(mjb_internal.char_stmt, character->titlecase, 13);
    }

    ret = sqlite3_clear_bindings(mjb_internal.char_stmt);
    DB_CHECK(ret, false)

    ret = sqlite3_reset(mjb_internal.char_stmt);
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

/* Normalize a string */
MJB_EXPORT void mjb_normalize(const char* buffer, size_t size, mjb_normalization form) {

}
