/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "mojibake-internal.h"
#include "utf.h"

extern mojibake mjb_global;

static bool mjb_is_cased(mjb_category category) {
    return category == MJB_CATEGORY_LU || category == MJB_CATEGORY_LL ||
        category == MJB_CATEGORY_LT;
}

// Mn, Me and Cf do not break words.
static bool mjb_is_case_ignorable(mjb_category category) {
    return category == MJB_CATEGORY_MN || category == MJB_CATEGORY_ME ||
        category == MJB_CATEGORY_CF;
}

static bool mjb_maybe_has_special_casing(mjb_codepoint codepoint) {
    // Codepoints with unconditional multi-char entries in SpecialCasing.txt.
    switch(codepoint) {
        case 223:  // U+00DF ß LATIN SMALL LETTER SHARP S
        case 304:  // U+0130 İ LATIN CAPITAL LETTER I WITH DOT ABOVE
        case 329:  // U+0149 ŉ LATIN SMALL LETTER N PRECEDED BY APOSTROPHE
        case 496:  // U+01F0 ǰ LATIN SMALL LETTER J WITH CARON
        case 912:  // U+0390 ΐ GREEK SMALL LETTER IOTA WITH DIALYTIKA AND TONOS
        case 944:  // U+03B0 ΰ GREEK SMALL LETTER UPSILON WITH DIALYTIKA AND TONOS
        case 1415: // U+0587 եւ ARMENIAN SMALL LIGATURE ECH YIWN
            return true;
    }

    return
        (codepoint >= 7830 && codepoint <= 7834) || // U+1E96–U+1E9A
        (codepoint >= 8016 && codepoint <= 8188) || // U+1F50–U+1FFC
        (codepoint >= 64256 && codepoint <= 64279); // U+FB00–U+FB17
}

static char *mjb_special_casing_codepoint(mjb_codepoint codepoint, char *output,
    size_t *output_index, size_t *output_size, mjb_case_type type) {
    sqlite3_stmt *stmt_special_casing = mjb_global.stmt_special_casing;

    sqlite3_reset(stmt_special_casing);

    if(sqlite3_bind_int(stmt_special_casing, 1, codepoint) != SQLITE_OK) {
        return NULL;
    }

    if(sqlite3_bind_int(stmt_special_casing, 2, type) != SQLITE_OK) {
        return NULL;
    }

    unsigned int found = 0;

    while(sqlite3_step(stmt_special_casing) == SQLITE_ROW) {
        for(int i = 0; i < 3; ++i) {
            if(sqlite3_column_type(stmt_special_casing, i) != SQLITE_NULL) {
                mjb_codepoint new_cp = (mjb_codepoint)sqlite3_column_int(stmt_special_casing,i);

                char *new_output = mjb_string_output_codepoint(new_cp, output, output_index,
                    output_size, MJB_ENCODING_UTF_8);

                if(new_output != NULL) {
                    output = new_output;
                }
            } else {
                break;
            }
        }

        ++found;
    }

    return found > 0 ? output : NULL;
}

// Peek at the next decoded codepoint and return whether it is cased (Lu/Ll/Lt).
static bool mjb_next_is_cased(const char *buffer, size_t size, size_t i,
    uint8_t state, mjb_encoding encoding, sqlite3_stmt *stmt) {
    mjb_codepoint next_cp = 0;
    bool next_in_error = false;
    mjb_decode_result ps;

    do {
        ps = mjb_next_codepoint(buffer, size, &state, &i, encoding, &next_cp, &next_in_error);
    } while(ps == MJB_DECODE_INCOMPLETE);

    if(ps == MJB_DECODE_END) {
        return false;
    }

    sqlite3_reset(stmt);

    if(sqlite3_bind_int(stmt, 1, next_cp) != SQLITE_OK || sqlite3_step(stmt) != SQLITE_ROW) {
        return false;
    }

    return mjb_is_cased((mjb_category)sqlite3_column_int(stmt, 0));
}

/**
 * See: https://www.unicode.org/versions/Unicode17.0.0/core-spec/chapter-3/#G34078
 */
