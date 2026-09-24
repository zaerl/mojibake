# The Mojibake library
#
# This file is distributed under the MIT License. See LICENSE for details.

cmake_minimum_required(VERSION 3.14...3.31)

project(
    MojibakeGenerator
    DESCRIPTION "Mojibake source generator"
    LANGUAGES CXX
)

option(MJB_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

add_executable(
    generator
    generator.cpp
    update-version.cpp
    utils.cpp
)

target_compile_features(generator PRIVATE cxx_std_17)
set_target_properties(generator PROPERTIES CXX_EXTENSIONS OFF)

if(MSVC)
    target_compile_options(generator PRIVATE /W4)

    if(MJB_WARNINGS_AS_ERRORS)
        target_compile_options(generator PRIVATE /WX)
    endif()
else()
    target_compile_options(generator PRIVATE -Wall -Wextra -Wpedantic)

    if(MJB_WARNINGS_AS_ERRORS)
        target_compile_options(generator PRIVATE -Werror)
    endif()
endif()
