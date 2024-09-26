/**
 * The mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "mojibake.h"
#include "sqlite3/sqlite3.h"

MJB_EXPORT mojibake mjb_global = { false, NULL, NULL, NULL, NULL, NULL };

// Initialize the library
MJB_EXPORT bool mjb_initialize(void) {
    if(mjb_global.ok) {
        return true;
    }

    return mjb_initialize_v2(malloc, realloc, free);
}

// Initialize the library with custom values
MJB_EXPORT bool mjb_initialize_v2(mjb_alloc_fn alloc_fn, mjb_realloc_fn realloc_fn, mjb_free_fn free_fn) {
    if(mjb_global.ok) {
        return true;
    }

    if(alloc_fn == NULL || realloc_fn == NULL || free_fn == NULL) {
        return false;
    }

    mjb_global.ok = false;

    int rc = sqlite3_initialize();

    if(rc != SQLITE_OK) {
        return false;
    }

    char *filename = getenv("WRD_DB_PATH");
    rc = sqlite3_open(filename, &mjb_global.db);

    if(rc != SQLITE_OK) {
        return false;
    }

    sqlite3_extended_result_codes(mjb_global.db, 1);
    const char *sql =
        "PRAGMA synchronous = OFF;"
        "PRAGMA locking_mode = EXCLUSIVE;"
        "PRAGMA mmap_size = 268435456;";

    rc = sqlite3_exec(mjb_global.db, sql, 0, 0, NULL);

    if(rc != SQLITE_OK) {
        return false;
    }

    const char query[] = "SELECT * FROM unicode_data WHERE codepoint = ?";
    rc = sqlite3_prepare_v2(mjb_global.db, query, sizeof(query), &mjb_global.get_codepoint, NULL);

    if(rc != SQLITE_OK) {
        return false;
    }

    mjb_global.memory_alloc = alloc_fn;
    mjb_global.memory_realloc = realloc_fn;
    mjb_global.memory_free = free_fn;
    mjb_global.ok = true;

    return true;
}

MJB_EXPORT void mjb_shutdown(void) {
    if(!mjb_global.ok) {
        return;
    }

    mjb_global.ok = false;
    mjb_global.memory_free = NULL;
    mjb_global.memory_realloc = NULL;
    mjb_global.memory_alloc = NULL;

    if(mjb_global.get_codepoint) {
        sqlite3_finalize(mjb_global.get_codepoint);
    }

    if(mjb_global.db) {
        sqlite3_close(mjb_global.db);
    }

    mjb_global.db = NULL;
}

/* Allocate and zero memory*/
MJB_EXPORT void *mjb_alloc(size_t size) {
    mjb_initialize();
    void *allocated = mjb_global.memory_alloc(size);

    if(allocated) {
        memset(allocated, 0, size);
    }

    return allocated;
}

/* Reallocate memory */
MJB_EXPORT void *mjb_realloc(void *ptr, size_t new_size) {
    mjb_initialize();

    return mjb_global.memory_realloc(ptr, new_size);
}

/* Free memory */
MJB_EXPORT void mjb_free(void *ptr) {
    mjb_initialize();
    mjb_global.memory_free(ptr);
}
