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
    if(buffer_index) {
        mjb_sort(characters_buffer, buffer_index);
    }

    char buffer_utf8[5];
    size_t utf8_size = 0;
    sqlite3_stmt *stmt = NULL;

    if(form == MJB_NORMALIZATION_NFC) {
        stmt = mjb_global.stmt_compose;
    } else if(form == MJB_NORMALIZATION_NFKC) {
        stmt = mjb_global.stmt_compat_compose;
    }

    if(stmt && buffer_index > 1) {
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

    if(form == MJB_NORMALIZATION_NFD) {
        stmt = mjb_global.stmt_decompose;
    } else {
        stmt = mjb_global.stmt_compat_decompose;
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

        int found = 0;
        bool valid_decomposition = false;

        switch(form) {
            case MJB_NORMALIZATION_NFD:
            case MJB_NORMALIZATION_NFC:
                valid_decomposition = current_character.decomposition == MJB_DECOMPOSITION_CANONICAL ||
                current_character.decomposition == MJB_DECOMPOSITION_NONE;
                break;

            case MJB_NORMALIZATION_NFKD:
            case MJB_NORMALIZATION_NFKC:
                valid_decomposition = true;
                break;

            default:
                return NULL;
        }

        // Hangul syllables have a special decomposition.
        if(mjb_codepoint_is_hangul_syllable(current_codepoint)) {
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
                ++found;
            }
        } else if(valid_decomposition) {
            // There are no combining characters. Add the character to the output.
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

                ++found;

                if(buffer_index && current_character.combining == MJB_CCC_NOT_REORDERED) {
                    ret = mjb_flush_buffer(characters_buffer, buffer_index, ret, &output_index, output_size, form);
                    buffer_index = 0;
                }

                characters_buffer[buffer_index++] = current_character;
            }

            sqlite3_reset(stmt);
        }

        if(!found) {
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
