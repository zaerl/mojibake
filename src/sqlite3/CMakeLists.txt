if(BUILD_WASM)
    set(SQLITE3_SOURCES
        sqlite3-wasm.c
    )
else()
    set(SQLITE3_SOURCES
        sqlite3.c
    )
endif()

add_library(mojibake_sqlite STATIC ${SQLITE3_SOURCES})
if(MSVC)
    # No GCC/Clang warning flags on MSVC
    target_compile_options(mojibake_sqlite PRIVATE)
else()
    target_compile_options(mojibake_sqlite
        PRIVATE
        # We usually don't use these. But by removing SQLite parts, we have multiple warnings.
        -Wno-unused-function
        -Wno-unused-variable
        -Wno-undefined-internal
        -Wno-unused-but-set-variable
    )
endif()

target_compile_definitions(mojibake_sqlite
    PRIVATE
    # Recommended
    SQLITE_DQS=0
    SQLITE_THREADSAFE=0
    SQLITE_DEFAULT_MEMSTATUS=0
    SQLITE_DEFAULT_WAL_SYNCHRONOUS=1
    SQLITE_LIKE_DOESNT_MATCH_BLOBS
    SQLITE_MAX_EXPR_DEPTH=0
    SQLITE_USE_ALLOCA
    SQLITE_STRICT_SUBTYPE=1
    # See generate-sqlite.sh for more details
    SQLITE_OMIT_ALTERTABLE
    SQLITE_OMIT_ANALYZE
    SQLITE_OMIT_AUTHORIZATION
    SQLITE_OMIT_AUTOINCREMENT
    SQLITE_OMIT_AUTOINIT # Recommended
    SQLITE_OMIT_BLOB_LITERAL
    SQLITE_OMIT_CASE_SENSITIVE_LIKE_PRAGMA
    SQLITE_OMIT_CHECK
    SQLITE_OMIT_COMPILEOPTION_DIAGS
    SQLITE_OMIT_COMPLETE
    SQLITE_OMIT_COMPOUND_SELECT
    SQLITE_OMIT_DECLTYPE # Recommended
    SQLITE_OMIT_DEPRECATED # Recommended
    SQLITE_OMIT_DESERIALIZE
    SQLITE_OMIT_EXPLAIN
    SQLITE_OMIT_FLAG_PRAGMAS
    SQLITE_OMIT_FOREIGN_KEY
    SQLITE_OMIT_GENERATED_COLUMNS
    SQLITE_OMIT_GET_TABLE
    SQLITE_OMIT_INCRBLOB
    SQLITE_OMIT_INTEGRITY_CHECK
    SQLITE_OMIT_INTROSPECTION_PRAGMAS
    SQLITE_OMIT_JSON
    SQLITE_OMIT_LOAD_EXTENSION
    SQLITE_OMIT_LOCALTIME
    SQLITE_OMIT_MEMORYDB
    SQLITE_OMIT_PROGRESS_CALLBACK # Recommended
    SQLITE_OMIT_REINDEX
    SQLITE_OMIT_SCHEMA_PRAGMAS
    SQLITE_OMIT_SCHEMA_VERSION_PRAGMAS
    SQLITE_OMIT_SHARED_CACHE # Recommended
    SQLITE_OMIT_TCL_VARIABLE
    SQLITE_OMIT_TRACE
    SQLITE_OMIT_TRIGGER
    SQLITE_OMIT_UTF16
)

# WASM-specific configuration for SQLite
# Not needed?
#[[if(BUILD_WASM)
    target_compile_options(mojibake_sqlite PRIVATE
        -DSQLITE_OS_OTHER=1
        -DSQLITE_SYSTEM_MALLOC=1
    )
    target_compile_definitions(mojibake_sqlite PRIVATE
        SQLITE_OS_OTHER=1
        SQLITE_SYSTEM_MALLOC=1
    )
endif()]]
