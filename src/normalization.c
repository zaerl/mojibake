/**
 * The mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <string.h>
#include "mojibake.h"

extern struct mojibake mjb_global;

/**
 * Normalize a string
 */
MJB_EXPORT char *mjb_normalize(char *buffer, size_t size, size_t *output_size, mjb_encoding encoding, mjb_normalization form) {
    if(!mjb_initialize()) {
        return NULL;
    }

    size_t next = 0;
    const char *current = buffer;
    size_t remaining = size;
    int rc = 0;

    if(output_size == NULL) {
        return NULL;
    }

    if(buffer == 0) {
        return NULL;
    }

    if(size == 0) {
        output_size = 0;

        return NULL;
    }

    // The encoding is unknown
    if(encoding != MJB_ENCODING_UTF_8) {
        return NULL;
    }

    if(form != MJB_NORMALIZATION_NFD) {
        return NULL;
    }

    sqlite3_reset(mjb_global.decompose);
    sqlite3_clear_bindings(mjb_global.decompose);

    char *ret = mjb_alloc(size);
    size_t output_index = 0;
    char buffer_utf8[5];
    *output_size = size;

    while(current < (char*)buffer + size) {
        mjb_codepoint codepoint = mjb_string_next_codepoint(current, remaining, &next);

        if(codepoint != MJB_CODEPOINT_NOT_VALID) {
            rc = sqlite3_bind_int(mjb_global.decompose, 1, codepoint);

            if(rc != SQLITE_OK) {
                return NULL;
            }

            while((rc = sqlite3_step(mjb_global.decompose)) == SQLITE_ROW) {
                mjb_codepoint decomposed = (mjb_codepoint)sqlite3_column_int(mjb_global.decompose, 0);

                if(decomposed != MJB_CODEPOINT_NOT_VALID) {
                    if(!mjb_codepoint_encode(decomposed, (char*)buffer_utf8, 5, encoding)) {
                        return NULL;
                    }

                    size_t buffer_utf8_size = strnlen(buffer_utf8, 5);

                    if(output_index + buffer_utf8_size > *output_size) {
                        *output_size *= 2;
                        ret = mjb_realloc(ret, *output_size);
                    }

                    memcpy((char*)ret + output_index, buffer_utf8, buffer_utf8_size);
                    output_index += buffer_utf8_size;
                }
            }

            sqlite3_reset(mjb_global.decompose);
        }

        current += next;
        remaining -= next;
    }

    // Guarantee null-terminated string
    output_index += 1;

    if(output_index >= *output_size) {
        ret = mjb_realloc(ret, *output_size + 1);
        ret[output_index] = '\0';
    }

    ret[output_index] = '\0';

    *output_size = output_index;
    // *output_size = current - (const char*)buffer;

    return ret;
}
