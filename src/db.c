#include <stdio.h>
#include <stdlib.h>

#include "db.h"

mjb_connection mjb;

MJB_EXPORT void mjb_db_error() {
    fprintf(stderr, "Error: %s\n", sqlite3_errmsg(mjb.db));
}

/* Initialize the library */
MJB_EXPORT bool mjb_initialize(const char *filename) {
    if(mjb_ready()) {
        return true;
    }

    mjb.db = NULL;
    mjb.memory_alloc = &malloc;
    mjb.memory_realloc = &realloc;
    mjb.memory_free = &free;

    int ret = sqlite3_open_v2(filename, &mjb.db, SQLITE_OPEN_READONLY, NULL);
    DB_CHECK_CLOSE(ret, false)

    ret = sqlite3_prepare_v3(mjb.db, "SELECT * FROM characters WHERE codepoint = ?", -1,
        SQLITE_PREPARE_PERSISTENT, &mjb.char_stmt, NULL);
    DB_CHECK_CLOSE(ret, false)

    ret = sqlite3_prepare_v3(mjb.db, "SELECT codepoint, decomposition FROM decompositions WHERE codepoint = ?", -1,
        SQLITE_PREPARE_PERSISTENT, &mjb.decomposition_stmt, NULL);
    DB_CHECK_CLOSE(ret, false)

    mjb.ok = true;

    return true;
}

/* The library is ready */
MJB_EXPORT bool mjb_ready() {
    return mjb.ok;
}

/* Close the library */
MJB_EXPORT bool mjb_close() {
    if(!mjb_ready()) {
        mjb_db_error();

        return false;
    }

    int ret = SQLITE_ERROR;

    if(mjb.char_stmt) {
        ret = sqlite3_finalize(mjb.char_stmt);
        mjb.char_stmt = NULL;
    }

    if(mjb.decomposition_stmt) {
        ret = sqlite3_finalize(mjb.decomposition_stmt);
        mjb.decomposition_stmt = NULL;
    }

    if(mjb.db) {
        ret = sqlite3_close(mjb.db);
        mjb.db = NULL;
    }

    mjb.ok = false;

    return ret == SQLITE_OK;
}