static char *mjb_titlecase(const char *buffer, size_t size, mjb_encoding encoding) {
    uint8_t state = MJB_UTF_ACCEPT;
    bool in_error = false;
    mjb_codepoint codepoint;
    sqlite3_stmt *stmt = mjb_global.stmt_case;
    char *output = (char*)mjb_alloc(size);

    // char *output = mjb_alloc(length);
    size_t output_index = 0;
    size_t output_size = size;
    bool in_word = false;

    for(size_t i = 0; i < size;) {
        // Find next codepoint.
        mjb_decode_result decode_status = mjb_next_codepoint(buffer, size, &state, &i, encoding,
            &codepoint, &in_error);

        if(decode_status == MJB_DECODE_END) {
            break;
        }

        if(decode_status == MJB_DECODE_INCOMPLETE) {
            continue;
        }

        sqlite3_reset(stmt);
        // sqlite3_clear_bindings(stmt);

        if(sqlite3_bind_int(stmt, 1, codepoint) != SQLITE_OK) {
            mjb_free(output);

            return NULL;
        }

        if(sqlite3_step(stmt) != SQLITE_ROW) {
            mjb_free(output);

            return NULL;
        }

        mjb_codepoint original = codepoint;
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

        // Final_Sigma (SpecialCasing.txt condition Final_Sigma):
        // In-word Σ (U+03A3) → ς (U+03C2) when not followed by a cased letter.
        if(original == 0x03A3 && case_type == MJB_CASE_LOWER) {
            mjb_codepoint sigma_out = mjb_next_is_cased(buffer, size, i, state, encoding, stmt)
                ? 0x03C3   // σ: followed by a cased letter, not word-final
                : 0x03C2;  // ς: word-final sigma

            output = mjb_string_output_codepoint(sigma_out, output, &output_index,
                &output_size, encoding);

            continue;
        }

        // Check against the original codepoint (before any remapping by the word-boundary
        // block above). The remapped form may not be the source of any special casing rule.
        if(mjb_maybe_has_special_casing(original)) {
            char *new_output = mjb_special_casing_codepoint(original, output,
                &output_index, &output_size, case_type == MJB_CASE_NONE ? MJB_CASE_TITLE :
                case_type);

            if(new_output != NULL) {
                output = new_output;

                continue;
            }
        }

        output = mjb_string_output_codepoint(codepoint, output, &output_index,
            &output_size, encoding);
    }

    if(output_index >= output_size) {
        output = (char*)mjb_realloc(output, output_size + 1);
    }

    output[output_index] = '\0';

    return output;
}

