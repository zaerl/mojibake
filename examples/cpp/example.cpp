/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "../../src/cpp/mojibake.hpp"

#include <iostream>
#include <string>
#include <string_view>

void print_string(std::string_view input);

// This is a simple C++ example of how to use the Mojibake library.
// Run `make example-cpp` from the examples directory to compile it.
// ./build/example-cpp to run it.
int main() {
    try {
        constexpr std::string_view input = "Cafe\xCC\x81";

        // Normalize example: in NFC e + ◌́ -> é (U+00E9)
        const std::string normalized = mjb::nfc(input);

        // Cafe + ◌́ (U+0301, COMBINING ACUTE ACCENT) -> Café
        print_string(input);

        // Caf + é (U+00E9, LATIN SMALL LETTER E WITH ACUTE) -> Café
        print_string(normalized);

        constexpr std::string_view mojibake = "文字化け";

        // Codepoint count example: mjb::codepoint_count counts Unicode codepoints, not bytes.
        std::cout << '"' << mojibake << "\" encoded in UTF-8 is " << mojibake.size()
                  << " bytes long, and " << mjb::codepoint_count(mojibake) << " codepoints long\n";

        constexpr std::string_view case_input = "Straße";

        // NFKC casefold example: in NFKC casefold, ß -> ss
        std::cout << case_input << " -> " << mjb::nfkc_casefold(case_input) << '\n';
    } catch(const mjb::LibraryError &) {
        return 1;
    }

    return 0;
}

void print_string(std::string_view input) {
    constexpr char hex_digits[] = "0123456789ABCDEF";

    for(const unsigned char byte : input) {
        if(byte >= 0x21 && byte <= 0x7E) {
            std::cout << static_cast<char>(byte);
        } else {
            std::cout << '<' << hex_digits[byte >> 4] << hex_digits[byte & 0x0F] << '>';
        }
    }

    std::cout << '\n';
}
