#include <stdio.h>
#include <stdlib.h>

#include "db.h"

MJB_EXPORT bool mjb_db_error(mojibake *mjb) {
    if(!mjb_ready(mjb)) {
        return true;
    }

    fprintf(stderr, "Error: %s\n", sqlite3_errmsg(mjb->db));

    return true;
}

/* Initialize the library */
bool mjb_initialize(const char *filename, mojibake **mjb) {
    return mjb_initialize_v2(filename, mjb, malloc, realloc, free);
}

/* Initialize the library with custom values */
MJB_EXPORT bool mjb_initialize_v2(const char *filename, mojibake **mjb, mjb_alloc_fn alloc_fn, mjb_realloc_fn realloc_fn, mjb_free_fn free_fn) {
    if(mjb == NULL) {
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

    if(alloc_fn && realloc_fn && free_fn) {
        (*mjb)->memory_alloc = alloc_fn;
        (*mjb)->memory_realloc = realloc_fn;
        (*mjb)->memory_free = free_fn;
    } else {
        (*mjb)->memory_alloc = malloc;
        (*mjb)->memory_realloc = realloc;
        (*mjb)->memory_free = free;
    }

    int ret = sqlite3_open_v2(filename, &(*mjb)->db, SQLITE_OPEN_READONLY, NULL);
    DB_CHECK_CLOSE(*mjb, ret, false)

    ret = sqlite3_prepare_v3((*mjb)->db, "SELECT * FROM characters WHERE codepoint = ?", -1,
        SQLITE_PREPARE_PERSISTENT, &(*mjb)->char_stmt, NULL);
    DB_CHECK_CLOSE(*mjb, ret, false)

    ret = sqlite3_prepare_v3((*mjb)->db, "SELECT characters.codepoint, characters.combining, decompositions.decomposition \
        FROM characters\
        LEFT JOIN\
        decompositions ON characters.codepoint = decompositions.codepoint\
        WHERE characters.codepoint = ?", -1,
        SQLITE_PREPARE_PERSISTENT, &(*mjb)->decomposition_stmt, NULL);
    DB_CHECK_CLOSE(*mjb, ret, false)

    (*mjb)->ok = true;

    return true;
}

/* The library is ready */
MJB_EXPORT bool mjb_ready(mojibake *mjb) {
    return mjb != NULL && mjb->ok;
}

/* Close the library */
MJB_EXPORT bool mjb_close(mojibake *mjb) {
    if(!mjb_ready(mjb)) {
        mjb_db_error(mjb);

        return false;
    }

    int ret = SQLITE_ERROR;

    if(mjb->char_stmt) {
        ret = sqlite3_finalize(mjb->char_stmt);
        mjb->char_stmt = NULL;
    }

    if(mjb->decomposition_stmt) {
        ret = sqlite3_finalize(mjb->decomposition_stmt);
        mjb->decomposition_stmt = NULL;
    }

    if(mjb->db) {
        ret = sqlite3_close(mjb->db);
        mjb->db = NULL;
    }

    mjb->ok = false;

    return ret == SQLITE_OK;
}
