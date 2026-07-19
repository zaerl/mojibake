# The Mojibake library
#
# This file is distributed under the MIT License. See LICENSE for details.

cmake_minimum_required(VERSION 3.14)

project(mojibake-embedded-consumer LANGUAGES C)

set(CMAKE_C_FLAGS "-DMJB_PARENT_FLAGS_PRESERVED")
set(CMAKE_C_FLAGS_DEBUG "-DMJB_PARENT_DEBUG_FLAGS_PRESERVED")
set(CMAKE_C_FLAGS_RELEASE "-DMJB_PARENT_RELEASE_FLAGS_PRESERVED")
set(CMAKE_BUILD_TYPE ParentConfig)
set(CMAKE_CONFIGURATION_TYPES "ParentOne;ParentTwo")

if(MJB_EXPECT_AUX_TARGETS)
    set(MJB_BUILD_TESTS ON)
    set(MJB_BUILD_CLI ON)
endif()

if(MJB_USE_FETCHCONTENT)
    include(FetchContent)
    FetchContent_Declare(Mojibake SOURCE_DIR "${MJB_SOURCE_UNDER_TEST}")
    FetchContent_MakeAvailable(Mojibake)
else()
    add_subdirectory("${MJB_SOURCE_UNDER_TEST}" mojibake)
endif()

if(NOT CMAKE_C_FLAGS STREQUAL "-DMJB_PARENT_FLAGS_PRESERVED")
    message(FATAL_ERROR "Mojibake changed the parent CMAKE_C_FLAGS")
endif()

if(NOT CMAKE_C_FLAGS_DEBUG STREQUAL "-DMJB_PARENT_DEBUG_FLAGS_PRESERVED")
    message(FATAL_ERROR "Mojibake changed the parent CMAKE_C_FLAGS_DEBUG")
endif()

if(NOT CMAKE_C_FLAGS_RELEASE STREQUAL "-DMJB_PARENT_RELEASE_FLAGS_PRESERVED")
    message(FATAL_ERROR "Mojibake changed the parent CMAKE_C_FLAGS_RELEASE")
endif()

if(NOT CMAKE_BUILD_TYPE STREQUAL "ParentConfig")
    message(FATAL_ERROR "Mojibake changed the parent CMAKE_BUILD_TYPE")
endif()

if(NOT CMAKE_CONFIGURATION_TYPES STREQUAL "ParentOne;ParentTwo")
    message(FATAL_ERROR "Mojibake changed the parent CMAKE_CONFIGURATION_TYPES")
endif()

if(MJB_INSTALL)
    message(FATAL_ERROR "Mojibake installation rules defaulted on in an embedded build")
endif()

get_target_property(_mjb_library_type mojibake_lib TYPE)
if(MJB_EXPECT_SHARED)
    if(NOT _mjb_library_type STREQUAL "SHARED_LIBRARY")
        message(FATAL_ERROR "BUILD_SHARED_LIBS=ON did not create a shared Mojibake library")
    endif()
elseif(NOT _mjb_library_type STREQUAL "STATIC_LIBRARY")
    message(FATAL_ERROR "BUILD_SHARED_LIBS=OFF did not create a static Mojibake library")
endif()

get_target_property(_mjb_compile_features mojibake_lib INTERFACE_COMPILE_FEATURES)
if(NOT c_std_11 IN_LIST _mjb_compile_features)
    message(FATAL_ERROR "Mojibake does not publish its C11 compile requirement")
endif()

if(MJB_EXPECT_AUX_TARGETS)
    if(NOT TARGET mojibake-test OR NOT TARGET mojibake)
        message(FATAL_ERROR "Explicitly requested Mojibake tests or CLI target is missing")
    endif()

    get_target_property(_mjb_test_definitions mojibake-test COMPILE_DEFINITIONS)
    set(_mjb_expected_source_definition "MJB_TEST_SOURCE_DIR=\"${MJB_SOURCE_UNDER_TEST}\"")
    if(NOT _mjb_expected_source_definition IN_LIST _mjb_test_definitions)
        message(FATAL_ERROR "Embedded tests use the wrong Mojibake source directory")
    endif()
else()
    if(TARGET mojibake-test OR TARGET mojibake)
        message(FATAL_ERROR "Mojibake added tests or CLI targets without an explicit request")
    endif()
endif()

add_executable(embedded-consumer main.c)
target_link_libraries(embedded-consumer PRIVATE Mojibake::mojibake)
