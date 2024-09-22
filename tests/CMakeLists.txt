set(TEST_SOURCES
    attractor/attractor.c
    array.c
    codepoint.c
    db.c
    encoding.c
    memory.c
    normalization.c
    plane.c
    test.c
    version.c
)

add_executable(mojibake-test ${TEST_SOURCES})
target_link_libraries(mojibake-test PRIVATE mojibake_lib)
