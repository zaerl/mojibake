/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <string.h>
#include <stdio.h>
#include "mojibake.h"

extern struct mojibake mjb_global;

typedef struct {
    uint32_t codepoint;
    uint16_t combining;
} mjb_buffer_character;

// Normalization sort.
static void mjb_normalization_sort(mjb_normalization_character array[], size_t size) {
    for(size_t step = 1; step < size; ++step) {
        mjb_normalization_character key = array[step];
        int j = step - 1;

        // Move elements of array[0..step-1], that are greater than key,
        // to one position ahead of their current position
        while(j >= 0 && array[j].combining > key.combining) {
            array[j + 1] = array[j];
            j = j - 1;
        }

        array[j + 1] = key;
    }
}

/**
 * A smaller version of mjb_normalize() that only returns the character information.
 * This is used to avoid the overhead of the full normalization process.
 */
static bool mjb_get_buffer_character(mjb_normalization_character *character, mjb_codepoint codepoint) {
    if(!mjb_codepoint_is_valid(codepoint)) {
        return false;
    }

    if(mjb_codepoint_is_hangul_syllable(codepoint) || mjb_codepoint_is_cjk_ideograph(codepoint)) {
        character->codepoint = codepoint;
        character->combining = MJB_CCC_NOT_REORDERED;
        character->decomposition = MJB_DECOMPOSITION_NONE;

        return true;
    }

    sqlite3_reset(mjb_global.stmt_buffer_character);
    sqlite3_clear_bindings(mjb_global.stmt_buffer_character);

    int rc = sqlite3_bind_int(mjb_global.stmt_buffer_character, 1, codepoint);

    if(rc != SQLITE_OK) {
        return false;
    }

    rc = sqlite3_step(mjb_global.stmt_buffer_character);

    if(rc != SQLITE_ROW) {
        return false;
    }

    character->codepoint = codepoint;
    character->combining = (uint8_t)sqlite3_column_int(mjb_global.stmt_buffer_character, 1);
    character->decomposition = (uint8_t)sqlite3_column_int(mjb_global.stmt_buffer_character, 2);

    return true;
}

static char *mjb_output_string(char *ret, char *buffer_utf8, size_t utf8_size, size_t *output_index, size_t *output_size) {
    if(!utf8_size) {
        return NULL;
    }

    if(*output_index + utf8_size > *output_size) {
        *output_size *= 2;
        ret = mjb_realloc(ret, *output_size);
    }

    memcpy((char*)ret + *output_index, buffer_utf8, utf8_size);
    *output_index += utf8_size;

    return ret;
}

// Flush the decomposition buffer to a UTF-8 string.
static char *mjb_flush_d_buffer(mjb_normalization_character *characters_buffer, size_t buffer_index,
    char *output, size_t *output_index, size_t *output_size, mjb_normalization form) {

    // Sort combining characters by Canonical Combining Class (required for all forms)
    if(buffer_index) {
        mjb_normalization_sort(characters_buffer, buffer_index);
    } else {
        return output;
    }

    char buffer_utf8[5];
    size_t utf8_size = 0;

    // Write combining characters.
    for(size_t i = 0; i < buffer_index; ++i) {
        if(characters_buffer[i].codepoint == MJB_CODEPOINT_NOT_VALID) {
            continue;
        }

        utf8_size = mjb_codepoint_encode(characters_buffer[i].codepoint, (char*)buffer_utf8, 5, MJB_ENCODING_UTF_8);
        output = mjb_output_string(output, buffer_utf8, utf8_size, output_index, output_size);
    }

    return output;
}

// Flush the composition buffer to a bit array.
static mjb_buffer_character *mjb_flush_c_buffer(mjb_normalization_character *characters_buffer, size_t buffer_index,
    mjb_buffer_character *output, size_t *output_index, size_t *output_size, mjb_normalization form) {

    // Sort combining characters by Canonical Combining Class (required for all forms)
    if(buffer_index) {
        mjb_normalization_sort(characters_buffer, buffer_index);
    } else {
        return output;
    }

    for(size_t i = 0; i < buffer_index; ++i) {
        if(characters_buffer[i].codepoint == MJB_CODEPOINT_NOT_VALID) {
            continue;
        }

        // Check if we need to reallocate the output buffer
        if(*output_index >= *output_size) {
            *output_size *= 2;
            output = mjb_realloc(output, *output_size * sizeof(mjb_buffer_character));
        }

        output[*output_index].codepoint = characters_buffer[i].codepoint;
        output[*output_index].combining = characters_buffer[i].combining;

        ++(*output_index);
    }

    return output;
}

/**
 * Recompose the string.
 * Canonical Composition Algorithm
 */
