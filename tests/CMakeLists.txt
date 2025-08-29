set(TEST_SOURCES
    attractor/attractor.c
    breaking.c
    case.c
    cjk.c
    codepoint.c
    encoding.c
    hangul-composition.c
    hangul.c
    mojibake.c
    next.c
    normalization.c
    plane.c
    quick-check.c
    special-case.c
    string.c
    test.c
    utf8.c
    version.c
)

add_executable(mojibake-test ${TEST_SOURCES})
target_link_libraries(mojibake-test PRIVATE mojibake_lib mojibake_sqlite)

# Add test to CTest
add_test(NAME "Mojibake Tests" COMMAND mojibake-test)

# Set environment variable for database path and working directory
set_tests_properties("Mojibake Tests" PROPERTIES
    ENVIRONMENT "WRD_DB_PATH=${CMAKE_SOURCE_DIR}/mojibake.db"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)
