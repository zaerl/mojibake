/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "../../../src/cpp/mojibake.hpp"
#include "../../test.h"

#include <format>
#include <iomanip>
#include <sstream>
#include <string>

// clang-format off
ATT_TEST(cpp_example) {
    std::ostringstream oss;

    // This function is automatically generated. Do not edit.
{
    // Example for mjb_codepoint_info
    MJB_TEST_COVERAGE(mjb_codepoint_info); // Added by the script
    auto character = mjb::Character::from(0x022A);

    if(!character || !character->lowercase()) {
        ATT_ASSERT(0, 1, "mjb_codepoint_info test failed"); // Added by the script
        return;
    }

    // U+022A lowercase: U+022B
    // std::cout << std::format("U+{:04X} lowercase: U+{:04X}", character->codepoint(),
    //     *character->lowercase()) << std::endl;
    oss << std::format("U+{:04X} lowercase: U+{:04X}", character->codepoint(), // Added by the script
        *character->lowercase()) << std::endl; // Added by the script
    ATT_ASSERT(oss.str(), "U+022A lowercase: U+022B\n", "mjb_codepoint_info test failed"); // Added by the script
    oss.str("");
    oss.clear();
}
    // End of generated examples.
}
// clang-format on
