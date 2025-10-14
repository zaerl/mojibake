/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mojibake-internal.h"

MJB_EXPORT mojibake mjb_global = {
    .ok = false,
    .memory_alloc = NULL,
    .memory_realloc = NULL,
    .memory_free = NULL,
    .db = NULL,
    .locale = MJB_LOCALE_EN,
    .stmt_get_codepoint = NULL,
    .stmt_get_block = NULL,
    .stmt_is_combining = NULL,
    .stmt_decompose = NULL,
    .stmt_compatibility_decompose = NULL,
    .stmt_compose = NULL,
    .stmt_buffer_character = NULL,
    .stmt_case = NULL,
    .stmt_special_casing = NULL,
    .stmt_line_breaking = NULL,
    .stmt_east_asian_width = NULL,
};

// Initialize the library
MJB_EXPORT bool mjb_initialize(void) {
    if(mjb_global.ok) {
        return true;
    }

    if(mjb_initialize_v2(malloc, realloc, free, NULL, 0)) {
        return true;
    }

    return false;
}

// Initialize the library with custom values
MJB_EXPORT bool mjb_initialize_v2(mjb_alloc_fn alloc_fn, mjb_realloc_fn realloc_fn,
    mjb_free_fn free_fn, const char *db, size_t db_size) {
    if(mjb_global.ok) {
        return true;
    }

    MJB_LOG("DB initialization");

    if(alloc_fn == NULL) {
        alloc_fn = malloc;
    }

    if(realloc_fn == NULL) {
        realloc_fn = realloc;
    }

    if(free_fn == NULL) {
        free_fn = free;
    }

    mjb_global.ok = false;

    int rc = sqlite3_initialize();

    if(rc != SQLITE_OK) {
        MJB_LOG_VA("Can't initialize database: %s", sqlite3_errmsg(mjb_global.db));

        return false;
    }

#ifdef __EMSCRIPTEN__
    if(db != NULL && db_size > 0) {
        rc = sqlite3_open(":memory:", &mjb_global.db);

        if(rc != SQLITE_OK) {
            MJB_LOG_VA("Can't create in-memory database: %s", sqlite3_errmsg(mjb_global.db));

            return false;
        }

        rc = sqlite3_deserialize(mjb_global.db, "main", (unsigned char*)db, db_size, db_size,
            SQLITE_DESERIALIZE_READONLY);

        if(rc != SQLITE_OK) {
            MJB_LOG_VA("Can't deserialize database: %s", sqlite3_errmsg(mjb_global.db));

            return false;
        }
    } else {
        MJB_LOG("No database provided for WASM build");

        return false;
    }
#else
    const char *filename = db;

    if(filename == NULL) {
        filename = getenv("WRD_DB_PATH");

        if(filename == NULL) {
            filename = "./mojibake.db";
        }
    }

    rc = sqlite3_open(filename, &mjb_global.db);

    if(rc != SQLITE_OK) {
        // Try again with the default path.
        rc = sqlite3_open("./mojibake.db", &mjb_global.db);

        if(rc != SQLITE_OK) {
            MJB_LOG_VA("Can't open database: %s", sqlite3_errmsg(mjb_global.db));

            return false;
        }
    }
#endif

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
        rc = sqlite3_prepare_v2(mjb_global.db, QUERY, -1, &STMT, NULL); \
        if(rc != SQLITE_OK) { \
            MJB_LOG_VA("Can't prepare statement: %s", sqlite3_errmsg(mjb_global.db)); \
            return false; \
        }

    const char query[] = "SELECT codepoint, name, category, combining, bidirectional, "
        "decomposition, decimal, digit, numeric, mirrored, uppercase, lowercase, titlecase "
        "FROM unicode_data WHERE codepoint = ?";
    MJB_PREPARE_STMT(mjb_global.stmt_get_codepoint, query)

    const char query_blocks[] = "SELECT * FROM blocks WHERE ? BETWEEN start AND end LIMIT 1";
    MJB_PREPARE_STMT(mjb_global.stmt_get_block, query_blocks)

    // MJB_CATEGORY_MN and MJB_CATEGORY_MC
    const char query_combining[] = "SELECT COUNT(*) from unicode_data WHERE codepoint = ? AND "
        "category IN (5, 6);";
    MJB_PREPARE_STMT(mjb_global.stmt_is_combining, query_combining)

    const char query_decompose[] = "SELECT value FROM decompositions WHERE id = ?";
    MJB_PREPARE_STMT(mjb_global.stmt_decompose, query_decompose)

    const char query_compatibility_decompose[] = "SELECT value FROM compatibility_decompositions "
        "WHERE id = ?";
    MJB_PREPARE_STMT(mjb_global.stmt_compatibility_decompose, query_compatibility_decompose)

    const char query_compose[] = "SELECT composite_codepoint FROM compositions WHERE "
        "starter_codepoint = ? AND combining_codepoint = ?";
    MJB_PREPARE_STMT(mjb_global.stmt_compose, query_compose)

    const char query_buffer_character[] = "SELECT codepoint, combining, decomposition, "
        "quick_check FROM unicode_data WHERE codepoint = ?";
    MJB_PREPARE_STMT(mjb_global.stmt_buffer_character, query_buffer_character)

    const char query_case[] = "SELECT category, uppercase, lowercase, titlecase FROM unicode_data "
        "WHERE codepoint = ?";
    MJB_PREPARE_STMT(mjb_global.stmt_case, query_case)

    const char query_special_casing[] = "SELECT new_case_1, new_case_2, new_case_3 FROM "
        "special_casing WHERE codepoint = ? AND case_type = ?";
    MJB_PREPARE_STMT(mjb_global.stmt_special_casing, query_special_casing)

    const char query_line_breaking[] = "SELECT line_breaking_class, category, extended_pictographic "
        "FROM unicode_data WHERE codepoint = ?";
    MJB_PREPARE_STMT(mjb_global.stmt_line_breaking, query_line_breaking)

    const char query_east_asian_width[] = "SELECT east_asian_width FROM unicode_data WHERE "
        "codepoint = ?";
    MJB_PREPARE_STMT(mjb_global.stmt_east_asian_width, query_east_asian_width)

    const char query_emoji[] = "SELECT * FROM emoji_properties WHERE codepoint = ?";
    MJB_PREPARE_STMT(mjb_global.stmt_get_emoji, query_emoji)

    #undef MJB_PREPARE_STMT

    mjb_global.memory_alloc = alloc_fn;
    mjb_global.memory_realloc = realloc_fn;
    mjb_global.memory_free = free_fn;
    mjb_global.ok = true;

    MJB_LOG("DB initialized successfully");

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

    if(mjb_global.stmt_is_combining) {
        sqlite3_finalize(mjb_global.stmt_is_combining);
    }

    if(mjb_global.stmt_decompose) {
        sqlite3_finalize(mjb_global.stmt_decompose);
    }

    if(mjb_global.stmt_compatibility_decompose) {
        sqlite3_finalize(mjb_global.stmt_compatibility_decompose);
    }

    if(mjb_global.stmt_compose) {
        sqlite3_finalize(mjb_global.stmt_compose);
    }

    if(mjb_global.stmt_buffer_character) {
        sqlite3_finalize(mjb_global.stmt_buffer_character);
    }

    if(mjb_global.stmt_case) {
        sqlite3_finalize(mjb_global.stmt_case);
    }

    if(mjb_global.stmt_special_casing) {
        sqlite3_finalize(mjb_global.stmt_special_casing);
    }

    if(mjb_global.stmt_line_breaking) {
        sqlite3_finalize(mjb_global.stmt_line_breaking);
    }

    if(mjb_global.stmt_east_asian_width) {
        sqlite3_finalize(mjb_global.stmt_east_asian_width);
    }

    if(mjb_global.stmt_get_emoji) {
        sqlite3_finalize(mjb_global.stmt_get_emoji);
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
