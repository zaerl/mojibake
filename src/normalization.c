/**
 * The mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <string.h>
#include "mojibake.h"

#include <stdio.h>

extern struct mojibake mjb_global;

#if 0
#define DEBUG_PRINTF(fmt, ...) printf(fmt, __VA_ARGS__)
#define DEBUG_SNPRINTF(buffer, size, fmt, ...) snprintf(buffer, size, fmt, __VA_ARGS__)
#define DEBUG_PUTS(str) puts(str)
#else
#define DEBUG_PRINTF(fmt, ...)
#define DEBUG_SNPRINTF(buffer, size, fmt, ...)
#define DEBUG_PUTS(str)
#endif

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

void mjb_string_to_hex(const char *input) {
    for(unsigned i = 0; i < strlen(input); ++i) {
        DEBUG_PRINTF("%02X ", (unsigned char)input[i]);
    }

    DEBUG_PUTS("");
}

static inline char *flush_buffer(mjb_character *characters_buffer, unsigned int buffer_index, char *ret, size_t *output_index, size_t *output_size) {
    mjb_sort(characters_buffer, buffer_index);
    char buffer_utf8[5];
    size_t utf8_size = 0;
    puts("FLUSH BUFFER");

    // Write combining characters.
    for(size_t i = 0; i < buffer_index; ++i) {
        utf8_size = mjb_codepoint_encode(characters_buffer[i].codepoint, (char*)buffer_utf8, 5, MJB_ENCODING_UTF_8);
        printf("FLUSH CODEPOINT %02X\n", characters_buffer[i].codepoint);
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

    /*if(form != MJB_NORMALIZATION_NFD) {
        return NULL;
    }*/

    sqlite3_reset(mjb_global.stmt_decompose);
    sqlite3_clear_bindings(mjb_global.stmt_decompose);

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
        puts("Loop");
        // Find next codepoint.
        state = mjb_utf8_decode_step(state, *index, &current_codepoint);

        if(state == MJB_UTF8_REJECT) {
            // Do nothing. The string is not well-formed
            DEBUG_PUTS("Reject");
            continue;
        }

        // Not found a UTF-8 character, continue.
        if(state != MJB_UTF8_ACCEPT) {
            continue;
        }

        // DEBUG_PRINTF("Codepoint: %X, length %ld\n", current_codepoint, index - buffer);

        // Get current character.
        if(!mjb_codepoint_character(&current_character, current_codepoint)) {
            DEBUG_PUTS("Invalid codepoint");
            continue;
        }

        printf("Char: %X %s, combining: %u, starter %u\n", current_character.codepoint,
            current_character.name, current_character.combining, current_character.combining == MJB_CCC_NOT_REORDERED);

        int found = 0;

        // Hangul syllables have a special decomposition.
        if(mjb_codepoint_is_hangul_syllable(current_codepoint)) {
            mjb_codepoint codepoints[3];
            mjb_hangul_syllable_decomposition(current_codepoint, codepoints);

            for(size_t i = 0; i < 3; ++i) {
                if(codepoints[i] == 0) {
                    continue;
                }

                if(!mjb_codepoint_character(&current_character, codepoints[i])) {
                    DEBUG_PUTS("Invalid hangul character");
                    continue;
                }

                if(buffer_index && current_character.combining == MJB_CCC_NOT_REORDERED) {
                    puts("hangul flush");
                    ret = flush_buffer(characters_buffer, buffer_index, ret, &output_index, output_size);
                    buffer_index = 0;
                }

                printf("Add hangul %X to buffer\n", codepoints[i]);
                // characters_buffer[buffer_index++] = current_character;
                ++found;

                utf8_size = mjb_codepoint_encode(codepoints[i], (char*)buffer_utf8, 5, encoding);
                printf("HANGUL CODEPOINT %02X\n", codepoints[i]);
                ret = mjb_output_string(ret, buffer_utf8, utf8_size, &output_index, output_size);
            }
        } else if(current_character.decomposition == MJB_DECOMPOSITION_CANONICAL ||
            current_character.decomposition == MJB_DECOMPOSITION_NONE) {
            // There are no combining characters. Add the character to the output.
            int rc = sqlite3_bind_int(mjb_global.stmt_decompose, 1, current_codepoint);

            printf("Decomposing: %X\n", current_codepoint);

            if(rc != SQLITE_OK) {
                DEBUG_PUTS("Error sqlite3_bind_int");
                return NULL;
            }

            while((rc = sqlite3_step(mjb_global.stmt_decompose)) == SQLITE_ROW) {
                mjb_codepoint decomposed = (mjb_codepoint)sqlite3_column_int(mjb_global.stmt_decompose, 0);

                if(decomposed == MJB_CODEPOINT_NOT_VALID) {
                    DEBUG_PUTS("Invalid codepoint");
                    continue;
                }

                if(!mjb_codepoint_character(&current_character, decomposed)) {
                    DEBUG_PUTS("Invalid character");
                    continue;
                }

                printf("SQLITE3 FOUND CODEPOINT %02X\n", decomposed);

                ++found;

                if(current_character.combining == MJB_CCC_NOT_REORDERED) {
                    if(buffer_index) {
                        puts("sqlite3 flush");
                        ret = flush_buffer(characters_buffer, buffer_index, ret, &output_index, output_size);
                        buffer_index = 0;
                    }

                    utf8_size = mjb_codepoint_encode(decomposed, (char*)buffer_utf8, 5, encoding);
                    printf("SQLITE3 CODEPOINT %02X\n", decomposed);
                    ret = mjb_output_string(ret, buffer_utf8, utf8_size, &output_index, output_size);
                } else {
                    printf("Add %X to buffer.\n", decomposed);
                    characters_buffer[buffer_index++] = current_character;
                }
            }

            sqlite3_reset(mjb_global.stmt_decompose);
        }/* else {
            printf("Add %X to buffer.\n", current_character.codepoint);
            characters_buffer[buffer_index++] = current_character;
        }*/

        // Simply add the character if not decomposed.
        /*if(!found) {
            if(buffer_index) {
                printf("Write last %u characters\n", buffer_index);
                ret = flush_buffer(characters_buffer, buffer_index, ret, &output_index, output_size);
                buffer_index = 0;
            }

            DEBUG_PUTS("Found zero");
            size_t utf8_size = mjb_codepoint_encode(current_codepoint, (char*)buffer_utf8, 5, encoding);
            printf("NOT FOUND CODEPOINT %02X\n", current_codepoint);
            ret = mjb_output_string(ret, buffer_utf8, utf8_size, &output_index, output_size);
            DEBUG_PUTS("Output string #2");
        }*/
       if(!found) {
            if(current_character.combining == MJB_CCC_NOT_REORDERED) {
                if(buffer_index) {
                    puts("last flush");
                    ret = flush_buffer(characters_buffer, buffer_index, ret, &output_index, output_size);
                    buffer_index = 0;
                }

                utf8_size = mjb_codepoint_encode(current_codepoint, (char*)buffer_utf8, 5, encoding);
                printf("SQLITE3 CODEPOINT %02X\n", current_codepoint);
                ret = mjb_output_string(ret, buffer_utf8, utf8_size, &output_index, output_size);
            } else {
                printf("Add %X to buffer.\n", current_codepoint);
                characters_buffer[buffer_index++] = current_character;
            }
       }
    }

    DEBUG_PUTS("End of loop");

    // We have combining characters in the buffer, we must output them.
    if(buffer_index) {
        DEBUG_PRINTF("Write last %u characters\n", buffer_index);
        ret = flush_buffer(characters_buffer, buffer_index, ret, &output_index, output_size);
        buffer_index = 0;
    }

    // Guarantee null-terminated string
    output_index += 1;

    if(output_index >= *output_size) {
        ret = mjb_realloc(ret, *output_size + 1);
    }

    ret[output_index] = '\0';
    *output_size = output_index - 1;
    mjb_string_to_hex(ret);

    return ret;
}
