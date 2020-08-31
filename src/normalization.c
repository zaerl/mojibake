#include "mojibake.h"
#include "db.h"

static size_t mjb_next_codepoint(void *buffer, size_t size, size_t index, mjb_encoding encoding, mjb_codepoint *codepoint) {
    /* if(encoding == MJB_ENCODING_UTF_32) { */
    *codepoint = ((mjb_codepoint*)buffer)[index];

    return index + 1;
}

/* Normalize a string */
MJB_EXPORT void *mjb_normalize(void *buffer, size_t size, mjb_encoding encoding, mjb_normalization form) {
    if(size == 0) {
        return NULL;
    }

    if(encoding == MJB_ENCODING_UNKNOWN) {
        encoding = mjb_string_encoding(buffer, size);
    }

    if(encoding == MJB_ENCODING_UNKNOWN) {
        return NULL;
    }

    mjb_codepoint codepoint;
    size_t next = 0;
    void *ret = mjb_alloc(size * sizeof(mjb_codepoint));
    unsigned int realloc_step = 2;
    unsigned int i = 0;

    /* Cycle the string */
    do {
        next = mjb_next_codepoint(buffer, size, next, encoding, &codepoint);

        if(next > size) {
            /* ret = mjb_realloc(ret, size * realloc_step);
            ++realloc_step; */
            break;
        }

        /* ASCII characters (U+0000..U+007F) are left unaffected by all of the Normalization Forms */
        /* Latin-1 characters (U+0000..U+00FF) are unaffected by NFC */
        if(codepoint <= 0x7F || (codepoint < 0xFF && form == MJB_NORMALIZATION_NFC)) {
            if(i == size) {
                size = size * realloc_step;
                ret = mjb_realloc(ret, size * sizeof(mjb_codepoint));
                /* ++realloc_step; */
            }

            ((mjb_codepoint*)ret)[i] = codepoint;
            ++i;
        } else {
            int res = sqlite3_bind_int(mjb.decomposition_stmt, 1, codepoint);
            DB_CHECK(res, NULL)

            do {
                res = sqlite3_step(mjb.decomposition_stmt);

                if(res == SQLITE_ROW) {
                    if(i == size) {
                        size = size * realloc_step;
                        ret = mjb_realloc(ret, size * sizeof(mjb_codepoint));
                        /* ++realloc_step; */
                    }

                    DB_COLUMN_INT(mjb.decomposition_stmt, ((mjb_codepoint*)ret)[i], 0);

                    ++i;
                } else if(res == SQLITE_DONE) {
                    break;
                } else {
                    DB_CHECK(res, NULL)
                }
            } while(1);

            /*ret = sqlite3_clear_bindings(mjb.decomposition_stmt);
            DB_CHECK(ret, false)*/

            res = sqlite3_reset(mjb.decomposition_stmt);
            DB_CHECK(res, NULL)
        }

        /* ((mjb_codepoint*)ret)[i] = codepoint; */
    } while(next);

    return ret;
}
