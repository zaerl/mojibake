# The Mojibake library
#
# This file is distributed under the MIT License. See LICENSE for details.

# Run the CLI given by MJB_SHELL and compare its exit status, standard output, and standard error.
#
# mjb_assert_shell_command(ARGS <arg>... OUTPUT <stdout> [STATUS <code>] [ERROR <stderr>])
#
# STATUS defaults to 0. Standard error is compared only when ERROR is given.
if(NOT DEFINED MJB_SHELL)
    message(FATAL_ERROR "MJB_SHELL is not set")
endif()

function(mjb_assert_shell_command)
    cmake_parse_arguments(PARSE_ARGV 0 MJB "" "OUTPUT;STATUS;ERROR" "ARGS")

    if(NOT DEFINED MJB_STATUS)
        set(MJB_STATUS 0)
    endif()

    execute_process(
        COMMAND "${MJB_SHELL}" ${MJB_ARGS}
        RESULT_VARIABLE status
        OUTPUT_VARIABLE output
        ERROR_VARIABLE error
    )

    string(REPLACE ";" " " command "${MJB_ARGS}")

    if(NOT status STREQUAL MJB_STATUS)
        message(SEND_ERROR "mojibake ${command}: expected status ${MJB_STATUS}, got ${status}")
    endif()

    if(NOT output STREQUAL MJB_OUTPUT)
        message(SEND_ERROR "mojibake ${command}: expected output\n${MJB_OUTPUT}\ngot\n${output}")
    endif()

    if(DEFINED MJB_ERROR AND NOT error STREQUAL MJB_ERROR)
        message(SEND_ERROR "mojibake ${command}: expected error\n${MJB_ERROR}\ngot\n${error}")
    endif()
endfunction()
