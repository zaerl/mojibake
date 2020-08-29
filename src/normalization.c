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
    void *ret = mjb.memory_alloc(size * sizeof(mjb_codepoint));
    unsigned int realloc_step = 2;
    unsigned int i = 0;

    /* Cycle the string */
    do {
        next = mjb_next_codepoint(buffer, size, next, encoding, &codepoint);

        if(next > size) {
            /* ret = mjb.memory_realloc(ret, size * realloc_step);
            ++realloc_step; */
            break;
        }

        /* ASCII characters (U+0000..U+007F) are left unaffected by all of the Normalization Forms */
        if(codepoint <= 0x7F) {
            ((mjb_codepoint*)ret)[i] = codepoint;
        } else if(codepoint < 0xFF && form == MJB_NORMALIZATION_NFC) { /* Latin-1 characters (U+0000..U+00FF) are unaffected by NFC */
            ((mjb_codepoint*)ret)[i] = codepoint;
        }

        /* ((mjb_codepoint*)ret)[i] = codepoint; */

        ++i;
    } while(next);

    return ret;
}
