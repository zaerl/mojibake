#include "mojibake.h"
#include "sqlite/sqlite3.h"

#define DB_CHECK(db_result, ret) if(db_result != SQLITE_OK) { mjb_db_error(); return ret; }
#define DB_CHECK_CLOSE(db_result, ret) if(db_result != SQLITE_OK) { mjb_db_error(); mjb_close(); return ret; }
#define DB_COLUMN_INT(stmt, name, col) name = sqlite3_column_int(stmt, col);
#define DB_COLUMN_TEXT(stmt, name, col) strncpy((char*)&name, (const char*)sqlite3_column_text(stmt, col), sqlite3_column_bytes(stmt, col));

typedef struct mjb_connection {
    sqlite3 *db;
    sqlite3_stmt *char_stmt;
    sqlite3_stmt *decomposition_stmt;
    bool ok;
    mjb_alloc memory_alloc;
    mjb_realloc memory_realloc;
    mjb_free memory_free;
} mjb_connection;

extern mjb_connection mjb;

MJB_EXPORT void mjb_db_error();
