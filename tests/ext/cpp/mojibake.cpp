/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "../../test.h"
#include "../../../ext/cpp/mojibake.hpp"

void *test_cpp_mojibake(void *arg) {
    auto character = mjb::Character((mjb_codepoint)0x1F642);

    ATT_ASSERT(character.is_valid(), true, "Character::is_valid");
    ATT_ASSERT(character.to_utf8(), std::string("\xF0\x9F\x99\x82"), "Character::to_utf8");

    return NULL;
}