static bool mjb_recompose(char **output, size_t *output_size, size_t codepoints_count,
    mjb_buffer_character *composition_buffer) {
    // Nothing to recompose. Output an empty string.
    if(codepoints_count == 0) {
        *output = mjb_alloc(1);
        *output_size = 0;
        (*output)[0] = '\0';

        return true;
    }

    // Apply Canonical Composition Algorithm
    *output_size = codepoints_count * 2; // A good starting size.
    char *composed_output = mjb_alloc(*output_size);

    size_t composed_output_index = 0;
    sqlite3_stmt *stmt = mjb_global.stmt_compose;

    char buffer_utf8[5];
    size_t utf8_size = 0;
    size_t i = 0;

    while(i < codepoints_count) {
        if(composition_buffer[i].combining != MJB_CCC_NOT_REORDERED) {
            // Non-starter: output and continue
            utf8_size = mjb_codepoint_encode(composition_buffer[i].codepoint, (char*)buffer_utf8, 5, MJB_ENCODING_UTF_8);
            composed_output = mjb_output_string(composed_output, buffer_utf8, utf8_size, &composed_output_index, output_size);

            ++i;

            continue;
        }

        // Found a starter
        mjb_codepoint starter = composition_buffer[i].codepoint;
        size_t starter_pos = i;
        uint8_t last_combining_class = 0;

        ++i; // Move to first character after starter

        // Process characters following this starter (try composition with next character first, then combining chars)
        while(i < codepoints_count) {
            // If we encounter another starter after processing combining chars, stop
            if(composition_buffer[i].combining == MJB_CCC_NOT_REORDERED && last_combining_class != 0) {
                break;
            }

            mjb_codepoint combining_char = composition_buffer[i].codepoint;
            uint8_t current_combining_class = composition_buffer[i].combining;

            // Check if composition is blocked
            bool blocked = (last_combining_class != 0 &&
            last_combining_class >= current_combining_class);

            if(!blocked) {
                // Try to compose starter with this combining character
                sqlite3_reset(stmt);
                sqlite3_clear_bindings(stmt);

                int rc = sqlite3_bind_int(stmt, 1, starter);
                if(rc != SQLITE_OK) {
                    mjb_free(composed_output);

                    return false;
                }

                rc = sqlite3_bind_int(stmt, 2, combining_char);
                if(rc != SQLITE_OK) {
                    mjb_free(composed_output);

                    return false;
                }

                mjb_codepoint composed = MJB_CODEPOINT_NOT_VALID;

                while((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
                    composed = (mjb_codepoint)sqlite3_column_int(stmt, 0);
                }

                if(composed != MJB_CODEPOINT_NOT_VALID) {
                    // Composition successful
                    starter = composed;
                    composition_buffer[i].codepoint = MJB_CODEPOINT_NOT_VALID; // Mark as consumed
                    // Don't update last_combining_class since the character was consumed
                } else {
                    // No composition found
                    if(current_combining_class != 0) {
                        last_combining_class = current_combining_class;
                    } else {
                        // If this was a starter (CCC=0) and no composition happened,
                        // we should stop here and process it in the next iteration
                        break;
                    }
                }
            } else {
                // Composition blocked
                if(current_combining_class != 0) {
                    last_combining_class = current_combining_class;
                } else {
                    // If this was a starter (CCC=0) and no composition happened,
                    // we should stop here and process it in the next iteration
                    break;
                }
            }

            ++i;
        }

        // Output the starter (possibly composed)
        utf8_size = mjb_codepoint_encode(starter, (char*)buffer_utf8, 5, MJB_ENCODING_UTF_8);
        composed_output = mjb_output_string(composed_output, buffer_utf8, utf8_size, &composed_output_index, output_size);

        // Output any non-consumed combining characters in order
        for(size_t j = starter_pos + 1; j < i; ++j) {
            if(composition_buffer[j].codepoint != MJB_CODEPOINT_NOT_VALID) {
                utf8_size = mjb_codepoint_encode(composition_buffer[j].codepoint, (char*)buffer_utf8, 5, MJB_ENCODING_UTF_8);
                composed_output = mjb_output_string(composed_output, buffer_utf8, utf8_size, &composed_output_index, output_size);
            }
        }
    }

    if(composed_output_index >= *output_size) {
        composed_output = mjb_realloc(composed_output, composed_output_index + 1);
    }

    *output = composed_output;
    *output_size = composed_output_index;
    composed_output[composed_output_index] = '\0';

    /*
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
    }*/

    return true;
}

/**
 * Normalize a string
 */
MJB_EXPORT char *mjb_normalize(const char *buffer, size_t size, size_t *output_size,
    mjb_encoding encoding, mjb_normalization form) {
    if(!mjb_initialize() || encoding != MJB_ENCODING_UTF_8 || size == 0) {
        return NULL;
    }

    if(form != MJB_NORMALIZATION_NFD && form != MJB_NORMALIZATION_NFKD &&
       form != MJB_NORMALIZATION_NFC && form != MJB_NORMALIZATION_NFKC) {
        return NULL;
    }

    sqlite3_stmt *stmt;

    uint8_t state = MJB_UTF8_ACCEPT;
    mjb_codepoint current_codepoint;
    mjb_normalization_character current_character;
    // size_t codepoints_count = 0;

    // Combining characters buffer.
    // TODO: set a limit and check it.
    mjb_normalization_character characters_buffer[32];
    size_t buffer_index = 0;

    // We directly return the string for NFD/NFKD forms.
    char *output = NULL;
    // output_size is set to the size of the input string.

    // A composition buffer is used for NFC/NFKC forms. A string is returned later.
    mjb_buffer_character *composition_buffer = NULL;

    // This is the index of the next character to be written to the output string or the composition
    // buffer.
    size_t output_index = 0;

    bool is_composition = form == MJB_NORMALIZATION_NFC || form == MJB_NORMALIZATION_NFKC;
    bool is_compatibility = form == MJB_NORMALIZATION_NFKC || form == MJB_NORMALIZATION_NFKD;

    if(is_composition) {
        composition_buffer = mjb_alloc(size * sizeof(mjb_buffer_character));
    } else {
        output = mjb_alloc(size);
    }

    *output_size = size;

    if(is_compatibility) {
        // SELECT value FROM compatibility_decompositions WHERE id = ?
        stmt = mjb_global.stmt_compatibility_decompose;
    } else {
        // SELECT value FROM decompositions WHERE id = ?
        stmt = mjb_global.stmt_decompose;
    }

    sqlite3_reset(stmt);
    sqlite3_clear_bindings(stmt);

    // String buffer, used for UTF-8 decoding.
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
        if(!mjb_get_buffer_character(&current_character, current_codepoint)) {
            continue;
        }

        // ++codepoints_count;

        int characters_decomposed = 0;  // Count of characters produced by decomposition
        bool should_decompose = false;

        /*
         * Determine whether this character should be decomposed based on the normalization form.
         * The should_decompose flag controls whether we attempt to decompose the character
         * or just pass it through unchanged.
         */
        if(is_compatibility) {
            should_decompose = true;
        } else {
            should_decompose = current_character.decomposition == MJB_DECOMPOSITION_CANONICAL;
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

                if(!mjb_get_buffer_character(&current_character, codepoints[i])) {
                    continue;
                }

                // ++codepoints_count;

                // Starter: Any code point (assigned or not) with combining class of zero (ccc = 0)
                if(buffer_index && current_character.combining == MJB_CCC_NOT_REORDERED) {
                    if(is_composition) {
                        composition_buffer = mjb_flush_c_buffer(characters_buffer, buffer_index,
                            composition_buffer, &output_index, output_size, form);
                    } else {
                        output = mjb_flush_d_buffer(characters_buffer, buffer_index, output,
                            &output_index, output_size, form);
                    }

                    buffer_index = 0;
                }

                characters_buffer[buffer_index++] = current_character;
                ++characters_decomposed;
            }
        } else if(should_decompose) {
            // Decompose the character using database lookup
            int rc = sqlite3_bind_int(stmt, 1, current_codepoint);

            if(rc != SQLITE_OK) {
                if(composition_buffer) {
                    mjb_free(composition_buffer);
                }

                return NULL;
            }

            while((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
                mjb_codepoint decomposed = (mjb_codepoint)sqlite3_column_int(stmt, 0);

                if(decomposed == MJB_CODEPOINT_NOT_VALID) {
                    continue;
                }

                if(!mjb_get_buffer_character(&current_character, decomposed)) {
                    continue;
                }

                // ++codepoints_count;
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
                    if(is_composition) {
                        composition_buffer = mjb_flush_c_buffer(characters_buffer, buffer_index,
                           composition_buffer, &output_index, output_size, form);
                    } else {
                        output = mjb_flush_d_buffer(characters_buffer, buffer_index, output,
                            &output_index, output_size, form);
                    }
                    buffer_index = 0;
                }

                characters_buffer[buffer_index++] = current_character;
            }

            sqlite3_reset(stmt);
        }

        if(!characters_decomposed) {
            // Special handling for Hangul composition
            if(is_composition) {
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
                if(is_composition) {
                    composition_buffer = mjb_flush_c_buffer(characters_buffer, buffer_index,
                       composition_buffer, &output_index, output_size, form);
                } else {
                    output = mjb_flush_d_buffer(characters_buffer, buffer_index, output, &output_index,
                        output_size, form);
                }
                buffer_index = 0;
            }

            characters_buffer[buffer_index++] = current_character;
       }
    }

    // We have combining characters in the buffer, we must output them.
    if(buffer_index) {
        if(is_composition) {
            composition_buffer = mjb_flush_c_buffer(characters_buffer, buffer_index,
                composition_buffer, &output_index, output_size, form);
        } else {
            output = mjb_flush_d_buffer(characters_buffer, buffer_index, output, &output_index,
                output_size, form);
        }
        buffer_index = 0;
    }

    if(is_composition) {
        // Recompose the string.
        mjb_recompose(&output, output_size, output_index, composition_buffer);
        mjb_free(composition_buffer);
    } else {
        // Guarantee null-terminated string
        if(output_index >= *output_size) {
            output = mjb_realloc(output, *output_size + 1);
        }

        output[output_index] = '\0';
        *output_size = output_index;
    }

    return output;
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
