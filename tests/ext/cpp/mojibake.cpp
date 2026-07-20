/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "../../../src/cpp/mojibake.hpp"
#include "../../test.h"

static size_t cpp_each_character_count = 0;

static bool cpp_each_character(mjb_character *character, mjb_character_position position) {
    (void)position;

    if(character != nullptr) {
        ++cpp_each_character_count;
    }

    return true;
}

int test_cpp_mojibake(void *arg) {
    ATT_ASSERT(mjb::property_binary(U'A', MJB_PR_ALPHABETIC).value_or(false), true,
        "C++ typed binary property")
    ATT_ASSERT(mjb::property_int(U'A', MJB_PR_SCRIPT).value_or(0), MJB_SC_LATN,
        "C++ typed enumerated property")

    const auto script_extensions = mjb::script_extensions(U'\u30FC');

    ATT_ASSERT(script_extensions.size(), 2u, "C++ Script_Extensions count")
    ATT_ASSERT((int)script_extensions[0], MJB_SC_HIRA, "C++ Script_Extensions Hiragana")
    ATT_ASSERT((int)script_extensions[1], MJB_SC_KANA, "C++ Script_Extensions Katakana")

    // U+1F642 = 🙂
    auto character = mjb::Character((mjb_codepoint)0x1F642);
    // U+1F643 = 🙃
    auto character2 = mjb::Character((mjb_codepoint)0x1F643);
    // U+1F642 = 🙂
    auto character3 = mjb::Character((mjb_codepoint)0x1F642);

    ATT_ASSERT(character.to_utf8(), std::string("\xF0\x9F\x99\x82"), "Character::to_utf8")
    ATT_ASSERT(character2.to_utf8(), std::string("\xF0\x9F\x99\x83"), "Character::to_utf8")

    ATT_ASSERT(mjb::Character::from(0x1F642).has_value(), true, "Character::from valid codepoint")
    ATT_ASSERT(mjb::Character::from(MJB_CODEPOINT_NOT_VALID).has_value(), false,
        "Character::from invalid codepoint")
    ATT_ASSERT(mjb::Character::from(0x1F642)->codepoint(), (mjb_codepoint)0x1F642,
        "Character::from codepoint accessor")

    ATT_ASSERT(character == character2, false, "Character::operator==")
    ATT_ASSERT(character == character3, true, "Character::operator==")
    ATT_ASSERT(character3 == character, true, "Character::operator==")
    ATT_ASSERT(character3 != character2, true, "Character::operator!=")

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
    ATT_ASSERT(mjb::confusable_skeleton("h\xD0\xB5llo"), std::string("hello"),
        "confusable_skeleton")

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
    ATT_ASSERT(std::string(mjb::truncate("hello", 10)), std::string("hello"),
        "truncate: beyond length")
    ATT_ASSERT(std::string(mjb::truncate_word("hello world", 1)), std::string("hello"),
        "truncate_word: 1 segment")

    ATT_ASSERT(std::string(mjb::property_name(MJB_PR_ALPHABETIC)), std::string("Alphabetic"),
        "property_name")
    ATT_ASSERT((int)mjb::script('A'), MJB_SC_LATN, "script")

    const auto block = mjb::block_info('A');
    ATT_ASSERT(block.has_value(), true, "block_info has value")
    ATT_ASSERT(std::string(block->name()), std::string("Basic Latin"), "block_info name")
    ATT_ASSERT(block->contains('Z'), true, "block_info contains")

    const auto emoji = mjb::emoji_properties(0x1F642);
    ATT_ASSERT(emoji.has_value(), true, "emoji_properties has value")
    ATT_ASSERT(emoji->is_emoji(), true, "emoji_properties is_emoji")
    ATT_ASSERT(emoji->is_extended_pictographic(), true,
        "emoji_properties is_extended_pictographic")

    ATT_ASSERT(mjb::is_valid('A'), true, "is_valid codepoint")
    ATT_ASSERT(mjb::is_graphic('A'), true, "is_graphic")
    ATT_ASSERT(mjb::is_combining(0x0301), true, "is_combining")
    ATT_ASSERT(mjb::category_is_graphic(MJB_CATEGORY_LL), true, "category_is_graphic")
    ATT_ASSERT(mjb::category_is_combining(MJB_CATEGORY_MN), true, "category_is_combining")
    ATT_ASSERT(mjb::is_hangul_l(0x1100), true, "is_hangul_l")
    ATT_ASSERT(mjb::is_hangul_v(0x1161), true, "is_hangul_v")
    ATT_ASSERT(mjb::is_hangul_t(0x11A8), true, "is_hangul_t")
    ATT_ASSERT(mjb::is_hangul_jamo(0x1100), true, "is_hangul_jamo")
    ATT_ASSERT(mjb::is_hangul_syllable(0xAC00), true, "is_hangul_syllable")
    ATT_ASSERT(mjb::is_cjk_ideograph(0x4E00), true, "is_cjk_ideograph")
    ATT_ASSERT(mjb::is_cjk_extension(0x3400), true, "is_cjk_extension")
    ATT_ASSERT(mjb::is_emoji(0x1F642), true, "is_emoji codepoint")
    ATT_ASSERT(mjb::is_emoji_presentation(0x1F642), true, "is_emoji_presentation")
    ATT_ASSERT(mjb::is_extended_pictographic(0x1F642), true,
        "is_extended_pictographic")
    ATT_ASSERT(mjb::is_id_start('A'), true, "is_id_start codepoint")
    ATT_ASSERT(mjb::is_id_continue('1'), true, "is_id_continue codepoint")
    ATT_ASSERT(mjb::is_pattern_syntax('!'), true, "is_pattern_syntax codepoint")

    ATT_ASSERT((int)mjb::plane('A'), MJB_PLANE_BMP, "plane")
    ATT_ASSERT(mjb::is_valid(MJB_PLANE_BMP), true, "is_valid plane")
    ATT_ASSERT(std::string(mjb::plane_name(MJB_PLANE_BMP, true)), std::string("BMP"),
        "plane_name")
    ATT_ASSERT((int)mjb::east_asian_width('A'), MJB_EAW_NARROW, "east_asian_width")
    ATT_ASSERT(mjb::encode(0x1F642), std::string("\xF0\x9F\x99\x82"), "encode")

    cpp_each_character_count = 0;
    ATT_ASSERT_STATUS(mjb::each_character("ABC", cpp_each_character), MJB_STATUS_OK,
        "each_character")
    ATT_ASSERT(cpp_each_character_count, 3u, "each_character count")

    const auto sequence = mjb::emoji_sequence("\xF0\x9F\x99\x82");
    ATT_ASSERT(sequence.has_value(), true, "emoji_sequence has value")
    ATT_ASSERT(sequence->size(), 1u, "emoji_sequence size")
    ATT_ASSERT(mjb::is_emoji_sequence("\xF0\x9F\x99\x82"), true, "is_emoji_sequence")
    ATT_ASSERT(mjb::is_rgi_emoji("\xF0\x9F\x99\x82"), true, "is_rgi_emoji")
    ATT_ASSERT(mjb::display_width("abc"), 3u, "display_width")

    ATT_ASSERT(mjb::hangul_syllable_name(0xAC00), std::string("HANGUL SYLLABLE GA"),
        "hangul_syllable_name")
    const auto decomposition = mjb::hangul_syllable_decomposition(0xAC00);
    ATT_ASSERT(decomposition[0], (mjb_codepoint)0x1100, "hangul decomposition L")
    ATT_ASSERT(decomposition[1], (mjb_codepoint)0x1161, "hangul decomposition V")
    ATT_ASSERT(decomposition[2], (mjb_codepoint)0, "hangul decomposition T")

    const auto composition = mjb::hangul_syllable_composition(std::vector<mjb_buffer_character>{
        { 0x1100, 0 }, { 0x1161, 0 } });
    ATT_ASSERT(composition.size(), 1u, "hangul composition size")
    ATT_ASSERT(composition[0].codepoint, (mjb_codepoint)0xAC00, "hangul composition codepoint")

    ATT_ASSERT(std::string(mjb::version()), std::string(MJB_VERSION), "version")
    ATT_ASSERT(mjb::version_number(), (unsigned int)MJB_VERSION_NUMBER, "version_number")
    ATT_ASSERT(std::string(mjb::unicode_version()), std::string(MJB_UNICODE_VERSION),
        "unicode_version")

    mjb::set_memory_functions(nullptr, nullptr, nullptr);
    void *memory = mjb::allocate(8);
    ATT_ASSERT(memory != nullptr, true, "allocate")
    memory = mjb::reallocate(memory, 16);
    ATT_ASSERT(memory != nullptr, true, "reallocate")
    mjb::deallocate(memory);

    return 0;
}
