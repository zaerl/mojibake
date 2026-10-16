# The Mojibake library
#
# This file is distributed under the MIT License. See LICENSE for details.

include("${CMAKE_CURRENT_LIST_DIR}/assert-command.cmake")

# The segments are printed from the input bytes, so characters that a rule absorbs into the
# previous one (SB5 and WB4 for format and extend characters) appear exactly once.
string(ASCII 226 128 139 ZWSP)
string(ASCII 204 129 ACUTE)
string(ASCII 195 183 BREAK)
string(ASCII 195 151 NO_BREAK)
# The CLI shows a space as U+2423 OPEN BOX.
string(ASCII 226 144 163 SPACE)

mjb_assert_shell_command(
    ARGS break sentence "a${ZWSP}b"
    OUTPUT
        "Raw input size: 5\nReal input size: 3\nTerminal width: 2\n\nRaw bytes: a<E2><80><8B>b\n\nSentence breaking:\n${BREAK}a${ZWSP}b${BREAK}"
)

mjb_assert_shell_command(
    ARGS --show-allowed-symbols break sentence "a${ZWSP}b"
    OUTPUT
        "Raw input size: 5\nReal input size: 3\nTerminal width: 2\n\nRaw bytes: a<E2><80><8B>b\n\nSentence breaking:\n${BREAK}a${NO_BREAK}${ZWSP}${NO_BREAK}b${BREAK}"
)

mjb_assert_shell_command(
    ARGS break word "a${ZWSP}b"
    OUTPUT
        "Raw input size: 5\nReal input size: 3\nTerminal width: 2\n\nRaw bytes: a<E2><80><8B>b\n\nWord breaking:\n${BREAK}a${BREAK}${ZWSP}${BREAK}b${BREAK}"
)

mjb_assert_shell_command(
    ARGS break all "e${ACUTE}x"
    OUTPUT
        "Raw input size: 4\nReal input size: 3\nTerminal width: 2\n\nRaw bytes: e<CC><81>x\n\nGrapheme cluster segmentation:\n${BREAK}e${ACUTE}${BREAK}x${BREAK}\n\nWord breaking:\n${BREAK}e${ACUTE}x${BREAK}\n\nLine breaking:\ne${ACUTE}x!\n\nSentence breaking:\n${BREAK}e${ACUTE}x${BREAK}"
)

mjb_assert_shell_command(
    ARGS --show-allowed-symbols break line "a b"
    OUTPUT
        "Raw input size: 3\nReal input size: 3\nTerminal width: 3\n\nRaw bytes: a<20>b\n\nLine breaking:\n${NO_BREAK}a${NO_BREAK}${SPACE}${BREAK}b!"
)
