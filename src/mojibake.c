/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "mojibake.h"
#include "sqlite3/sqlite3.h"

MJB_EXPORT mojibake mjb_global = {
    .ok = false,
    .memory_alloc = NULL,
    .memory_realloc = NULL,
    .memory_free = NULL,
    .db = NULL,
    .stmt_get_codepoint = NULL,
    .stmt_get_block = NULL,
    .stmt_is_combining = NULL,
    .stmt_decompose = NULL,
    .stmt_compatibility_decompose = NULL,
    .stmt_compose = NULL,
    .stmt_compatibility_compose = NULL
};

// Initialize the library
MJB_EXPORT bool mjb_initialize(void) {
    if(mjb_global.ok) {
        return true;
    }

    if(mjb_initialize_v2(malloc, realloc, free, NULL)) {
        return true;
    }

    return false;
}

int mjb_sqlite3_roundup(int size) {
    return (size + 1) & ~1;
}

// Initialize the library with custom values
MJB_EXPORT bool mjb_initialize_v2(mjb_alloc_fn alloc_fn, mjb_realloc_fn realloc_fn, mjb_free_fn free_fn,
    sqlite3_mem_methods *db_mem_methods) {
    if(mjb_global.ok) {
        return true;
    }

    if(alloc_fn == NULL || realloc_fn == NULL || free_fn == NULL) {
        return false;
    }

    mjb_global.ok = false;

    if(db_mem_methods) {
        sqlite3_config(SQLITE_CONFIG_MALLOC, db_mem_methods);
    }

    int rc = sqlite3_initialize();

    if(rc != SQLITE_OK) {
        return false;
    }

    char *filename = getenv("WRD_DB_PATH");

    if(filename == NULL) {
        filename = "./mojibake.db";
    }

    rc = sqlite3_open(filename, &mjb_global.db);

    if(rc != SQLITE_OK) {
        // Try again with the default path.
        rc = sqlite3_open("./mojibake.db", &mjb_global.db);

        if(rc != SQLITE_OK) {
            return false;
        }
    }

    sqlite3_extended_result_codes(mjb_global.db, 1);
    // TODO: Check what PRAGMA are valid
    const char *sql =
        "PRAGMA synchronous = OFF;"
        "PRAGMA temp_store = MEMORY;"
        "PRAGMA journal_mode = OFF;"
        "PRAGMA cache_size = -1000000;"
        "PRAGMA query_only = TRUE;"
        "PRAGMA locking_mode = EXCLUSIVE;"
        "PRAGMA mmap_size = 268435456;";

    rc = sqlite3_exec(mjb_global.db, sql, 0, 0, NULL);

    if(rc != SQLITE_OK) {
        return false;
    }

    #define MJB_PREPARE_STMT(STMT, QUERY) \
        rc = sqlite3_prepare_v2(mjb_global.db, QUERY, sizeof(QUERY), &STMT, NULL); \
        if(rc != SQLITE_OK) { \
            return false; \
        }

    const char query[] = "SELECT * FROM unicode_data WHERE codepoint = ?";
    MJB_PREPARE_STMT(mjb_global.stmt_get_codepoint, query)

    const char query_blocks[] = "SELECT * FROM blocks WHERE ? BETWEEN start AND end LIMIT 1";
    MJB_PREPARE_STMT(mjb_global.stmt_get_block, query_blocks)

    // MJB_CATEGORY_MN and MJB_CATEGORY_MC
    const char query_combining[] = "SELECT COUNT(*) from unicode_data WHERE codepoint = ? AND category IN (5, 6);";
    MJB_PREPARE_STMT(mjb_global.stmt_is_combining, query_combining)

    const char query_decompose[] = "SELECT value FROM decompositions WHERE id = ?";
    MJB_PREPARE_STMT(mjb_global.stmt_decompose, query_decompose)

    const char query_compatibility_decompose[] = "SELECT value FROM compatibility_decompositions WHERE id = ?";
    MJB_PREPARE_STMT(mjb_global.stmt_compatibility_decompose, query_compatibility_decompose)

    const char query_compose[] = "SELECT id FROM decompositions WHERE value IN (?, ?) GROUP BY id HAVING COUNT(DISTINCT VALUE) = 2";
    MJB_PREPARE_STMT(mjb_global.stmt_compose, query_compose)

    const char query_compatibility_compose[] = "SELECT id FROM compatibility_decompositions WHERE value IN (?, ?) GROUP BY id HAVING COUNT(DISTINCT VALUE) = 2";
    MJB_PREPARE_STMT(mjb_global.stmt_compatibility_compose, query_compatibility_compose)

    #undef MJB_PREPARE_STMT

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

    if(mjb_global.stmt_get_codepoint) {
        sqlite3_finalize(mjb_global.stmt_get_codepoint);
    }

    if(mjb_global.stmt_get_block) {
        sqlite3_finalize(mjb_global.stmt_get_block);
    }

    if(mjb_global.stmt_decompose) {
        sqlite3_finalize(mjb_global.stmt_decompose);
    }

    if(mjb_global.stmt_compatibility_decompose) {
        sqlite3_finalize(mjb_global.stmt_compatibility_decompose);
    }

    if(mjb_global.stmt_is_combining) {
        sqlite3_finalize(mjb_global.stmt_is_combining);
    }

    if(mjb_global.db) {
        sqlite3_close(mjb_global.db);
    }

    mjb_global.db = NULL;
}

// Allocate and zero memory
MJB_EXPORT void *mjb_alloc(size_t size) {
    if(!mjb_initialize()) {
        return NULL;
    }

    void *allocated = mjb_global.memory_alloc(size);

    if(allocated) {
        memset(allocated, 0, size);
    }

    return allocated;
}

// Reallocate memory
MJB_EXPORT void *mjb_realloc(void *ptr, size_t new_size) {
    if(!mjb_initialize()) {
        return NULL;
    }

    return mjb_global.memory_realloc(ptr, new_size);
}

// Free memory
MJB_EXPORT void mjb_free(void *ptr) {
    if(!mjb_initialize()) {
        return;
    }

    mjb_global.memory_free(ptr);
}
