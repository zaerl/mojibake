# The Mojibake library
#
# This file is distributed under the MIT License. See LICENSE for details.

include("${CMAKE_CURRENT_LIST_DIR}/assert-command.cmake")

mjb_assert_shell_command(
    ARGS --output json break all "a b"
    OUTPUT
        "{\"raw_input_size\":3,\"real_input_size\":3,\"display_width\":3,\"raw_bytes\":[97,32,98],\"grapheme_breaks\":[\"allowed\",\"allowed\",\"allowed\"],\"word_breaks\":[\"allowed\",\"allowed\",\"allowed\"],\"line_breaks\":[\"no_break\",\"allowed\",\"mandatory\"],\"sentence_breaks\":[\"no_break\",\"no_break\",\"allowed\"]}\n"
)

mjb_assert_shell_command(
    ARGS --output json --json-indent 2 break line "a b"
    OUTPUT
        "{\n  \"raw_input_size\": 3,\n  \"real_input_size\": 3,\n  \"display_width\": 3,\n  \"raw_bytes\": [97, 32, 98],\n  \"line_breaks\": [\"no_break\", \"allowed\", \"mandatory\"]\n}\n"
)

mjb_assert_shell_command(
    ARGS --output json break
    STATUS 1
    ERROR "break: JSON output requires an input\n"
    OUTPUT ""
)
