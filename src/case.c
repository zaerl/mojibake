/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "mojibake-internal.h"
#include "utf8.h"
#include "utf16.h"

extern mojibake mjb_global;

static bool mjb_maybe_has_special_casing(mjb_codepoint codepoint) {
    // This values are manually picked from the SpecialCasing.txt file during the generation.

    switch(codepoint) {
        case 73:
        case 74:
        case 105:
        case 204:
        case 205:
        case 223:
        case 296:
        case 302:
        case 304:
        case 329:
        case 496:
        case 775:
        case 912:
        case 931:
        case 944:
        case 1415:
            return true;
    }

    return
        (codepoint >= 7830 && codepoint <= 7834) ||
        (codepoint >= 8016 && codepoint <= 8188) ||
        (codepoint >= 64256 && codepoint <= 64279);
}

static unsigned int mjb_special_casing_codepoint(mjb_codepoint codepoint, char *output,
    size_t *output_index, size_t *output_size, mjb_case_type type) {
    sqlite3_stmt *stmt_special_casing = mjb_global.stmt_special_casing;

    // Potential query:
    // SELECT new_case_1, new_case_2, new_case_3 FROM special_casing WHERE id = ? AND case_type = ?
    // UNION ALL
    // SELECT uppercase, lowercase, titlecase FROM characters WHERE id = ?
    // AND NOT EXISTS (SELECT 1 FROM special_casing WHERE id = ? AND case_type = ?);

    sqlite3_reset(stmt_special_casing);
    // sqlite3_clear_bindings(stmt_special_casing);

    if(sqlite3_bind_int(stmt_special_casing, 1, codepoint) != SQLITE_OK) {
        return 0;
    }

    if(sqlite3_bind_int(stmt_special_casing, 2, type) != SQLITE_OK) {
        return 0;
    }

    unsigned int found = 0;

    while(sqlite3_step(stmt_special_casing) == SQLITE_ROW) {
        for(int i = 0; i < 3; ++i) {
            if(sqlite3_column_type(stmt_special_casing, i) != SQLITE_NULL) {
                mjb_codepoint new_cp = (mjb_codepoint)sqlite3_column_int(stmt_special_casing,i);
                output = mjb_string_output_codepoint(new_cp, output, output_index, output_size);
            } else {
                break;
            }
        }

        ++found;
    }

    return found;
}

/**
 * See: https://www.unicode.org/versions/Unicode16.0.0/core-spec/chapter-3/#G34078
 */
