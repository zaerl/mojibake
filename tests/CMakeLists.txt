set(TEST_SOURCES
    attractor/attractor.c
    breaking.c
    case.c
    cjk.c
    codepoint.c
    emoji.c
    encoding.c
    east-asian-width.c
    filter.c
    hangul-composition.c
    hangul.c
    mojibake.c
    next.c
    normalization.c
    plane.c
    properties.c
    quick-check.c
    segmentation.c
    special-case.c
    string.c
    test.c
    version.c
)

if(BUILD_CPP)
    # Add C++ test sources
    list(APPEND TEST_SOURCES ext/cpp/mojibake.cpp)
    list(APPEND TEST_SOURCES ext/cpp/normalization.cpp)
endif()

# Add getopt compatibility for Windows
if(WIN32)
    list(APPEND TEST_SOURCES ../src/shell/getopt/getopt.c)
endif()

add_executable(mojibake-test ${TEST_SOURCES})

target_link_libraries(mojibake-test PRIVATE mojibake_lib mojibake_sqlite)

# Force C++ compilation if BUILD_CPP is enabled
if(BUILD_CPP)
    set_target_properties(mojibake-test PROPERTIES
        LINKER_LANGUAGE CXX
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
    )
    set_source_files_properties(${TEST_SOURCES} PROPERTIES LANGUAGE CXX)
endif()

if(WIN32)
    # Suppress warnings for Windows
    target_compile_options(mojibake-test
        PRIVATE
        /wd4996 # Suppress 'the POSIX name for this item is deprecated'
        /wd4245 # Suppress 'signed/unsigned mismatch'
        /wd4702 # Suppress 'unreachable code'
        /wd4701 # Suppress 'potentially uninitialized local variable'
    )
endif()

# Add test to CTest
add_test(NAME "Breaking" COMMAND mojibake-test -f breaking)
add_test(NAME "Case" COMMAND mojibake-test -f case)
add_test(NAME "CJK" COMMAND mojibake-test -f cjk)
add_test(NAME "Codepoint" COMMAND mojibake-test -f codepoint)
add_test(NAME "East Asian Width" COMMAND mojibake-test -f east_asian_width)
add_test(NAME "Emoji" COMMAND mojibake-test -f emoji)
add_test(NAME "Encoding" COMMAND mojibake-test -f encoding)
add_test(NAME "Filter" COMMAND mojibake-test -f filter)
add_test(NAME "Hangul Composition" COMMAND mojibake-test -f hangul_composition)
add_test(NAME "Hangul" COMMAND mojibake-test -f hangul)
add_test(NAME "Mojibake" COMMAND mojibake-test -f mojibake)
add_test(NAME "Next" COMMAND mojibake-test -f next)
add_test(NAME "Normalization" COMMAND mojibake-test -f normalization)
add_test(NAME "Plane" COMMAND mojibake-test -f plane)
add_test(NAME "Quick Check" COMMAND mojibake-test -f quick_check)
add_test(NAME "Segmentation" COMMAND mojibake-test -f segmentation)
add_test(NAME "Special Case" COMMAND mojibake-test -f special_case)
add_test(NAME "String" COMMAND mojibake-test -f string)
add_test(NAME "Version" COMMAND mojibake-test -f version)
