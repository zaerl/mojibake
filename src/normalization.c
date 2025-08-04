/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <string.h>
#include <stdio.h>
#include "mojibake.h"

extern struct mojibake mjb_global;

char *mjb_output_string(char *ret, char *buffer_utf8, size_t utf8_size, size_t *output_index, size_t *output_size) {
    if(!utf8_size) {
        return NULL;
    }

    if(*output_index + utf8_size > *output_size) {
        *output_size *= 2;
        ret = mjb_realloc(ret, *output_size);
    }

    memcpy((char*)ret + *output_index, buffer_utf8, utf8_size);
    *output_index += utf8_size;;

    return ret;
}

static inline char *mjb_flush_buffer(mjb_character *characters_buffer, unsigned int buffer_index,
    char *ret, size_t *output_index, size_t *output_size, mjb_normalization form) {
    // Sort combining characters by Canonical Combining Class (required for all forms)
    if(buffer_index) {
        mjb_sort(characters_buffer, buffer_index);
    }

    char buffer_utf8[5];
    size_t utf8_size = 0;
    bool do_recomposition = form == MJB_NORMALIZATION_NFC || form == MJB_NORMALIZATION_NFKC;

    if(do_recomposition && buffer_index > 1) {
        sqlite3_stmt *stmt = mjb_global.stmt_compose;
        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);

        for(size_t i = 1; i < buffer_index; ++i) {
            int rc = sqlite3_bind_int(stmt, 1, characters_buffer[0].codepoint);
            rc = sqlite3_bind_int(stmt, 2, characters_buffer[i].codepoint);

            if(rc != SQLITE_OK) {
                return ret;
            }

            while((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
                mjb_codepoint composed = (mjb_codepoint)sqlite3_column_int(stmt, 0);

                if(composed == MJB_CODEPOINT_NOT_VALID) {
                    continue;
                }

                characters_buffer[0].codepoint = composed;
                characters_buffer[i].codepoint = MJB_CODEPOINT_NOT_VALID;
            }

            sqlite3_reset(stmt);
        }

        // Apply Hangul composition after general Unicode composition
        // First check if there are any Hangul Jamo characters or Hangul syllables in the buffer
        bool has_hangul_jamo = false;
        bool has_hangul_syllable = false;

        for(size_t i = 0; i < buffer_index; ++i) {
            if(characters_buffer[i].codepoint != MJB_CODEPOINT_NOT_VALID) {
                if(mjb_codepoint_is_hangul_jamo(characters_buffer[i].codepoint)) {
                    has_hangul_jamo = true;
                }

                if(mjb_codepoint_is_hangul_syllable(characters_buffer[i].codepoint)) {
                    has_hangul_syllable = true;
                }
            }
        }

        bool has_hangul_content = has_hangul_jamo || has_hangul_syllable;

        if(has_hangul_content) {
            // Apply Hangul composition to the entire buffer
            // This handles all cases: L+V+T sequences, L+V sequences, and S+T sequences
            size_t result_len = mjb_hangul_syllable_composition(characters_buffer, buffer_index);

            // Update buffer_index to reflect the new length
            buffer_index = result_len;
        }
    }

    // Write combining characters.
    for(size_t i = 0; i < buffer_index; ++i) {
        if(characters_buffer[i].codepoint == MJB_CODEPOINT_NOT_VALID) {
            continue;
        }

        utf8_size = mjb_codepoint_encode(characters_buffer[i].codepoint, (char*)buffer_utf8, 5, MJB_ENCODING_UTF_8);
        ret = mjb_output_string(ret, buffer_utf8, utf8_size, output_index, output_size);
    }

    return ret;
}

/**
 * Normalize a string
 */
