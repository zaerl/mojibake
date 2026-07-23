# The Mojibake library
#
# This file is distributed under the MIT License. See LICENSE for details.

set(TEST_SOURCES
    attractor/attractor.c
    utils/utils.c
    bidi.c
    bidi-class.c
    break-line.c
    break-sentence.c
    break-word.c
    case.c
    caseless.c
    cjk.c
    codepoint.c
    collation.c
    display.c
    emoji.c
    encoding.c
    east-asian-width.c
    embedded-null.c
    example.c
    filter.c
    format.c
    hangul-composition.c
    hangul.c
    identifier.c
    locales.c
    mojibake.c
    next.c
    normalization.c
    plane.c
    properties.c
    quick-check.c
    security.c
    segmentation.c
    special-case.c
    string.c
    test.c
    utf.c
    version.c
)

if(MJB_BUILD_CPP)
    # Add C++ test sources
    list(APPEND TEST_SOURCES ext/cpp/break.cpp)
    list(APPEND TEST_SOURCES ext/cpp/locales.cpp)
    list(APPEND TEST_SOURCES ext/cpp/mojibake.cpp)
    list(APPEND TEST_SOURCES ext/cpp/normalization.cpp)
endif()

# Add getopt compatibility for Windows
if(WIN32)
    list(APPEND TEST_SOURCES ../src/shell/getopt/getopt.c)
endif()

add_executable(mojibake-test ${TEST_SOURCES})

target_link_libraries(mojibake-test PRIVATE mojibake_lib)
mjb_configure_target(mojibake-test)
target_compile_definitions(mojibake-test PRIVATE MJB_TEST_SOURCE_DIR="${Mojibake_SOURCE_DIR}")

# The tests use strdup and strnlen, which strict C11 hides on glibc/musl.
target_compile_definitions(mojibake-test PRIVATE _DEFAULT_SOURCE)

# Force C++ compilation if MJB_BUILD_CPP is enabled
if(MJB_BUILD_CPP)
    set_target_properties(mojibake-test PROPERTIES
        LINKER_LANGUAGE CXX
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
    )
    set_source_files_properties(${TEST_SOURCES} PROPERTIES LANGUAGE CXX)
endif()

if(MSVC)
    # Suppress warnings from the Microsoft compiler
    target_compile_options(mojibake-test
        PRIVATE
        /wd4996 # Suppress 'the POSIX name for this item is deprecated'
        /wd4245 # Suppress 'signed/unsigned mismatch'
        /wd4702 # Suppress 'unreachable code'
        /wd4701 # Suppress 'potentially uninitialized local variable'
    )
endif()

# Add test to CTest
add_test(NAME "Bidi" COMMAND mojibake-test -f bidi)
add_test(NAME "Bidi Class" COMMAND mojibake-test -f bidi_class)
add_test(NAME "Break Line" COMMAND mojibake-test -f break_line)
add_test(NAME "Break Sentence" COMMAND mojibake-test -f break_sentence)
add_test(NAME "Break Word" COMMAND mojibake-test -f break_word)
add_test(NAME "Case" COMMAND mojibake-test -f case)
add_test(NAME "Caseless" COMMAND mojibake-test -f caseless)
add_test(NAME "CJK" COMMAND mojibake-test -f cjk)
add_test(NAME "Codepoint" COMMAND mojibake-test -f codepoint)
add_test(NAME "Collation" COMMAND mojibake-test -f collation)
add_test(NAME "Display" COMMAND mojibake-test -f display)
add_test(NAME "East Asian Width" COMMAND mojibake-test -f east_asian_width)
add_test(NAME "Embedded NULL" COMMAND mojibake-test -f embedded_null)
add_test(NAME "Emoji" COMMAND mojibake-test -f emoji)
add_test(NAME "Encoding" COMMAND mojibake-test -f encoding)
add_test(NAME "Example" COMMAND mojibake-test -f example)
add_test(NAME "Filter" COMMAND mojibake-test -f filter)
add_test(NAME "Format" COMMAND mojibake-test -f format)
add_test(NAME "Hangul Composition" COMMAND mojibake-test -f hangul_composition)
add_test(NAME "Hangul" COMMAND mojibake-test -f hangul)
add_test(NAME "Identifier" COMMAND mojibake-test -f identifier)
add_test(NAME "Locales" COMMAND mojibake-test -f locales)
add_test(NAME "Mojibake" COMMAND mojibake-test -f mojibake)
add_test(NAME "Next" COMMAND mojibake-test -f next)
add_test(NAME "Normalization" COMMAND mojibake-test -f normalization)
add_test(NAME "Plane" COMMAND mojibake-test -f plane)
add_test(NAME "Properties" COMMAND mojibake-test -f properties)
add_test(NAME "Quick Check" COMMAND mojibake-test -f quick_check)
add_test(NAME "Security" COMMAND mojibake-test -f security)
add_test(NAME "Segmentation" COMMAND mojibake-test -f segmentation)
add_test(NAME "Special Case" COMMAND mojibake-test -f special_case)
add_test(NAME "String" COMMAND mojibake-test -f string)
add_test(NAME "UTF" COMMAND mojibake-test -f utf)
add_test(NAME "Version" COMMAND mojibake-test -f version)

