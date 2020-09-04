/**
 * The mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "db.h"
#include <string.h>

/* Allocate and zero memory*/
MJB_EXPORT void *mjb_alloc(mojibake *mjb, size_t size) {
    if(!mjb_ready(mjb)) {
        return NULL;
    }

    void *allocated = mjb->memory_alloc(size);

    if(allocated) {
        memset(allocated, 0, size);
    }

    return allocated;
}

/* Reallocate memory */
MJB_EXPORT void *mjb_realloc(mojibake *mjb, void *ptr, size_t new_size) {
    if(!mjb_ready(mjb)) {
        return NULL;
    }

    return mjb->memory_realloc(ptr, new_size);
}

/* Free memory */
MJB_EXPORT void mjb_free(mojibake *mjb, void *ptr) {
    if(!mjb_ready(mjb)) {
        return;
    }

    mjb->memory_free(ptr);
}