static char *mjb_titlecase(const char *buffer, size_t size, mjb_encoding encoding) {
    uint8_t state = MJB_UTF_ACCEPT;
    mjb_codepoint codepoint;
    sqlite3_stmt *stmt = mjb_global.stmt_case;
    char *output = (char*)mjb_alloc(size);

    // char *output = mjb_alloc(length);
    size_t output_index = 0;
    size_t output_size = size;
    bool in_word = false;

    for(size_t i = 0; i < size && buffer[i]; ++i) {
        // Find next codepoint.
        if(encoding == MJB_ENCODING_UTF_8) {
            state = mjb_utf8_decode_step(state, buffer[i], &codepoint);
        } else {
            state = mjb_utf16_decode_step(state, buffer[i], buffer[i + 1], &codepoint,
                encoding == MJB_ENCODING_UTF_16_BE);
            ++i;
        }

        if(state == MJB_UTF_REJECT) {
            continue;
        }

        if(state != MJB_UTF_ACCEPT) {
            continue;
        }

        sqlite3_reset(stmt);
        // sqlite3_clear_bindings(stmt);

        if(sqlite3_bind_int(stmt, 1, codepoint) != SQLITE_OK) {
            return NULL;
        }

        if(sqlite3_step(stmt) != SQLITE_ROW) {
            return NULL;
        }

        mjb_category category = (mjb_category)sqlite3_column_int(stmt, 0);
        mjb_case_type case_type = MJB_CASE_NONE;

        // Word boundary.
        if(
            // Is cased.
            category == MJB_CATEGORY_LU || category == MJB_CATEGORY_LL ||
            category == MJB_CATEGORY_LT ||
            // Is modifier.
            category == MJB_CATEGORY_LM ||
            // Is number.
            category == MJB_CATEGORY_LO || category == MJB_CATEGORY_NL) {

            if(!in_word) {
                // Try titlecase first.
                if(sqlite3_column_type(stmt, 3) == SQLITE_NULL) {
                    // If titlecase is not available, try uppercase.
                    if(sqlite3_column_type(stmt, 1) != SQLITE_NULL) {
                        codepoint = (mjb_codepoint)sqlite3_column_int(stmt, 1);
                        case_type = MJB_CASE_UPPER;
                    }
                } else {
                    codepoint = (mjb_codepoint)sqlite3_column_int(stmt, 3);
                    case_type = MJB_CASE_TITLE;
                }

                in_word = true;
            } else {
                // Try lowercase.
                if(sqlite3_column_type(stmt, 2) != SQLITE_NULL) {
                    codepoint = (mjb_codepoint)sqlite3_column_int(stmt, 2);
                    case_type = MJB_CASE_LOWER;
                }
            }
        } else {
            in_word = false;
        }

        if(mjb_maybe_has_special_casing(codepoint)) {
            unsigned int found = mjb_special_casing_codepoint(codepoint, output,
                &output_index, &output_size, case_type == MJB_CASE_NONE ? MJB_CASE_TITLE :
                case_type);

            if(found) {
                continue;
            }
        }

        output = mjb_string_output_codepoint(codepoint, output, &output_index,
            &output_size);
    }

    if(output_index >= output_size) {
        output = (char*)mjb_realloc(output, output_size + 1);
    }

    output[output_index] = '\0';

    return output;
}

MJB_EXPORT char *mjb_case(const char *buffer, size_t length, mjb_case_type type,
    mjb_encoding encoding) {
    if(length == 0) {
        return (char*)buffer;
    }

    if(!mjb_initialize()) {
        return NULL;
    }

    if(type == MJB_CASE_TITLE) {
        return mjb_titlecase(buffer, length, encoding);
    }

    uint8_t state = MJB_UTF_ACCEPT;
    mjb_codepoint codepoint;
    sqlite3_stmt *stmt = mjb_global.stmt_case;
    char *output = (char*)mjb_alloc(length);

    size_t output_index = 0;
    size_t output_size = length;

    for(size_t i = 0; i < length && buffer[i]; ++i) {
        // Find next codepoint.
        if(encoding == MJB_ENCODING_UTF_8) {
            state = mjb_utf8_decode_step(state, buffer[i], &codepoint);
        } else {
            state = mjb_utf16_decode_step(state, buffer[i], buffer[i + 1], &codepoint,
                encoding == MJB_ENCODING_UTF_16_BE);
            ++i;
        }

        if(state == MJB_UTF_REJECT) {
            continue;
        }

        if(state != MJB_UTF_ACCEPT) {
            continue;
        }

        if(type == MJB_CASE_CASEFOLD) {
            output = mjb_string_output_codepoint(codepoint, output, &output_index,
                &output_size);

            continue;
        }

        if(mjb_maybe_has_special_casing(codepoint)) {
            unsigned int found = mjb_special_casing_codepoint(codepoint, output,
                &output_index, &output_size, type);

            if(found) {
                continue;
            }
        }

        sqlite3_reset(stmt);
        // sqlite3_clear_bindings(stmt);

        int rc = sqlite3_bind_int(stmt, 1, codepoint);

        if(rc != SQLITE_OK) {
            return NULL;
        }

        rc = sqlite3_step(stmt);

        if(rc != SQLITE_ROW) {
            return NULL;
        }

        if(sqlite3_column_type(stmt, type) == SQLITE_NULL) {
            // Skip.
        } else {
            codepoint = (mjb_codepoint)sqlite3_column_int(stmt, type);
        }

        output = mjb_string_output_codepoint(codepoint, output, &output_index,
            &output_size);
    }

    if(output_index >= output_size) {
        output = (char*)mjb_realloc(output, output_size + 1);
    }

    output[output_index] = '\0';

    return output;
}