MJB_EXPORT char *mjb_normalize(const char *buffer, size_t size, size_t *output_size, mjb_encoding encoding, mjb_normalization form) {
    if(!mjb_initialize()) {
        return NULL;
    }

    if(encoding != MJB_ENCODING_UTF_8) {
        return NULL;
    }

    if(size == 0) {
        *output_size = 0;

        return NULL;
    }

    sqlite3_stmt *stmt;

    if(form == MJB_NORMALIZATION_NFC || form == MJB_NORMALIZATION_NFD) {
        // SELECT value FROM decompositions WHERE id = ?
        stmt = mjb_global.stmt_decompose;
    } else {
        // SELECT value FROM compatibility_decompositions WHERE id = ?
        stmt = mjb_global.stmt_compatibility_decompose;
    }

    sqlite3_reset(stmt);
    sqlite3_clear_bindings(stmt);

    uint8_t state = MJB_UTF8_ACCEPT;
    mjb_codepoint current_codepoint;
    mjb_character current_character;

    // Combining characters buffer.
    // TODO: set a limit and check it.
    mjb_character characters_buffer[32];
    unsigned int buffer_index = 0;

    // Return string.
    char *ret = mjb_alloc(size);
    *output_size = size;
    size_t output_index = 0;

    // String buffer.
    const char *index = buffer;

    // Loop through the string.
    for(; *index; ++index) {
        // Find next codepoint.
        state = mjb_utf8_decode_step(state, *index, &current_codepoint);

        if(state == MJB_UTF8_REJECT) {
            // Do nothing. The string is not well-formed.
            continue;
        }

        // Still not found a UTF-8 character, continue.
        if(state != MJB_UTF8_ACCEPT) {
            continue;
        }

        // Get current character.
        if(!mjb_codepoint_character(&current_character, current_codepoint)) {
            continue;
        }

        int characters_decomposed = 0;  // Count of characters produced by decomposition
        bool should_decompose = false;

        /*
         * Determine whether this character should be decomposed based on the normalization form.
         * The should_decompose flag controls whether we attempt to decompose the character
         * or just pass it through unchanged.
         */
        switch(form) {
            case MJB_NORMALIZATION_NFD:  // Canonical decomposition without recomposition
            case MJB_NORMALIZATION_NFC:  // Canonical decomposition followed by canonical composition
                should_decompose = current_character.decomposition == MJB_DECOMPOSITION_CANONICAL;
                break;

            case MJB_NORMALIZATION_NFKD: // Compatibility decomposition without recomposition
            case MJB_NORMALIZATION_NFKC: // Compatibility decomposition followed by canonical composition
                should_decompose = true;
                break;

            default:
                /*
                 * Invalid normalization form specified.
                 * Valid forms are: NFC, NFD, NFKC, NFKD
                 */
                return NULL;
        }

        // Hangul syllables have a special decomposition.
        // Only decompose in NFD/NFKD forms, not in NFC/NFKC forms
        if(mjb_codepoint_is_hangul_syllable(current_codepoint) &&
           (form == MJB_NORMALIZATION_NFD || form == MJB_NORMALIZATION_NFKD)) {
            mjb_codepoint codepoints[3];
            mjb_hangul_syllable_decomposition(current_codepoint, codepoints);

            for(size_t i = 0; i < 3; ++i) {
                if(codepoints[i] == 0) {
                    continue;
                }

                if(!mjb_codepoint_character(&current_character, codepoints[i])) {
                    continue;
                }

                // Starter: Any code point (assigned or not) with combining class of zero (ccc = 0)
                if(buffer_index && current_character.combining == MJB_CCC_NOT_REORDERED) {
                    ret = mjb_flush_buffer(characters_buffer, buffer_index, ret, &output_index, output_size, form);
                    buffer_index = 0;
                }

                characters_buffer[buffer_index++] = current_character;
                ++characters_decomposed;
            }
        } else if(should_decompose) {
            // Decompose the character using database lookup
            int rc = sqlite3_bind_int(stmt, 1, current_codepoint);

            if(rc != SQLITE_OK) {
                return NULL;
            }

            while((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
                mjb_codepoint decomposed = (mjb_codepoint)sqlite3_column_int(stmt, 0);

                if(decomposed == MJB_CODEPOINT_NOT_VALID) {
                    continue;
                }

                if(!mjb_codepoint_character(&current_character, decomposed)) {
                    continue;
                }

                ++characters_decomposed;

                /*
                 * When we encounter a "starter" character (CCC = 0), we must flush any pending
                 * combining characters in the buffer to ensure proper ordering.
                 *
                 * Examples:
                 * - 'é' (U+00E9) → 'e' (CCC=0, starter) + '́' (acute, CCC=230)
                 * - 'ñ' (U+00F1) → 'n' (CCC=0, starter) + '̃' (tilde, CCC=230)
                 *
                 * When processing "éñ", we encounter 'n' (starter) while '́' is still
                 * in the buffer, so we flush 'e' + '́' before processing 'n' + '̃'.
                 */
                if(buffer_index && current_character.combining == MJB_CCC_NOT_REORDERED) {
                    ret = mjb_flush_buffer(characters_buffer, buffer_index, ret, &output_index, output_size, form);
                    buffer_index = 0;
                }

                characters_buffer[buffer_index++] = current_character;
            }

            sqlite3_reset(stmt);
        }

        if(!characters_decomposed) {
            // Special handling for Hangul composition
            if(form == MJB_NORMALIZATION_NFC || form == MJB_NORMALIZATION_NFKC) {
                // Check if we have a Hangul syllable followed by a trailing consonant
                if(buffer_index > 0 &&
                   mjb_codepoint_is_hangul_syllable(characters_buffer[buffer_index - 1].codepoint) &&
                   mjb_codepoint_is_hangul_t(current_codepoint)) {
                    // Check if the syllable can accept a trailing consonant
                    mjb_codepoint syllable = characters_buffer[buffer_index - 1].codepoint;
                    int s_index = syllable - MJB_CP_HANGUL_S_BASE;
                    if(s_index >= 0 && s_index < MJB_CP_HANGUL_S_COUNT && (s_index % MJB_CP_HANGUL_T_COUNT) == 0) {
                        // The syllable has no trailing consonant, so we can add one
                        mjb_codepoint trailing = current_codepoint;
                        mjb_codepoint composed = syllable + (trailing - MJB_CP_HANGUL_T_BASE);
                        characters_buffer[buffer_index - 1].codepoint = composed;
                        // Don't add the trailing consonant since it's been composed
                        continue;
                    }
                }
            }

            if(buffer_index && current_character.combining == MJB_CCC_NOT_REORDERED) {
                ret = mjb_flush_buffer(characters_buffer, buffer_index, ret, &output_index, output_size, form);
                buffer_index = 0;
            }

            characters_buffer[buffer_index++] = current_character;
       }
    }

    // We have combining characters in the buffer, we must output them.
    if(buffer_index) {
        ret = mjb_flush_buffer(characters_buffer, buffer_index, ret, &output_index, output_size, form);
        buffer_index = 0;
    }

    // Guarantee null-terminated string
    if(output_index >= *output_size) {
        ret = mjb_realloc(ret, *output_size + 1);
    }

    ret[output_index] = '\0';
    *output_size = output_index;

    return ret;
}

/**
 * Return the next character from the string.
 */
MJB_EXPORT bool mjb_next_character(const char *buffer, size_t size, mjb_encoding encoding, mjb_next_character_fn fn) {
    if(!mjb_initialize()) {
        return false;
    }

    if(encoding != MJB_ENCODING_UTF_8) {
        return false;
    }

    if(size == 0) {
        return false;
    }

    uint8_t state = MJB_UTF8_ACCEPT;
    mjb_codepoint codepoint;
    mjb_character character = {0};
    bool has_previous_character = false;
    bool first_character = true;

    // String buffer.
    const char *index = buffer;

    // Loop through the string.
    for(; *index; ++index) {
        // Find next codepoint.
        state = mjb_utf8_decode_step(state, *index, &codepoint);

        if(state == MJB_UTF8_REJECT) {
            // Do nothing. The string is not well-formed.
            return false;
        }

        // Still not found a UTF-8 character, continue.
        if(state != MJB_UTF8_ACCEPT) {
            continue;
        }

        if(has_previous_character) {
            // Call the callback function.
            if(!fn(&character, first_character ? MJB_NEXT_CHAR_FIRST : MJB_NEXT_CHAR_NONE)) {
                return false;
            }

            has_previous_character = false;
            first_character = false;
        }

        // Get current character.
        if(!mjb_codepoint_character(&character, codepoint)) {
            continue;
        }

        has_previous_character = true;
    }

    if(has_previous_character) {
        // Call the callback function.
        if(!fn(&character, first_character ? MJB_NEXT_CHAR_FIRST | MJB_NEXT_CHAR_LAST : MJB_NEXT_CHAR_LAST)) {
            return false;
        }
    }

    return true;
}
