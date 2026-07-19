# The Mojibake library
#
# This file is distributed under the MIT License. See LICENSE for details.

if(MJB_USE_FETCHCONTENT)
    set(_mjb_test_name fetchcontent)
else()
    set(_mjb_test_name add-subdirectory)
endif()

if(MJB_EXPECT_AUX_TARGETS)
    string(APPEND _mjb_test_name -with-targets)
endif()

set(_mjb_build_dir "${MJB_BINARY_ROOT}/${_mjb_test_name}")
file(REMOVE_RECURSE "${_mjb_build_dir}")

set(
    _mjb_configure_command
    "${CMAKE_COMMAND}"
    -S "${MJB_SOURCE_DIR}/tests/cmake/embedded-project"
    -B "${_mjb_build_dir}"
    -G "${MJB_GENERATOR}"
    "-DMJB_SOURCE_UNDER_TEST=${MJB_SOURCE_DIR}"
    "-DMJB_USE_FETCHCONTENT=${MJB_USE_FETCHCONTENT}"
    "-DMJB_EXPECT_SHARED=${MJB_EXPECT_SHARED}"
    "-DMJB_EXPECT_AUX_TARGETS=${MJB_EXPECT_AUX_TARGETS}"
    "-DBUILD_SHARED_LIBS=${MJB_EXPECT_SHARED}"
)

if(MJB_GENERATOR_PLATFORM)
    list(APPEND _mjb_configure_command -A "${MJB_GENERATOR_PLATFORM}")
endif()

if(MJB_GENERATOR_TOOLSET)
    list(APPEND _mjb_configure_command -T "${MJB_GENERATOR_TOOLSET}")
endif()

execute_process(
    COMMAND ${_mjb_configure_command}
    RESULT_VARIABLE _mjb_configure_result
    OUTPUT_VARIABLE _mjb_configure_stdout
    ERROR_VARIABLE _mjb_configure_stderr
)

if(NOT _mjb_configure_result EQUAL 0)
    message(
        FATAL_ERROR
        "Embedded configure failed:\n${_mjb_configure_stdout}\n${_mjb_configure_stderr}"
    )
endif()

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" --build "${_mjb_build_dir}" --config ParentOne
        --target embedded-consumer
    RESULT_VARIABLE _mjb_build_result
    OUTPUT_VARIABLE _mjb_build_stdout
    ERROR_VARIABLE _mjb_build_stderr
)

if(NOT _mjb_build_result EQUAL 0)
    message(FATAL_ERROR "Embedded build failed:\n${_mjb_build_stdout}\n${_mjb_build_stderr}")
endif()
