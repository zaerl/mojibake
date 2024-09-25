/**
 * The mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "db.h"

/* Initialize the library */
bool mjb_initialize(mojibake **mjb) {
    return mjb_initialize_v2(mjb, malloc, realloc, free);
}

/* Initialize the library with custom values */
MJB_EXPORT bool mjb_initialize_v2(mojibake **mjb, mjb_alloc_fn alloc_fn, mjb_realloc_fn realloc_fn, mjb_free_fn free_fn) {
    if(mjb == NULL || alloc_fn == NULL || realloc_fn == NULL || free_fn == NULL) {
        return false;
    }

    if(alloc_fn) {
        *mjb = alloc_fn(sizeof(mojibake));
    } else {
        *mjb = malloc(sizeof(mojibake));
    }

    if(!*mjb) {
        return false;
    }

    memset(*mjb, 0, sizeof(mojibake));

    if(alloc_fn && realloc_fn && free_fn) {
        (*mjb)->memory_alloc = alloc_fn;
        (*mjb)->memory_realloc = realloc_fn;
        (*mjb)->memory_free = free_fn;
    } else {
        (*mjb)->memory_alloc = malloc;
        (*mjb)->memory_realloc = realloc;
        (*mjb)->memory_free = free;
    }

    (*mjb)->ok = true;

    return true;
}

/* The library is ready */
MJB_EXPORT bool mjb_ready(mojibake *mjb) {
    return mjb != NULL && mjb->ok;
}
