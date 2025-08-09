/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <string.h>

#include "mojibake-internal.h"
#include "utf8.h"
#include "utf16.h"

extern mojibake mjb_global;

MJB_EXPORT char *mjb_string_output(char *ret, char *input, size_t input_size, size_t *output_index, size_t *output_size) {
    if(!input_size) {
        return NULL;
    }

    if(*output_index + input_size > *output_size) {
        *output_size *= 2;
        ret = mjb_realloc(ret, *output_size);
    }

    memcpy((char*)ret + *output_index, input, input_size);
    *output_index += input_size;

    return ret;
}

/**
 * Return size of a string.
 */
MJB_EXPORT size_t mjb_strnlen(const char *buffer, size_t max_length, mjb_encoding encoding) {
    if(buffer == 0 || max_length == 0) {
        return 0;
    }

    uint8_t state = MJB_UTF8_ACCEPT;
    mjb_codepoint codepoint;
    size_t count = 0;

    if(encoding == MJB_ENCODING_UTF_8) {
        const char *current = buffer;

        while(*current && (size_t)(current - buffer) < max_length) {
            state = mjb_utf8_decode_step(state, *current, &codepoint);

            if(state == MJB_UTF8_ACCEPT) {
                ++count;
            }

            ++current;
        }
    } else if(encoding == MJB_ENCODING_UTF_16_LE || encoding == MJB_ENCODING_UTF_16_BE) {
        state = MJB_UTF16_ACCEPT;
        const uint8_t *bytes = (const uint8_t *)buffer;

        for(size_t i = 0; i < max_length; i += 2) {
            uint16_t unit = encoding == MJB_ENCODING_UTF_16_BE ?
                (bytes[i] << 8) | bytes[i + 1] :
                bytes[i] | (bytes[i + 1] << 8);

            state = mjb_utf16_decode_step(state, unit, &codepoint);

            if(state == MJB_UTF16_ACCEPT) {
                ++count;
            }
        }
    }

    return count;
}

MJB_EXPORT size_t mjb_strncmp(const char *s1, const char *s2, size_t max_length, mjb_encoding encoding) {
    if(s1 == 0 || s2 == 0 || encoding != MJB_ENCODING_UTF_8) {
        // We return 0 to indicate an error. But the behavior is undefined.
        return 0;
    }

    return 0;
}

MJB_EXPORT char *mjb_case(const char *buffer, size_t length, mjb_case_type type, mjb_encoding encoding) {
    if(length == 0) {
        return (char*)buffer;
    }

    if(!mjb_initialize()) {
        return false;
    }

    if(encoding != MJB_ENCODING_UTF_8) {
        return false;
    }

    uint8_t state = MJB_UTF8_ACCEPT;
    mjb_codepoint current_codepoint;
    sqlite3_stmt *stmt = mjb_global.stmt_case;
    char *output = mjb_alloc(length);

    // char *output = mjb_alloc(length);
    size_t output_index = 0;
    size_t output_size = length;

    unsigned int case_index = 0;

    if(type == MJB_CASE_UPPER) {
        case_index = 0;
    } else if(type == MJB_CASE_LOWER) {
        case_index = 1;
    } else if(type == MJB_CASE_TITLE) {
        case_index = 2;
    } else if(type == MJB_CASE_CASEFOLD) {
        // Not implemented.
    }

    const char *index = buffer;
    const char *end = buffer + length;

    for(; index < end && *index; ++index) {
        // Find next codepoint.
        state = mjb_utf8_decode_step(state, *index, &current_codepoint);

        if(state == MJB_UTF8_REJECT) {
            // Do nothing. The string is not well-formed.
            return NULL;
        }

        if(state != MJB_UTF8_ACCEPT) {
            continue;
        }

        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);

        int rc = sqlite3_bind_int(stmt, 1, current_codepoint);

        if(rc != SQLITE_OK) {
            return false;
        }

        rc = sqlite3_step(stmt);

        if(rc != SQLITE_ROW) {
            return false;
        }

        if(sqlite3_column_type(stmt, case_index) == SQLITE_NULL) {
            // Skip.
        } else {
            current_codepoint = (mjb_codepoint)sqlite3_column_int(stmt, case_index);
        }

        char buffer_utf8[5];
        size_t utf8_size = mjb_codepoint_encode(current_codepoint, (char*)buffer_utf8, 5, MJB_ENCODING_UTF_8);

        output = mjb_string_output(output, buffer_utf8, utf8_size, &output_index, &output_size);
    }

    if(output_index >= output_size) {
        output = mjb_realloc(output, output_size + 1);
    }

    output[output_index] = '\0';

    return output;
}
