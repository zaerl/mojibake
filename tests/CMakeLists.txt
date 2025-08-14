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
