@echo off
setlocal EnableDelayedExpansion

REM The Mojibake library
REM This file is distributed under the MIT License. See LICENSE for details.

set SQLITE_VERSION=3510000
set SQLITE_YEAR=2025

if not exist "sqlite-src-%SQLITE_VERSION%" (
    curl -o sqlite3.zip "https://www.sqlite.org/%SQLITE_YEAR%/sqlite-src-%SQLITE_VERSION%.zip"
    if errorlevel 1 (
        echo ERROR: Failed to download SQLite source.
        exit /b 1
    )
    tar -xf sqlite3.zip
    if errorlevel 1 (
        echo ERROR: Failed to extract SQLite source.
        exit /b 1
    )
)

cd sqlite-src-%SQLITE_VERSION%
if errorlevel 1 (
    echo ERROR: Cannot enter directory sqlite-src-%SQLITE_VERSION%.
    exit /b 1
)

nmake /f Makefile.msc clean

REM OPTS is passed to both lemon (parse.y generator) and intermediate compilers via Makefile.msc.
REM This is critical: lemon must receive these flags so it conditionally guards destructor calls
REM in the generated parse.c (e.g. sqlite3DeleteTriggerStep under SQLITE_OMIT_TRIGGER).
REM Using CCOPTS instead would bypass lemon and cause LNK2019 unresolved external errors.
set OPTS=-DSQLITE_MAX_MMAP_SIZE=0
set OPTS=%OPTS% -DSQLITE_OMIT_ALTERTABLE
set OPTS=%OPTS% -DSQLITE_OMIT_ANALYZE
set OPTS=%OPTS% -DSQLITE_OMIT_AUTHORIZATION
set OPTS=%OPTS% -DSQLITE_OMIT_AUTOINCREMENT
set OPTS=%OPTS% -DSQLITE_OMIT_AUTOINIT
set OPTS=%OPTS% -DSQLITE_OMIT_BLOB_LITERAL
set OPTS=%OPTS% -DSQLITE_OMIT_CASE_SENSITIVE_LIKE_PRAGMA
set OPTS=%OPTS% -DSQLITE_OMIT_CHECK
set OPTS=%OPTS% -DSQLITE_OMIT_COMPILEOPTION_DIAGS
set OPTS=%OPTS% -DSQLITE_OMIT_COMPLETE
set OPTS=%OPTS% -DSQLITE_OMIT_COMPOUND_SELECT
set OPTS=%OPTS% -DSQLITE_OMIT_DECLTYPE
set OPTS=%OPTS% -DSQLITE_OMIT_DEPRECATED
set OPTS=%OPTS% -DSQLITE_OMIT_EXPLAIN
set OPTS=%OPTS% -DSQLITE_OMIT_FLAG_PRAGMAS
set OPTS=%OPTS% -DSQLITE_OMIT_FOREIGN_KEY
set OPTS=%OPTS% -DSQLITE_OMIT_GENERATED_COLUMNS
set OPTS=%OPTS% -DSQLITE_OMIT_GET_TABLE
set OPTS=%OPTS% -DSQLITE_OMIT_INCRBLOB
set OPTS=%OPTS% -DSQLITE_OMIT_INTEGRITY_CHECK
set OPTS=%OPTS% -DSQLITE_OMIT_INTROSPECTION_PRAGMAS
set OPTS=%OPTS% -DSQLITE_OMIT_JSON
set OPTS=%OPTS% -DSQLITE_OMIT_LOAD_EXTENSION
set OPTS=%OPTS% -DSQLITE_OMIT_LOCALTIME
set OPTS=%OPTS% -DSQLITE_OMIT_PROGRESS_CALLBACK
set OPTS=%OPTS% -DSQLITE_OMIT_REINDEX
set OPTS=%OPTS% -DSQLITE_OMIT_SCHEMA_PRAGMAS
set OPTS=%OPTS% -DSQLITE_OMIT_SCHEMA_VERSION_PRAGMAS
set OPTS=%OPTS% -DSQLITE_OMIT_SHARED_CACHE
set OPTS=%OPTS% -DSQLITE_OMIT_TCL_VARIABLE
set OPTS=%OPTS% -DSQLITE_OMIT_TRACE
set OPTS=%OPTS% -DSQLITE_OMIT_TRIGGER
set OPTS=%OPTS% -DSQLITE_OMIT_UTF16
set OPTS=%OPTS% -DSQLITE_OMIT_WAL
REM SQLITE_OMIT_ATTACH         To solve missing sqlite3DbIsNamed symbol
REM SQLITE_OMIT_AUTOMATIC_INDEX
REM SQLITE_OMIT_AUTORESET
REM SQLITE_OMIT_AUTOVACUUM
REM SQLITE_OMIT_BETWEEN_OPTIMIZATION
REM SQLITE_OMIT_CAST
REM SQLITE_OMIT_CTE
REM SQLITE_OMIT_DATETIME_FUNCS To solve missing field 'u' initializer
REM SQLITE_OMIT_DESERIALIZE
REM SQLITE_OMIT_FLOATING_POINT Not needed?
REM SQLITE_OMIT_HEX_INTEGER
REM SQLITE_OMIT_LIKE_OPTIMIZATION
REM SQLITE_OMIT_LOOKASIDE
REM SQLITE_OMIT_MEMORYDB
REM SQLITE_OMIT_OR_OPTIMIZATION
REM SQLITE_OMIT_PAGER_PRAGMAS  To solve missing sqlite3BtreeSetPagerFlags symbol
REM SQLITE_OMIT_PRAGMA
REM SQLITE_OMIT_QUICKBALANCE
REM SQLITE_OMIT_SEH
REM SQLITE_OMIT_SUBQUERY
REM SQLITE_OMIT_TEMPDB
REM SQLITE_OMIT_TRUNCATE_OPTIMIZATION
REM SQLITE_OMIT_VACUUM          To solve missing sqlite3PagerClearCache symbol
REM SQLITE_OMIT_VIEW
REM SQLITE_OMIT_VIRTUALTABLE
REM SQLITE_OMIT_WINDOWFUNC
REM SQLITE_OMIT_WSD
REM SQLITE_OMIT_XFER_OPT
REM SQLITE_UNTESTABLE
REM SQLITE_ZERO_MALLOC

nmake /f Makefile.msc sqlite3.c OPTS="%OPTS%"
if errorlevel 1 (
    echo ERROR: nmake failed.
    exit /b 1
)

REM Append windows-fixes.c to the generated sqlite3.c
type ..\windows-fixes.c >> sqlite3.c

move /Y sqlite3.c ..\..\..\src\sqlite3\sqlite3.c
move /Y sqlite3.h ..\..\..\src\sqlite3\sqlite3.h
copy /Y ext\wasm\api\sqlite3-wasm.c ..\..\..\src\sqlite3\sqlite3-wasm.c

endlocal
