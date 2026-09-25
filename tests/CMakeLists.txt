# The Mojibake library
#
# This file is distributed under the MIT License. See LICENSE for details.

set(TEST_SOURCES
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
    idna.c
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
    terminal-width.c
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

# Add a CTest entry that runs a single mojibake-test filter. The DEF_SOURCE_LINE
# property points editors (VSCode CMake Tools) back at the call site instead of the
# first source of the mojibake-test target.
function(mjb_add_test name filter line)
    add_test(NAME "${name}" COMMAND mojibake-test -f ${filter})
    set_tests_properties("${name}" PROPERTIES DEF_SOURCE_LINE "${CMAKE_CURRENT_LIST_FILE}:${line}")
endfunction()

# Add test to CTest
mjb_add_test("Bidi" bidi ${CMAKE_CURRENT_LIST_LINE})
mjb_add_test("Bidi Class" bidi_class ${CMAKE_CURRENT_LIST_LINE})
mjb_add_test("Break Line" break_line ${CMAKE_CURRENT_LIST_LINE})
mjb_add_test("Break Sentence" break_sentence ${CMAKE_CURRENT_LIST_LINE})
mjb_add_test("Break Word" break_word ${CMAKE_CURRENT_LIST_LINE})
mjb_add_test("Case" case ${CMAKE_CURRENT_LIST_LINE})
mjb_add_test("Caseless" caseless ${CMAKE_CURRENT_LIST_LINE})
mjb_add_test("CJK" cjk ${CMAKE_CURRENT_LIST_LINE})
mjb_add_test("Codepoint" codepoint ${CMAKE_CURRENT_LIST_LINE})
mjb_add_test("Collation" collation ${CMAKE_CURRENT_LIST_LINE})
mjb_add_test("Terminal Width" terminal_width ${CMAKE_CURRENT_LIST_LINE})
mjb_add_test("East Asian Width" east_asian_width ${CMAKE_CURRENT_LIST_LINE})
mjb_add_test("Embedded NULL" embedded_null ${CMAKE_CURRENT_LIST_LINE})
mjb_add_test("Emoji" emoji ${CMAKE_CURRENT_LIST_LINE})
mjb_add_test("Encoding" encoding ${CMAKE_CURRENT_LIST_LINE})
mjb_add_test("Example" example ${CMAKE_CURRENT_LIST_LINE})
mjb_add_test("Filter" filter ${CMAKE_CURRENT_LIST_LINE})
mjb_add_test("Format" format ${CMAKE_CURRENT_LIST_LINE})
mjb_add_test("Hangul Composition" hangul_composition ${CMAKE_CURRENT_LIST_LINE})
mjb_add_test("Hangul" hangul ${CMAKE_CURRENT_LIST_LINE})
mjb_add_test("Identifier" identifier ${CMAKE_CURRENT_LIST_LINE})
mjb_add_test("IDNA" idna ${CMAKE_CURRENT_LIST_LINE})
mjb_add_test("Locales" locales ${CMAKE_CURRENT_LIST_LINE})
mjb_add_test("Mojibake" mojibake ${CMAKE_CURRENT_LIST_LINE})
mjb_add_test("Next" next ${CMAKE_CURRENT_LIST_LINE})
mjb_add_test("Normalization" normalization ${CMAKE_CURRENT_LIST_LINE})
mjb_add_test("Plane" plane ${CMAKE_CURRENT_LIST_LINE})
mjb_add_test("Properties" properties ${CMAKE_CURRENT_LIST_LINE})
mjb_add_test("Quick Check" quick_check ${CMAKE_CURRENT_LIST_LINE})
mjb_add_test("Security" security ${CMAKE_CURRENT_LIST_LINE})
mjb_add_test("Segmentation" segmentation ${CMAKE_CURRENT_LIST_LINE})
mjb_add_test("Special Case" special_case ${CMAKE_CURRENT_LIST_LINE})
mjb_add_test("String" string ${CMAKE_CURRENT_LIST_LINE})
mjb_add_test("UTF" utf ${CMAKE_CURRENT_LIST_LINE})
mjb_add_test("Version" version ${CMAKE_CURRENT_LIST_LINE})

if(MJB_BUILD_CPP)
    mjb_add_test("C++ Break" cpp_break ${CMAKE_CURRENT_LIST_LINE})
    mjb_add_test("C++ Mojibake" cpp_mojibake ${CMAKE_CURRENT_LIST_LINE})
    mjb_add_test("C++ Normalization" cpp_normalization ${CMAKE_CURRENT_LIST_LINE})
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
