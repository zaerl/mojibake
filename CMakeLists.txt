cmake_minimum_required(VERSION 3.14...3.31)

# The Mojibake library
#
# This file is distributed under the MIT License. See LICENSE for details.

# Option to build with C++
option(BUILD_CPP "Build with C++ instead of C" OFF)

project(
    Mojibake
    VERSION 0.2.8
    DESCRIPTION "Mojibake"
    HOMEPAGE_URL "https://mojibake.zaerl.com"
    LANGUAGES C
)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Used by IDEs and language servers for IntelliSense
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Enable CTest
enable_testing()

# WASM build option
option(BUILD_WASM "Build for WebAssembly" OFF)

# AddressSanitizer option
option(USE_ASAN "Enable AddressSanitizer" OFF)

# UndefinedBehaviorSanitizer option
option(USE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)

# Shared library option
option(BUILD_SHARED "Build as shared library instead of static" OFF)

# Character name table option
option(MJB_FEATURE_CHARACTER_NAMES "Compile Unicode character name lookup tables" ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Common compiler flags
if(MSVC)
    # MSVC-specific flags
    set(COMMON_FLAGS "/W4 /wd4100")
    set(COMMON_FLAGS_DEBUG "/Zi /Od")
    set(COMMON_FLAGS_RELEASE "/O2")
    set(COMMON_FLAGS_TEST "/Zi /Od /DMJB_TEST")
else()
    # GCC/Clang flags
    set(COMMON_FLAGS "-Wall -Wextra -pedantic -Wno-unused-parameter -Wtype-limits")
    set(COMMON_FLAGS_DEBUG "-g -O0")
    set(COMMON_FLAGS_RELEASE "-O3")
    set(COMMON_FLAGS_TEST "-g -O0 -DMJB_TEST")
endif()

# AddressSanitizer flags (GCC/Clang only)
if(USE_ASAN AND NOT MSVC)
    set(ASAN_FLAGS "-fsanitize=address -fno-omit-frame-pointer")
    set(COMMON_FLAGS "${COMMON_FLAGS} ${ASAN_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=address")
endif()

# UndefinedBehaviorSanitizer flags (GCC/Clang only)
if(USE_UBSAN AND NOT MSVC)
    set(UBSAN_FLAGS "-fsanitize=undefined -fno-sanitize-recover=undefined -fno-omit-frame-pointer")
    set(COMMON_FLAGS "${COMMON_FLAGS} ${UBSAN_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=undefined")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=undefined")
endif()

# Language standards
if(BUILD_CPP)
    enable_language(CXX)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_FLAGS_DEBUG ${COMMON_FLAGS_DEBUG})
    set(CMAKE_CXX_FLAGS_RELEASE ${COMMON_FLAGS_RELEASE})
    set(CMAKE_CXX_FLAGS_TEST ${COMMON_FLAGS_TEST})

    if(MSVC)
        set(CMAKE_CXX_FLAGS "${COMMON_FLAGS} /Zc:__cplusplus")
    else()
        set(CMAKE_CXX_FLAGS ${COMMON_FLAGS})
    endif()
else()
    # The code itself targets C11 (see mojibake.h).
    set(CMAKE_C_STANDARD 17)
    set(CMAKE_C_STANDARD_REQUIRED ON)
    set(CMAKE_C_EXTENSIONS OFF)
    set(CMAKE_C_FLAGS ${COMMON_FLAGS})
    set(CMAKE_C_FLAGS_DEBUG ${COMMON_FLAGS_DEBUG})
    set(CMAKE_C_FLAGS_RELEASE ${COMMON_FLAGS_RELEASE})
    set(CMAKE_C_FLAGS_TEST ${COMMON_FLAGS_TEST})
endif()

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Test" CACHE STRING "Configurations" FORCE)

# Mojibake library
add_subdirectory(src)

if(NOT BUILD_WASM)
    set(MOJIBAKE_INSTALL_CMAKEDIR
        "${CMAKE_INSTALL_LIBDIR}/cmake/Mojibake"
        CACHE STRING "CMake package configuration install directory"
    )

    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/MojibakeConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/MojibakeConfig.cmake"
        INSTALL_DESTINATION "${MOJIBAKE_INSTALL_CMAKEDIR}"
    )

    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/MojibakeConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    install(
        EXPORT MojibakeTargets
        NAMESPACE Mojibake::
        DESTINATION "${MOJIBAKE_INSTALL_CMAKEDIR}"
    )

    install(
        FILES
            "${CMAKE_CURRENT_BINARY_DIR}/MojibakeConfig.cmake"
            "${CMAKE_CURRENT_BINARY_DIR}/MojibakeConfigVersion.cmake"
        DESTINATION "${MOJIBAKE_INSTALL_CMAKEDIR}"
    )
endif()

# Tests and shell (only for native builds)
if(NOT BUILD_WASM)
    add_subdirectory(tests)
    add_subdirectory(src/shell)
endif()
