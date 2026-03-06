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

    // Identifier properties on Character
    // U+0041 = 'A': valid ID_Start and ID_Continue
    auto char_a = mjb::Character((mjb_codepoint)'A');
    ATT_ASSERT(char_a.is_id_start(), true, "A: is_id_start")
    ATT_ASSERT(char_a.is_id_continue(), true, "A: is_id_continue")
    ATT_ASSERT(char_a.is_xid_start(), true, "A: is_xid_start")
    ATT_ASSERT(char_a.is_xid_continue(), true, "A: is_xid_continue")
    ATT_ASSERT(char_a.is_pattern_syntax(), false, "A: is_pattern_syntax")
    ATT_ASSERT(char_a.is_pattern_white_space(), false, "A: is_pattern_white_space")

    // U+0021 = '!': pattern syntax
    auto char_bang = mjb::Character((mjb_codepoint)'!');
    ATT_ASSERT(char_bang.is_id_start(), false, "!: is_id_start")
    ATT_ASSERT(char_bang.is_pattern_syntax(), true, "!: is_pattern_syntax")

    // is_identifier free function
    ATT_ASSERT(mjb::is_identifier("hello"), true, "is_identifier: hello")
    ATT_ASSERT(mjb::is_identifier("hello world"), false, "is_identifier: hello world")
    ATT_ASSERT(mjb::is_identifier(""), false, "is_identifier: empty")

    // is_confusable free function
    ATT_ASSERT(mjb::is_confusable("\xD0\x90", "A"), true, "is_confusable: Cyrillic A / Latin A")
    ATT_ASSERT(mjb::is_confusable("hello", "hello"), true, "is_confusable: same string")
    ATT_ASSERT(mjb::is_confusable("a", "b"), false, "is_confusable: a / b")

    // NumericValue and codepoint_numeric_value
    // U+0031 = '1'
    auto num1 = mjb::codepoint_numeric_value('1');
    ATT_ASSERT(num1.decimal().has_value(), true, "numeric_value '1': decimal has_value")
    ATT_ASSERT(*num1.decimal(), 1, "numeric_value '1': decimal")
    ATT_ASSERT(*num1.digit(), 1, "numeric_value '1': digit")
    ATT_ASSERT(std::string(num1.numeric()), std::string("1"), "numeric_value '1': numeric")

    // U+00BD = '½': only numeric string
    auto num_half = mjb::codepoint_numeric_value(0x00BD);
    ATT_ASSERT(num_half.decimal().has_value(), false, "numeric_value ½: no decimal")
    ATT_ASSERT(std::string(num_half.numeric()), std::string("1/2"), "numeric_value ½: numeric")

    // collation_key
    auto key_a = mjb::collation_key("a");
    auto key_b = mjb::collation_key("b");
    ATT_ASSERT(key_a.empty(), false, "collation_key: a not empty")
    ATT_ASSERT(key_a < key_b, true, "collation_key: a < b")
    ATT_ASSERT(mjb::collation_key(""), std::string(""), "collation_key: empty")

    // truncate
    ATT_ASSERT(std::string(mjb::truncate("hello", 3)), std::string("hel"), "truncate: 3 graphemes")
    ATT_ASSERT(std::string(mjb::truncate("hello", 10)), std::string("hello"), "truncate: beyond length")
    ATT_ASSERT(std::string(mjb::truncate_word("hello world", 1)), std::string("hello"), "truncate_word: 1 segment")

    return NULL;
}
