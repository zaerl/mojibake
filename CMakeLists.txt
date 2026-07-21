# The Mojibake library
#
# This file is distributed under the MIT License. See LICENSE for details.

cmake_minimum_required(VERSION 3.14...3.31)

project(
    Mojibake
    VERSION 0.3.2
    DESCRIPTION "Mojibake"
    HOMEPAGE_URL "https://mojibake.zaerl.com"
    LANGUAGES C
)

include(GNUInstallDirs)

# PROJECT_IS_TOP_LEVEL was added in CMake 3.21.
if(CMAKE_VERSION VERSION_LESS 3.21)
    if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
        set(PROJECT_IS_TOP_LEVEL ON)
    else()
        set(PROJECT_IS_TOP_LEVEL OFF)
    endif()
endif()

option(MJB_BUILD_CPP "Build Mojibake sources with C++ instead of C" OFF)
option(MJB_BUILD_WASM "Build Mojibake for WebAssembly" OFF)
option(MJB_USE_ASAN "Enable AddressSanitizer for Mojibake targets" OFF)
option(MJB_USE_UBSAN "Enable UndefinedBehaviorSanitizer for Mojibake targets" OFF)
option(MJB_ALLOW_EMBEDDED_NULLS "Enable embedded NULL support" OFF)
option(MJB_BUILD_CLI "Build the Mojibake command-line interface" ${PROJECT_IS_TOP_LEVEL})
option(MJB_BUILD_TESTS "Build Mojibake tests" ${PROJECT_IS_TOP_LEVEL})
option(MJB_FEATURE_CHARACTER_NAMES "Compile Unicode character name lookup tables" ON)
option(MJB_INSTALL "Generate Mojibake installation rules" ${PROJECT_IS_TOP_LEVEL})

option(BUILD_SHARED_LIBS "Build libraries as shared libraries" OFF)

# Used by IDEs and language servers for IntelliSense
if(PROJECT_IS_TOP_LEVEL)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

if(MJB_BUILD_CPP)
    enable_language(CXX)
endif()

function(mjb_configure_target target)
    get_target_property(_mjb_target_type ${target} TYPE)
    if(_mjb_target_type MATCHES "^(STATIC|SHARED|MODULE)_LIBRARY$")
        set(_mjb_sanitizer_link_scope PUBLIC)
    else()
        set(_mjb_sanitizer_link_scope PRIVATE)
    endif()

    set_target_properties(${target} PROPERTIES C_EXTENSIONS OFF)

    if(MJB_BUILD_CPP)
        target_compile_features(${target} PRIVATE cxx_std_17)
    endif()

    if(MSVC)
        target_compile_options(
            ${target}
            PRIVATE
                /W4
                /wd4100
                $<$<OR:$<CONFIG:Debug>,$<CONFIG:Test>>:/Zi>
                $<$<OR:$<CONFIG:Debug>,$<CONFIG:Test>>:/Od>
                $<$<CONFIG:Release>:/O2>
        )

        if(MJB_BUILD_CPP)
            target_compile_options(${target} PRIVATE /Zc:__cplusplus)
        endif()
    else()
        target_compile_options(
            ${target}
            PRIVATE
                -Wall
                -Wextra
                -pedantic
                -Wno-unused-parameter
                -Wtype-limits
                $<$<OR:$<CONFIG:Debug>,$<CONFIG:Test>>:-g>
                $<$<OR:$<CONFIG:Debug>,$<CONFIG:Test>>:-O0>
                $<$<CONFIG:Release>:-O3>
        )

        if(MJB_USE_ASAN)
            target_compile_options(${target} PRIVATE -fsanitize=address -fno-omit-frame-pointer)
            target_link_options(${target} ${_mjb_sanitizer_link_scope} -fsanitize=address)
        endif()

        if(MJB_USE_UBSAN)
            target_compile_options(
                ${target}
                PRIVATE
                    -fsanitize=undefined
                    -fno-sanitize-recover=undefined
                    -fno-omit-frame-pointer
            )
            target_link_options(${target} ${_mjb_sanitizer_link_scope} -fsanitize=undefined)
        endif()
    endif()

    target_compile_definitions(${target} PRIVATE $<$<CONFIG:Test>:MJB_TEST>)
endfunction()

# Mojibake library
add_subdirectory(src)

if(MJB_INSTALL AND NOT MJB_BUILD_WASM)
    include(CMakePackageConfigHelpers)

    set(MJB_INSTALL_CMAKEDIR
        "${CMAKE_INSTALL_LIBDIR}/cmake/Mojibake"
        CACHE STRING "CMake package configuration install directory"
    )

    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/MojibakeConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/MojibakeConfig.cmake"
        INSTALL_DESTINATION "${MJB_INSTALL_CMAKEDIR}"
    )

    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/MojibakeConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    install(
        EXPORT MojibakeTargets
        NAMESPACE Mojibake::
        DESTINATION "${MJB_INSTALL_CMAKEDIR}"
    )

    install(
        FILES
            "${CMAKE_CURRENT_BINARY_DIR}/MojibakeConfig.cmake"
            "${CMAKE_CURRENT_BINARY_DIR}/MojibakeConfigVersion.cmake"
        DESTINATION "${MJB_INSTALL_CMAKEDIR}"
    )
endif()

# Tests (only for native builds when requested)
if(MJB_BUILD_TESTS AND NOT MJB_BUILD_WASM)
    enable_testing()
    add_subdirectory(tests)
endif()

# Shell (only for native builds when requested)
if(MJB_BUILD_CLI AND NOT MJB_BUILD_WASM)
    add_subdirectory(src/shell)
endif()
