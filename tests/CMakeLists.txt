set(TEST_SOURCES
    attractor/attractor.c
    cjk.c
    codepoint.c
    encoding.c
    hangul.c
    hangul-composition.c
    mojibake.c
    next.c
    normalization.c
    plane.c
    string.c
    utf8.c
    test.c
    version.c
)

add_executable(mojibake-test ${TEST_SOURCES})
target_link_libraries(mojibake-test PRIVATE mojibake_lib mojibake_sqlite)
