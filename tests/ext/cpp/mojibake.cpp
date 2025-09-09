/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "../../test.h"
#include "../../../ext/cpp/mojibake.hpp"

void *test_cpp_mojibake(void *arg) {
    // U+1F642 = 🙂
    auto character = mjb::Character((mjb_codepoint)0x1F642);
    // U+1F643 = 🙃
    auto character2 = mjb::Character((mjb_codepoint)0x1F643);
    // U+1F642 = 🙂
    auto character3 = mjb::Character((mjb_codepoint)0x1F642);

    ATT_ASSERT(character.is_valid(), true, "Character::is_valid");
    ATT_ASSERT(character.to_utf8(), std::string("\xF0\x9F\x99\x82"), "Character::to_utf8");

    ATT_ASSERT(character2.is_valid(), true, "Character::is_valid");
    ATT_ASSERT(character2.to_utf8(), std::string("\xF0\x9F\x99\x83"), "Character::to_utf8");

    ATT_ASSERT(character == character2, false, "Character::operator==");
    ATT_ASSERT(character == character3, true, "Character::operator==");
    ATT_ASSERT(character3 == character, true, "Character::operator==");
    ATT_ASSERT(character3 != character2, true, "Character::operator!=");

    return NULL;
}
