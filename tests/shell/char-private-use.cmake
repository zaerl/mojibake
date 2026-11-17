# The Mojibake library
#
# This file is distributed under the MIT License. See LICENSE for details.

include("${CMAKE_CURRENT_LIST_DIR}/assert-command.cmake")

# Private-use characters are <Private Use, First>/<..., Last> ranges in UnicodeData.txt: they have
# no name and the General_Category Co.
string(ASCII 238 128 128 PRIVATE_USE)
string(ASCII 243 176 128 128 SUPPLEMENTARY_PRIVATE_USE_A)

mjb_assert_shell_command(
    ARGS char "${PRIVATE_USE}"
    OUTPUT
        "Codepoint: U+E000\nName: \nCharacter: ${PRIVATE_USE}\nHex UTF-8: EE 80 80\nCategory: [29] Other, private use\n"
)

mjb_assert_shell_command(
    ARGS codepoint E000
    OUTPUT
        "Codepoint: U+E000\nName: \nCharacter: ${PRIVATE_USE}\nHex UTF-8: EE 80 80\nCategory: [29] Other, private use\n"
)

mjb_assert_shell_command(
    ARGS -o json char "${SUPPLEMENTARY_PRIVATE_USE_A}"
    OUTPUT
        "[{\"codepoint\":\"U+F0000\",\"name\":\"\",\"character\":\"${SUPPLEMENTARY_PRIVATE_USE_A}\",\"hex_utf_8\":[243, 176, 128, 128],\"category\":{\"code\":29,\"value\":\"Other, private use\"}}]"
)
