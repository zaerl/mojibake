cmake_minimum_required(VERSION 3.14...3.30)

# Option to build with C++
option(BUILD_CPP "Build with C++ instead of C" OFF)

project(
    Mojibake
    VERSION 0.0.6
    DESCRIPTION "Mojibake"
    LANGUAGES C CXX
)

# Enable CTest
enable_testing()

# WASM build option
option(BUILD_WASM "Build for WebAssembly" OFF)

# AddressSanitizer option
option(USE_ASAN "Enable AddressSanitizer" OFF)

# Embedded database option
option(USE_EMBEDDED_DB "Use embedded database instead of external file" OFF)

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

# Language standards
if(BUILD_CPP)
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
    set(CMAKE_C_STANDARD 17)
    set(CMAKE_C_STANDARD_REQUIRED ON)
    set(CMAKE_C_FLAGS ${COMMON_FLAGS})
    set(CMAKE_C_FLAGS_DEBUG ${COMMON_FLAGS_DEBUG})
    set(CMAKE_C_FLAGS_RELEASE ${COMMON_FLAGS_RELEASE})
    set(CMAKE_C_FLAGS_TEST ${COMMON_FLAGS_TEST})
endif()

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Test" CACHE STRING "Configurations" FORCE)

# Mojibake library
add_subdirectory(src)

# SQLite library
add_subdirectory(src/sqlite3)

# Tests and shell (only for native builds)
if(NOT BUILD_WASM)
    add_subdirectory(tests)
    add_subdirectory(src/shell)
endif()