MJB_EXPORT char *mjb_case(const char *buffer, size_t size, mjb_case_type type,
    mjb_encoding encoding) {
    if(size == 0) {
        return (char*)buffer;
    }

    if(!mjb_initialize()) {
        return NULL;
    }

    if(type == MJB_CASE_TITLE) {
        return mjb_titlecase(buffer, size, encoding);
    }

    uint8_t state = MJB_UTF_ACCEPT;
    bool in_error = false;
    mjb_codepoint codepoint;
    sqlite3_stmt *stmt = mjb_global.stmt_case;
    char *output = (char*)mjb_alloc(size);

    size_t output_index = 0;
    size_t output_size = size;
    bool in_word = false; // tracks word context for Final_Sigma (MJB_CASE_LOWER only)

    for(size_t i = 0; i < size;) {
        // Find next codepoint.
        mjb_decode_result decode_status = mjb_next_codepoint(buffer, size, &state, &i, encoding,
            &codepoint, &in_error);

        if(decode_status == MJB_DECODE_END) {
            break;
        }

        if(decode_status == MJB_DECODE_INCOMPLETE) {
            continue;
        }

        if(type == MJB_CASE_CASEFOLD) {
            sqlite3_stmt *scf = mjb_global.stmt_special_casing;

            sqlite3_reset(scf);

            if(sqlite3_bind_int(scf, 1, codepoint) == SQLITE_OK &&
                sqlite3_bind_int(scf, 2, MJB_CASE_CASEFOLD) == SQLITE_OK &&
                sqlite3_step(scf) == SQLITE_ROW) {
                // Emit up to 3 mapped codepoints (F entries have 2-3, C exceptions have 1)
                for(int k = 0; k < 3; ++k) {
                    if(sqlite3_column_type(scf, k) == SQLITE_NULL) {
                        break;
                    }

                    mjb_codepoint mapped = (mjb_codepoint)sqlite3_column_int(scf, k);
                    output = mjb_string_output_codepoint(mapped, output, &output_index,
                        &output_size, encoding);
                }

                continue;
            }

            // Fall back to unicode_data lowercase
            sqlite3_reset(stmt);

            if(
                sqlite3_bind_int(stmt, 1, codepoint) == SQLITE_OK &&
                sqlite3_step(stmt) == SQLITE_ROW &&
                sqlite3_column_type(stmt, MJB_CASE_LOWER) != SQLITE_NULL
            ) {
                codepoint = (mjb_codepoint)sqlite3_column_int(stmt, MJB_CASE_LOWER);
            }

            // Identity: codepoint unchanged if no lowercase found
            output = mjb_string_output_codepoint(codepoint, output, &output_index,
                &output_size, encoding);

            continue;
        }

        // Final_Sigma (SpecialCasing.txt condition Final_Sigma):
        // Σ (U+03A3) → ς (U+03C2) when preceded by a cased letter in the same word
        // and not followed by a cased letter.
        if(type == MJB_CASE_LOWER && codepoint == 0x03A3) {
            mjb_codepoint sigma_out;

            if(in_word && !mjb_next_is_cased(buffer, size, i, state, encoding, stmt)) {
                sigma_out = 0x03C2; // ς: word-final sigma
            } else {
                sigma_out = 0x03C3; // σ: non-final or no preceding cased letter
            }

            in_word = true;  // Σ is a cased letter

            output = mjb_string_output_codepoint(sigma_out, output, &output_index,
                &output_size, encoding);

            continue;
        }

        if(mjb_maybe_has_special_casing(codepoint)) {
            char *new_output = mjb_special_casing_codepoint(codepoint, output,
                &output_index, &output_size, type);

            if(new_output != NULL) {
                if(type == MJB_CASE_LOWER) {
                    in_word = true;
                }

                output = new_output;

                continue;
            }
        }

        sqlite3_reset(stmt);

        int rc = sqlite3_bind_int(stmt, 1, codepoint);

        if(rc != SQLITE_OK) {
            mjb_free(output);

            return NULL;
        }

        rc = sqlite3_step(stmt);

        if(rc != SQLITE_ROW) {
            mjb_free(output);

            return NULL;
        }

        if(type == MJB_CASE_LOWER) {
            mjb_category cat = (mjb_category)sqlite3_column_int(stmt, 0);

            if(mjb_is_cased(cat)) {
                in_word = true;
            } else if(!mjb_is_case_ignorable(cat)) {
                in_word = false;
            }
        }

        if(sqlite3_column_type(stmt, type) == SQLITE_NULL) {
            // Skip.
        } else {
            codepoint = (mjb_codepoint)sqlite3_column_int(stmt, type);
        }

        output = mjb_string_output_codepoint(codepoint, output, &output_index,
            &output_size, encoding);
    }

    if(output_index >= output_size) {
        output = (char*)mjb_realloc(output, output_size + 1);
    }

    output[output_index] = '\0';

    return output;
}

// Return the codepoint lowercase codepoint
MJB_EXPORT mjb_codepoint mjb_codepoint_to_lowercase(mjb_codepoint codepoint) {
    mjb_character character;

    if(!mjb_codepoint_character(codepoint, &character)) {
        return codepoint;
    }

    return character.lowercase == 0 ? codepoint : character.lowercase;
}

// Return the codepoint uppercase codepoint
MJB_EXPORT mjb_codepoint mjb_codepoint_to_uppercase(mjb_codepoint codepoint) {
    mjb_character character;

    if(!mjb_codepoint_character(codepoint, &character)) {
        return codepoint;
    }

    return character.uppercase == 0 ? codepoint : character.uppercase;
}

// Return the codepoint titlecase codepoint
MJB_EXPORT mjb_codepoint mjb_codepoint_to_titlecase(mjb_codepoint codepoint) {
    mjb_character character;

    if(!mjb_codepoint_character(codepoint, &character)) {
        return codepoint;
    }

    return character.titlecase == 0 ? codepoint : character.titlecase;
}
