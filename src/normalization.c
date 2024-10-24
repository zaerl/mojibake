/**
 * The mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <string.h>
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

static inline char *flush_buffer(mjb_character *characters_buffer, unsigned int buffer_index, char *ret, size_t *output_index, size_t *output_size) {
    mjb_sort(characters_buffer, buffer_index);
    char buffer_utf8[5];
    size_t utf8_size = 0;

    // Write combining characters.
    for(size_t i = 0; i < buffer_index; ++i) {
        utf8_size = mjb_codepoint_encode(characters_buffer[i].codepoint, (char*)buffer_utf8, 5, MJB_ENCODING_UTF_8);
        ret = mjb_output_string(ret, buffer_utf8, utf8_size, output_index, output_size);
    }

    return ret;
}

/**
 * Normalize a string
 *
 * First example:
 *   1E0A ccc 0 LATIN CAPITAL LETTER D WITH DOT ABOVE
 *
 * NFD:
 * 0044 LATIN CAPITAL LETTER D ccc 0
 * 0307 COMBINING DOT ABOVE ccc 230
 *
 * Second example:
 *   1E0A ccc 0 LATIN CAPITAL LETTER D WITH DOT ABOVE
 *   0323 COMBINING DOT BELOW ccc 220
 *
 * NFD:
 * Should be: 0044 0307 0323
 *
 * But is: 0044 0323 0307. Ordered by ccc
 *
 * 0044 LATIN CAPITAL LETTER D ccc 0
 * 0323 COMBINING DOT BELOW ccc 220
 * 0307 COMBINING DOT ABOVE ccc 230
 */
MJB_EXPORT char *mjb_normalize(char *buffer, size_t size, size_t *output_size, mjb_encoding encoding, mjb_normalization form) {
    if(!mjb_initialize()) {
        return NULL;
    }

    if(output_size == NULL || buffer == 0 || encoding != MJB_ENCODING_UTF_8) {
        return NULL;
    }

    if(size == 0) {
        output_size = 0;

        return NULL;
    }

    sqlite3_stmt *stmt = form == MJB_NORMALIZATION_NFD ? mjb_global.stmt_decompose : mjb_global.stmt_compat_decompose;

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

    // UTF-8 buffer.
    const char *index = buffer;
    char buffer_utf8[5];
    size_t utf8_size = 0;

    // Loop through the string.
    for(; *index; ++index) {
        // Find next codepoint.
        state = mjb_utf8_decode_step(state, *index, &current_codepoint);

        if(state == MJB_UTF8_REJECT) {
            // Do nothing. The string is not well-formed
            continue;
        }

        // Not found a UTF-8 character, continue.
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
                valid_decomposition = current_character.decomposition == MJB_DECOMPOSITION_CANONICAL ||
                current_character.decomposition == MJB_DECOMPOSITION_NONE;

                break;
            case MJB_NORMALIZATION_NFKD:
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

                if(buffer_index && current_character.combining == MJB_CCC_NOT_REORDERED) {
                    ret = flush_buffer(characters_buffer, buffer_index, ret, &output_index, output_size);
                    buffer_index = 0;
                }

                // characters_buffer[buffer_index++] = current_character;
                ++found;

                utf8_size = mjb_codepoint_encode(codepoints[i], (char*)buffer_utf8, 5, encoding);
                ret = mjb_output_string(ret, buffer_utf8, utf8_size, &output_index, output_size);
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

                if(current_character.combining == MJB_CCC_NOT_REORDERED) {
                    if(buffer_index) {
                        ret = flush_buffer(characters_buffer, buffer_index, ret, &output_index, output_size);
                        buffer_index = 0;
                    }

                    utf8_size = mjb_codepoint_encode(decomposed, (char*)buffer_utf8, 5, encoding);
                    ret = mjb_output_string(ret, buffer_utf8, utf8_size, &output_index, output_size);
                } else {
                    characters_buffer[buffer_index++] = current_character;
                }
            }

            sqlite3_reset(stmt);
        }

        if(!found) {
            if(current_character.combining == MJB_CCC_NOT_REORDERED) {
                if(buffer_index) {
                    ret = flush_buffer(characters_buffer, buffer_index, ret, &output_index, output_size);
                    buffer_index = 0;
                }

                utf8_size = mjb_codepoint_encode(current_codepoint, (char*)buffer_utf8, 5, encoding);
                ret = mjb_output_string(ret, buffer_utf8, utf8_size, &output_index, output_size);
            } else {
                characters_buffer[buffer_index++] = current_character;
            }
       }
    }

    // We have combining characters in the buffer, we must output them.
    if(buffer_index) {
        ret = flush_buffer(characters_buffer, buffer_index, ret, &output_index, output_size);
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
