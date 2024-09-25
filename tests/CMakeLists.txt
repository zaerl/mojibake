set(TEST_SOURCES
    attractor/attractor.c
    encoding.c
    memory.c
    plane.c
    test.c
    version.c
)

add_executable(mojibake-test ${TEST_SOURCES})
target_link_libraries(mojibake-test PRIVATE mojibake_lib mojibake_sqlite)
