/**
 * The mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#ifndef MJB_DB_H
#define MJB_DB_H

#include "mojibake.h"
#include "sqlite/sqlite3.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DB_CHECK(mjb, db_result, ret) if(db_result != SQLITE_OK) { mjb_db_error(mjb); return ret; }
#define DB_CHECK_CLOSE(mjb, db_result, ret) if(db_result != SQLITE_OK) { mjb_db_error(mjb); mjb_close(mjb); return ret; }
#define DB_COLUMN_INT(stmt, name, col) name = sqlite3_column_int(stmt, col);
#define DB_COLUMN_TEXT(stmt, name, col) strncpy((char*)&name, (const char*)sqlite3_column_text(stmt, col), sqlite3_column_bytes(stmt, col));

struct mojibake {
    sqlite3 *db;
    sqlite3_stmt *char_stmt;
    sqlite3_stmt *decomposition_stmt;
    bool ok;
    mjb_alloc_fn memory_alloc;
    mjb_realloc_fn memory_realloc;
    mjb_free_fn memory_free;
};

MJB_EXPORT bool mjb_db_error(mojibake *mjb);

#ifdef __cplusplus
}
#endif

#endif /* MJB_DB_H */