if(MJB_BUILD_CPP)
    add_test(NAME "C++ Break" COMMAND mojibake-test -f cpp_break)
    add_test(NAME "C++ Mojibake" COMMAND mojibake-test -f cpp_mojibake)
    add_test(NAME "C++ Normalization" COMMAND mojibake-test -f cpp_normalization)
endif()

set(MJB_CMAKE_TEST_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/cmake/run-embedded-test.cmake")

add_test(
    NAME "CMake add_subdirectory dependency"
    COMMAND
        ${CMAKE_COMMAND}
        "-DMJB_SOURCE_DIR=${Mojibake_SOURCE_DIR}"
        "-DMJB_BINARY_ROOT=${CMAKE_CURRENT_BINARY_DIR}/cmake"
        "-DMJB_GENERATOR=${CMAKE_GENERATOR}"
        "-DMJB_GENERATOR_PLATFORM=${CMAKE_GENERATOR_PLATFORM}"
        "-DMJB_GENERATOR_TOOLSET=${CMAKE_GENERATOR_TOOLSET}"
        -DMJB_USE_FETCHCONTENT=OFF
        -DMJB_EXPECT_SHARED=OFF
        -DMJB_EXPECT_AUX_TARGETS=OFF
        -P "${MJB_CMAKE_TEST_SCRIPT}"
)

add_test(
    NAME "CMake FetchContent dependency"
    COMMAND
        ${CMAKE_COMMAND}
        "-DMJB_SOURCE_DIR=${Mojibake_SOURCE_DIR}"
        "-DMJB_BINARY_ROOT=${CMAKE_CURRENT_BINARY_DIR}/cmake"
        "-DMJB_GENERATOR=${CMAKE_GENERATOR}"
        "-DMJB_GENERATOR_PLATFORM=${CMAKE_GENERATOR_PLATFORM}"
        "-DMJB_GENERATOR_TOOLSET=${CMAKE_GENERATOR_TOOLSET}"
        -DMJB_USE_FETCHCONTENT=ON
        -DMJB_EXPECT_SHARED=ON
        -DMJB_EXPECT_AUX_TARGETS=OFF
        -P "${MJB_CMAKE_TEST_SCRIPT}"
)

add_test(
    NAME "CMake embedded opt-in targets"
    COMMAND
        ${CMAKE_COMMAND}
        "-DMJB_SOURCE_DIR=${Mojibake_SOURCE_DIR}"
        "-DMJB_BINARY_ROOT=${CMAKE_CURRENT_BINARY_DIR}/cmake"
        "-DMJB_GENERATOR=${CMAKE_GENERATOR}"
        "-DMJB_GENERATOR_PLATFORM=${CMAKE_GENERATOR_PLATFORM}"
        "-DMJB_GENERATOR_TOOLSET=${CMAKE_GENERATOR_TOOLSET}"
        -DMJB_USE_FETCHCONTENT=OFF
        -DMJB_EXPECT_SHARED=OFF
        -DMJB_EXPECT_AUX_TARGETS=ON
        -P "${MJB_CMAKE_TEST_SCRIPT}"
)
