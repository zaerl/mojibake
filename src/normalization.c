/**
 * The mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "array.h"
#include "db.h"

static size_t mjb_next_codepoint(void *buffer, size_t size, size_t index, mjb_encoding encoding, mjb_codepoint *codepoint) {
    /* if(encoding == MJB_ENCODING_UTF_32) { */
    *codepoint = ((mjb_codepoint*)buffer)[index];

    return index + 1;
}

/* Normalize a string */
MJB_EXPORT void *mjb_normalize(mojibake *mjb, void *source, size_t source_size, size_t *output_size, mjb_encoding encoding, mjb_normalization form) {
    if(!mjb_ready(mjb)) {
        return NULL;
    }

    if(source_size == 0) {
        return NULL;
    }

    if(form == MJB_ENCODING_UNKNOWN) {
        encoding = mjb_string_encoding(source, source_size);
    }

    if(encoding == MJB_ENCODING_UNKNOWN) {
        return NULL;
    }

    mjb_codepoint codepoint;
    size_t next = 0;
    size_t size = source_size;
    unsigned int i = 0;
    unsigned short combining = 0;
    bool starter = false;

    mjb_array array;

    *output_size = 0;

    /* Cycle the string */
    do {
        next = mjb_next_codepoint(source, source_size, next, encoding, &codepoint);

        if(next > size) {
            /* ret = mjb_realloc(ret, size * realloc_step);
            ++realloc_step; */
            break;
        }

        /* ASCII characters (U+0000..U+007F) are left unaffected by all of the Normalization Forms */
        /* Latin-1 characters (U+0000..U+00FF) are unaffected by NFC */
        if(codepoint <= 0x7F || (codepoint < 0xFF && form == MJB_NORMALIZATION_NFC)) {
            mjb_array_push(mjb, &array, (char*)&codepoint);

            ++i;
        } else {
            do {
                break;
            } while(1);

            /* The codepoint is a starter */
            starter = combining == 0;

            /*ret = sqlite3_clear_bindings(mjb.decomposition_stmt);
            DB_CHECK(ret, false)*/
        }

        /* ((mjb_codepoint*)ret)[i] = codepoint; */
    } while(next < source_size);

    *output_size = i;

    return array.buffer;
}
