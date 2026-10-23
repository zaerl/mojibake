/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "../../../src/cpp/mojibake.hpp"
#include "../../test.h"

#include <iomanip>
#include <sstream>
#include <string>

// clang-format off
ATT_TEST(cpp_example) {
    std::ostringstream oss;

#define RESET_STATE() \
    oss.clear(); \
    oss.copyfmt(std::ostringstream());

    // This function is automatically generated. Do not edit.
{
    // Example for mjb_codepoint_info
    MJB_TEST_COVERAGE(mjb_codepoint_info); // Added by the script
    // U+0665 ARABIC-INDIC DIGIT FIVE
    auto character = mjb::Character::from(0x0665);

    if(!character) {
        ATT_ASSERT(0, 1, "mjb_codepoint_info test failed"); // Added by the script
        return;
    }

    bool is_digit = character->category() == MJB_CATEGORY_ND;

    // Decimal digit: yes, value: 5
    // std::cout << "Decimal digit: " << (is_digit ? "yes" : "no") << ", value: "
    //     << character->decimal_value().value_or(-1) << std::endl;
    oss << "Decimal digit: " << (is_digit ? "yes" : "no") << ", value: " // Added by the script
        << character->decimal_value().value_or(-1) << std::endl; // Added by the script
    ATT_ASSERT(oss.str(), "Decimal digit: yes, value: 5\n", "mjb_codepoint_info test failed"); // Added by the script
    oss.str(""); // Added by the script
    RESET_STATE()
}

{
    // Example for mjb_normalize
    MJB_TEST_COVERAGE(mjb_normalize); // Added by the script
    // "Cafe" + U+0301 COMBINING ACUTE ACCENT
    std::string input = "Cafe\xCC\x81";
    std::string output = mjb::normalize(input, mjb::NormalizationForm::NFC);

    // NFC: Café
    // std::cout << "NFC: " << output << std::endl;
    oss << "NFC: " << output << std::endl; // Added by the script
    ATT_ASSERT(oss.str(), "NFC: Café\n", "mjb_normalize test failed"); // Added by the script
    oss.str(""); // Added by the script
    RESET_STATE()
}

{
    // Example for mjb_normalize_into
    MJB_TEST_COVERAGE(mjb_normalize_into); // Added by the script
    // "Cafe" + U+0301 COMBINING ACUTE ACCENT
    std::string input = "Cafe\xCC\x81";
    std::string output;

    // Replaces the content of output and reuses its capacity on later calls
    mjb::normalize(input, mjb::NormalizationForm::NFC, output);

    // NFC: Café
    // std::cout << "NFC: " << output << std::endl;
    oss << "NFC: " << output << std::endl; // Added by the script
    ATT_ASSERT(oss.str(), "NFC: Café\n", "mjb_normalize_into test failed"); // Added by the script
    oss.str(""); // Added by the script
    RESET_STATE()
}

{
    // Example for mjb_filter
    MJB_TEST_COVERAGE(mjb_filter); // Added by the script
    std::string mixed_whitespace = "Hello\t\t\n\nworld";

    // Filtered: Hello world
    // std::cout << "Filtered: " << mjb::filter(mixed_whitespace, mjb::Filter::CollapseSpaces)
    //     << std::endl;
    oss << "Filtered: " << mjb::filter(mixed_whitespace, mjb::Filter::CollapseSpaces) // Added by the script
        << std::endl; // Added by the script
    ATT_ASSERT(oss.str(), "Filtered: Hello world\n", "mjb_filter test failed"); // Added by the script
    oss.str(""); // Added by the script

    std::string controls = "\x1\x2\t\n\v\f\r\x1f";

    // Filtered: \t\n\v\f\r
    // std::cout << "Filtered: " << mjb::filter(controls, mjb::Filter::Controls) << std::endl;
    oss << "Filtered: " << mjb::filter(controls, mjb::Filter::Controls) << std::endl; // Added by the script
    ATT_ASSERT(oss.str(), "Filtered: \t\n\v\f\r\n", "mjb_filter test failed"); // Added by the script
    oss.str(""); // Added by the script
    RESET_STATE()
}

{
    // Example for mjb_filter_into
    MJB_TEST_COVERAGE(mjb_filter_into); // Added by the script
    std::string input = "Hello\t\t\nworld";
    std::string output;

    // Replaces the content of output and reuses its capacity on later calls
    mjb::filter(input, mjb::Filter::CollapseSpaces, output);

    // Filtered: Hello world
    // std::cout << "Filtered: " << output << std::endl;
    oss << "Filtered: " << output << std::endl; // Added by the script
    ATT_ASSERT(oss.str(), "Filtered: Hello world\n", "mjb_filter_into test failed"); // Added by the script
    oss.str(""); // Added by the script
    RESET_STATE()
}

{
    // Example for mjb_nfkc_casefold
    MJB_TEST_COVERAGE(mjb_nfkc_casefold); // Added by the script
    // "Straße" + U+00AD SOFT HYPHEN
    std::string input = "Stra\xC3\x9F" "e\xC2\xAD";

    // strasse
    // std::cout << mjb::nfkc_casefold(input) << std::endl;
    oss << mjb::nfkc_casefold(input) << std::endl; // Added by the script
    ATT_ASSERT(oss.str(), "strasse\n", "mjb_nfkc_casefold test failed"); // Added by the script
    oss.str(""); // Added by the script
    RESET_STATE()
}

{
    // Example for mjb_nfkc_casefold_into
    MJB_TEST_COVERAGE(mjb_nfkc_casefold_into); // Added by the script
    // "Straße" + U+00AD SOFT HYPHEN
    std::string input = "Stra\xC3\x9F" "e\xC2\xAD";
    std::string output;

    // Replaces the content of output and reuses its capacity on later calls
    mjb::nfkc_casefold(input, output);

    // NFKC casefold: strasse
    // std::cout << "NFKC casefold: " << output << std::endl;
    oss << "NFKC casefold: " << output << std::endl; // Added by the script
    ATT_ASSERT(oss.str(), "NFKC casefold: strasse\n", "mjb_nfkc_casefold_into test failed"); // Added by the script
    oss.str(""); // Added by the script
    RESET_STATE()
}

#if MJB_FEATURE_IDNA
{
    // Example for mjb_idna_to_ascii
    MJB_TEST_COVERAGE(mjb_idna_to_ascii); // Added by the script
    // "bücher.example"
    auto domain = mjb::idna_to_ascii("b\xC3\xBC" "cher.example");

    if(!domain.valid()) {
        ATT_ASSERT(0, 1, "mjb_idna_to_ascii test failed"); // Added by the script
        return;
    }

    // xn--bcher-kva.example
    // std::cout << domain.text << std::endl;
    oss << domain.text << std::endl; // Added by the script
    ATT_ASSERT(oss.str(), "xn--bcher-kva.example\n", "mjb_idna_to_ascii test failed"); // Added by the script
    oss.str(""); // Added by the script
    RESET_STATE()
}
#endif // MJB_FEATURE_IDNA

#if MJB_FEATURE_IDNA
{
    // Example for mjb_idna_to_ascii_into
    MJB_TEST_COVERAGE(mjb_idna_to_ascii_into); // Added by the script
    // "bücher.example"
    std::string domain = "b\xC3\xBC" "cher.example";
    std::string output;

    // Returns the mjb_idna_error flags
    if(mjb::idna_to_ascii(domain, output) != MJB_IDNA_ERROR_NONE) {
        ATT_ASSERT(0, 1, "mjb_idna_to_ascii_into test failed"); // Added by the script
        return;
    }

    // xn--bcher-kva.example
    // std::cout << output << std::endl;
    oss << output << std::endl; // Added by the script
    ATT_ASSERT(oss.str(), "xn--bcher-kva.example\n", "mjb_idna_to_ascii_into test failed"); // Added by the script
    oss.str(""); // Added by the script
    RESET_STATE()
}
#endif // MJB_FEATURE_IDNA

#if MJB_FEATURE_IDNA
{
    // Example for mjb_idna_to_unicode
    MJB_TEST_COVERAGE(mjb_idna_to_unicode); // Added by the script
    auto domain = mjb::idna_to_unicode("xn--bcher-kva.example");

    if(!domain.valid()) {
        ATT_ASSERT(0, 1, "mjb_idna_to_unicode test failed"); // Added by the script
        return;
    }

    // bücher.example
    // std::cout << domain.text << std::endl;
    oss << domain.text << std::endl; // Added by the script
    ATT_ASSERT(oss.str(), "bücher.example\n", "mjb_idna_to_unicode test failed"); // Added by the script
    oss.str(""); // Added by the script
    RESET_STATE()
}
#endif // MJB_FEATURE_IDNA

#if MJB_FEATURE_IDNA
{
    // Example for mjb_idna_to_unicode_into
    MJB_TEST_COVERAGE(mjb_idna_to_unicode_into); // Added by the script
    std::string domain = "xn--bcher-kva.example";
    std::string output;

    // Returns the mjb_idna_error flags
    if(mjb::idna_to_unicode(domain, output) != MJB_IDNA_ERROR_NONE) {
        ATT_ASSERT(0, 1, "mjb_idna_to_unicode_into test failed"); // Added by the script
        return;
    }

    // bücher.example
    // std::cout << output << std::endl;
    oss << output << std::endl; // Added by the script
    ATT_ASSERT(oss.str(), "bücher.example\n", "mjb_idna_to_unicode_into test failed"); // Added by the script
    oss.str(""); // Added by the script
    RESET_STATE()
}
#endif // MJB_FEATURE_IDNA

{
    // Example for mjb_is_ascii
    MJB_TEST_COVERAGE(mjb_is_ascii); // Added by the script
    std::string input = "Plain ASCII";

    // ASCII: yes
    // std::cout << "ASCII: " << (mjb::is_ascii(input) ? "yes" : "no") << std::endl;
    oss << "ASCII: " << (mjb::is_ascii(input) ? "yes" : "no") << std::endl; // Added by the script
    ATT_ASSERT(oss.str(), "ASCII: yes\n", "mjb_is_ascii test failed"); // Added by the script
    oss.str(""); // Added by the script
    RESET_STATE()
}

{
    // Example for mjb_is_utf8
    MJB_TEST_COVERAGE(mjb_is_utf8); // Added by the script
    std::string input = "caf\xC3\xA9";

    // Valid UTF-8: yes
    // std::cout << "Valid UTF-8: " << (mjb::is_utf8(input) ? "yes" : "no") << std::endl;
    oss << "Valid UTF-8: " << (mjb::is_utf8(input) ? "yes" : "no") << std::endl; // Added by the script
    ATT_ASSERT(oss.str(), "Valid UTF-8: yes\n", "mjb_is_utf8 test failed"); // Added by the script
    oss.str(""); // Added by the script
    RESET_STATE()
}

{
    // Example for mjb_is_utf16
    MJB_TEST_COVERAGE(mjb_is_utf16); // Added by the script
    using namespace std::string_view_literals;

    // BOM + "Hi" in UTF-16BE. The sv suffix keeps the embedded NUL bytes.
    auto utf16be = "\xFE\xFF\0H\0i"sv;

    // UTF-16: yes
    // std::cout << "UTF-16: " << (mjb::is_utf16(utf16be) ? "yes" : "no") << std::endl;
    oss << "UTF-16: " << (mjb::is_utf16(utf16be) ? "yes" : "no") << std::endl; // Added by the script
    ATT_ASSERT(oss.str(), "UTF-16: yes\n", "mjb_is_utf16 test failed"); // Added by the script
    oss.str(""); // Added by the script
    RESET_STATE()
}

{
    // Example for mjb_validate_encoding
    MJB_TEST_COVERAGE(mjb_validate_encoding); // Added by the script
    std::string invalid = "\xE2\x82"; // A truncated three-byte sequence
    mjb_diagnostic diagnostic{};

    if(mjb::validate_encoding(invalid, &diagnostic) != MJB_STATUS_MALFORMED_INPUT) {
        ATT_ASSERT(0, 1, "mjb_validate_encoding test failed"); // Added by the script
        return;
    }

    // Malformed input at byte 0
    // std::cout << "Malformed input at byte " << diagnostic.byte_offset << std::endl;
    oss << "Malformed input at byte " << diagnostic.byte_offset << std::endl; // Added by the script
    ATT_ASSERT(oss.str(), "Malformed input at byte 0\n", "mjb_validate_encoding test failed"); // Added by the script
    oss.str(""); // Added by the script
    RESET_STATE()
}

{
    // Example for mjb_codepoint_block
    MJB_TEST_COVERAGE(mjb_codepoint_block); // Added by the script
    auto block = mjb::block_info('A');

    if(!block) {
        ATT_ASSERT(0, 1, "mjb_codepoint_block test failed"); // Added by the script
        return;
    }

    // Block: Basic Latin
    // std::cout << "Block: " << block->name() << std::endl;
    oss << "Block: " << block->name() << std::endl; // Added by the script
    ATT_ASSERT(oss.str(), "Block: Basic Latin\n", "mjb_codepoint_block test failed"); // Added by the script
    oss.str(""); // Added by the script
    RESET_STATE()
}

{
    // Example for mjb_codepoint_script
    MJB_TEST_COVERAGE(mjb_codepoint_script); // Added by the script
    mjb_script script = mjb::script(0x03A9); // Greek capital omega

    // Greek script: yes
    // std::cout << "Greek script: " << (script == MJB_SC_GREK ? "yes" : "no") << std::endl;
    oss << "Greek script: " << (script == MJB_SC_GREK ? "yes" : "no") << std::endl; // Added by the script
    ATT_ASSERT(oss.str(), "Greek script: yes\n", "mjb_codepoint_script test failed"); // Added by the script
    oss.str(""); // Added by the script
    RESET_STATE()
}

{
    // Example for mjb_codepoint_script_extensions
    MJB_TEST_COVERAGE(mjb_codepoint_script_extensions); // Added by the script
    // Throws mjb::LibraryError on failure
    std::vector<mjb_script> scripts = mjb::script_extensions(0x30FC);

    // U+30FC has 2 Script_Extensions
    // std::cout << "U+30FC has " << scripts.size() << " Script_Extensions" << std::endl;
    oss << "U+30FC has " << scripts.size() << " Script_Extensions" << std::endl; // Added by the script
    ATT_ASSERT(oss.str(), "U+30FC has 2 Script_Extensions\n", "mjb_codepoint_script_extensions test failed"); // Added by the script
    oss.str(""); // Added by the script
    RESET_STATE()
}

{
    // Example for mjb_codepoint_is_valid
    MJB_TEST_COVERAGE(mjb_codepoint_is_valid); // Added by the script
    // U+10FFFD valid: yes
    // std::cout << "U+10FFFD valid: " << (mjb::is_valid(0x10FFFD) ? "yes" : "no") << std::endl;
    oss << "U+10FFFD valid: " << (mjb::is_valid(0x10FFFD) ? "yes" : "no") << std::endl; // Added by the script
    ATT_ASSERT(oss.str(), "U+10FFFD valid: yes\n", "mjb_codepoint_is_valid test failed"); // Added by the script
    oss.str(""); // Added by the script
    RESET_STATE()
}

{
    // Example for mjb_codepoint_is_graphic
    MJB_TEST_COVERAGE(mjb_codepoint_is_graphic); // Added by the script
    // Letter A is graphic: yes
    // std::cout << "Letter A is graphic: " << (mjb::is_graphic('A') ? "yes" : "no") << std::endl;
    oss << "Letter A is graphic: " << (mjb::is_graphic('A') ? "yes" : "no") << std::endl; // Added by the script
    ATT_ASSERT(oss.str(), "Letter A is graphic: yes\n", "mjb_codepoint_is_graphic test failed"); // Added by the script
    oss.str(""); // Added by the script
    RESET_STATE()
}

{
    // Example for mjb_codepoint_is_combining
    MJB_TEST_COVERAGE(mjb_codepoint_is_combining); // Added by the script
    // U+0301 is combining: yes
    // std::cout << "U+0301 is combining: " << (mjb::is_combining(0x0301) ? "yes" : "no") << std::endl;
    oss << "U+0301 is combining: " << (mjb::is_combining(0x0301) ? "yes" : "no") << std::endl; // Added by the script
    ATT_ASSERT(oss.str(), "U+0301 is combining: yes\n", "mjb_codepoint_is_combining test failed"); // Added by the script
    oss.str(""); // Added by the script
    RESET_STATE()
}

{
    // Example for mjb_codepoint_is_hangul_leading_jamo
    MJB_TEST_COVERAGE(mjb_codepoint_is_hangul_leading_jamo); // Added by the script
    // U+1100 is a leading Jamo: yes
    // std::cout << "U+1100 is a leading Jamo: "
    //     << (mjb::is_hangul_leading_jamo(0x1100) ? "yes" : "no") << std::endl;
    oss << "U+1100 is a leading Jamo: " // Added by the script
        << (mjb::is_hangul_leading_jamo(0x1100) ? "yes" : "no") << std::endl; // Added by the script
    ATT_ASSERT(oss.str(), "U+1100 is a leading Jamo: yes\n", "mjb_codepoint_is_hangul_leading_jamo test failed"); // Added by the script
    oss.str(""); // Added by the script
    RESET_STATE()
}

{
    // Example for mjb_codepoint_is_hangul_vowel_jamo
    MJB_TEST_COVERAGE(mjb_codepoint_is_hangul_vowel_jamo); // Added by the script
    // U+1161 is a vowel Jamo: yes
    // std::cout << "U+1161 is a vowel Jamo: "
    //     << (mjb::is_hangul_vowel_jamo(0x1161) ? "yes" : "no") << std::endl;
    oss << "U+1161 is a vowel Jamo: " // Added by the script
        << (mjb::is_hangul_vowel_jamo(0x1161) ? "yes" : "no") << std::endl; // Added by the script
    ATT_ASSERT(oss.str(), "U+1161 is a vowel Jamo: yes\n", "mjb_codepoint_is_hangul_vowel_jamo test failed"); // Added by the script
    oss.str(""); // Added by the script
    RESET_STATE()
}

{
    // Example for mjb_codepoint_is_hangul_trailing_jamo
    MJB_TEST_COVERAGE(mjb_codepoint_is_hangul_trailing_jamo); // Added by the script
    // U+11A8 is a trailing Jamo: yes
    // std::cout << "U+11A8 is a trailing Jamo: "
    //     << (mjb::is_hangul_trailing_jamo(0x11A8) ? "yes" : "no") << std::endl;
    oss << "U+11A8 is a trailing Jamo: " // Added by the script
        << (mjb::is_hangul_trailing_jamo(0x11A8) ? "yes" : "no") << std::endl; // Added by the script
    ATT_ASSERT(oss.str(), "U+11A8 is a trailing Jamo: yes\n", "mjb_codepoint_is_hangul_trailing_jamo test failed"); // Added by the script
    oss.str(""); // Added by the script
    RESET_STATE()
}

{
    // Example for mjb_codepoint_is_hangul_jamo
    MJB_TEST_COVERAGE(mjb_codepoint_is_hangul_jamo); // Added by the script
    // U+1100 is Hangul Jamo: yes
    // std::cout << "U+1100 is Hangul Jamo: " << (mjb::is_hangul_jamo(0x1100) ? "yes" : "no") << std::endl;
    oss << "U+1100 is Hangul Jamo: " << (mjb::is_hangul_jamo(0x1100) ? "yes" : "no") << std::endl; // Added by the script
    ATT_ASSERT(oss.str(), "U+1100 is Hangul Jamo: yes\n", "mjb_codepoint_is_hangul_jamo test failed"); // Added by the script
    oss.str(""); // Added by the script
    RESET_STATE()
}

{
    // Example for mjb_codepoint_is_hangul_syllable
    MJB_TEST_COVERAGE(mjb_codepoint_is_hangul_syllable); // Added by the script
    // U+AC00 is a Hangul syllable: yes
    // std::cout << "U+AC00 is a Hangul syllable: "
    //     << (mjb::is_hangul_syllable(0xAC00) ? "yes" : "no") << std::endl;
    oss << "U+AC00 is a Hangul syllable: " // Added by the script
        << (mjb::is_hangul_syllable(0xAC00) ? "yes" : "no") << std::endl; // Added by the script
    ATT_ASSERT(oss.str(), "U+AC00 is a Hangul syllable: yes\n", "mjb_codepoint_is_hangul_syllable test failed"); // Added by the script
    oss.str(""); // Added by the script
    RESET_STATE()
}

{
    // Example for mjb_codepoint_is_cjk_ideograph
    MJB_TEST_COVERAGE(mjb_codepoint_is_cjk_ideograph); // Added by the script
    // U+4E00 is a CJK ideograph: yes
    // std::cout << "U+4E00 is a CJK ideograph: "
    //     << (mjb::is_cjk_ideograph(0x4E00) ? "yes" : "no") << std::endl;
    oss << "U+4E00 is a CJK ideograph: " // Added by the script
        << (mjb::is_cjk_ideograph(0x4E00) ? "yes" : "no") << std::endl; // Added by the script
    ATT_ASSERT(oss.str(), "U+4E00 is a CJK ideograph: yes\n", "mjb_codepoint_is_cjk_ideograph test failed"); // Added by the script
    oss.str(""); // Added by the script
    RESET_STATE()
}

{
    // Example for mjb_codepoint_is_cjk_extension_ideograph
    MJB_TEST_COVERAGE(mjb_codepoint_is_cjk_extension_ideograph); // Added by the script
    // U+20000 is a CJK extension ideograph: yes
    // std::cout << "U+20000 is a CJK extension ideograph: "
    //     << (mjb::is_cjk_extension_ideograph(0x20000) ? "yes" : "no") << std::endl;
    oss << "U+20000 is a CJK extension ideograph: " // Added by the script
        << (mjb::is_cjk_extension_ideograph(0x20000) ? "yes" : "no") << std::endl; // Added by the script
    ATT_ASSERT(oss.str(), "U+20000 is a CJK extension ideograph: yes\n", "mjb_codepoint_is_cjk_extension_ideograph test failed"); // Added by the script
    oss.str(""); // Added by the script
    RESET_STATE()
}

{
    // Example for mjb_codepoint_is_id_start
    MJB_TEST_COVERAGE(mjb_codepoint_is_id_start); // Added by the script
    // Greek alpha starts an identifier: yes
    // std::cout << "Greek alpha starts an identifier: "
    //     << (mjb::is_id_start(0x03B1) ? "yes" : "no") << std::endl;
    oss << "Greek alpha starts an identifier: " // Added by the script
        << (mjb::is_id_start(0x03B1) ? "yes" : "no") << std::endl; // Added by the script
    ATT_ASSERT(oss.str(), "Greek alpha starts an identifier: yes\n", "mjb_codepoint_is_id_start test failed"); // Added by the script
    oss.str(""); // Added by the script
    RESET_STATE()
}

{
    // Example for mjb_codepoint_is_id_continue
    MJB_TEST_COVERAGE(mjb_codepoint_is_id_continue); // Added by the script
    // Digit 7 continues an identifier: yes
    // std::cout << "Digit 7 continues an identifier: "
    //     << (mjb::is_id_continue('7') ? "yes" : "no") << std::endl;
    oss << "Digit 7 continues an identifier: " // Added by the script
        << (mjb::is_id_continue('7') ? "yes" : "no") << std::endl; // Added by the script
    ATT_ASSERT(oss.str(), "Digit 7 continues an identifier: yes\n", "mjb_codepoint_is_id_continue test failed"); // Added by the script
    oss.str(""); // Added by the script
    RESET_STATE()
}

{
    // Example for mjb_codepoint_is_xid_start
    MJB_TEST_COVERAGE(mjb_codepoint_is_xid_start); // Added by the script
    // Letter A is XID_Start: yes
    // std::cout << "Letter A is XID_Start: " << (mjb::is_xid_start('A') ? "yes" : "no") << std::endl;
    oss << "Letter A is XID_Start: " << (mjb::is_xid_start('A') ? "yes" : "no") << std::endl; // Added by the script
    ATT_ASSERT(oss.str(), "Letter A is XID_Start: yes\n", "mjb_codepoint_is_xid_start test failed"); // Added by the script
    oss.str(""); // Added by the script
    RESET_STATE()
}

{
    // Example for mjb_codepoint_is_xid_continue
    MJB_TEST_COVERAGE(mjb_codepoint_is_xid_continue); // Added by the script
    // Underscore is XID_Continue: yes
    // std::cout << "Underscore is XID_Continue: "
    //     << (mjb::is_xid_continue('_') ? "yes" : "no") << std::endl;
    oss << "Underscore is XID_Continue: " // Added by the script
        << (mjb::is_xid_continue('_') ? "yes" : "no") << std::endl; // Added by the script
    ATT_ASSERT(oss.str(), "Underscore is XID_Continue: yes\n", "mjb_codepoint_is_xid_continue test failed"); // Added by the script
    oss.str(""); // Added by the script
    RESET_STATE()
}

{
    // Example for mjb_codepoint_is_pattern_syntax
    MJB_TEST_COVERAGE(mjb_codepoint_is_pattern_syntax); // Added by the script
    // Plus sign is Pattern_Syntax: yes
    // std::cout << "Plus sign is Pattern_Syntax: "
    //     << (mjb::is_pattern_syntax('+') ? "yes" : "no") << std::endl;
    oss << "Plus sign is Pattern_Syntax: " // Added by the script
        << (mjb::is_pattern_syntax('+') ? "yes" : "no") << std::endl; // Added by the script
    ATT_ASSERT(oss.str(), "Plus sign is Pattern_Syntax: yes\n", "mjb_codepoint_is_pattern_syntax test failed"); // Added by the script
    oss.str(""); // Added by the script
    RESET_STATE()
}

{
    // Example for mjb_codepoint_is_pattern_white_space
    MJB_TEST_COVERAGE(mjb_codepoint_is_pattern_white_space); // Added by the script
    // Space is Pattern_White_Space: yes
    // std::cout << "Space is Pattern_White_Space: "
    //     << (mjb::is_pattern_white_space(' ') ? "yes" : "no") << std::endl;
    oss << "Space is Pattern_White_Space: " // Added by the script
        << (mjb::is_pattern_white_space(' ') ? "yes" : "no") << std::endl; // Added by the script
    ATT_ASSERT(oss.str(), "Space is Pattern_White_Space: yes\n", "mjb_codepoint_is_pattern_white_space test failed"); // Added by the script
    oss.str(""); // Added by the script
    RESET_STATE()
}

{
    // Example for mjb_codepoint_is_emoji
    MJB_TEST_COVERAGE(mjb_codepoint_is_emoji); // Added by the script
    // Number sign has the Emoji property: yes
    // std::cout << "Number sign has the Emoji property: "
    //     << (mjb::is_emoji('#') ? "yes" : "no") << std::endl;
    oss << "Number sign has the Emoji property: " // Added by the script
        << (mjb::is_emoji('#') ? "yes" : "no") << std::endl; // Added by the script
    ATT_ASSERT(oss.str(), "Number sign has the Emoji property: yes\n", "mjb_codepoint_is_emoji test failed"); // Added by the script
    oss.str(""); // Added by the script
    RESET_STATE()
}

{
    // Example for mjb_codepoint_is_emoji_presentation
    MJB_TEST_COVERAGE(mjb_codepoint_is_emoji_presentation); // Added by the script
    // Grinning face defaults to emoji presentation: yes
    // std::cout << "Grinning face defaults to emoji presentation: "
    //     << (mjb::is_emoji_presentation(0x1F600) ? "yes" : "no") << std::endl;
    oss << "Grinning face defaults to emoji presentation: " // Added by the script
        << (mjb::is_emoji_presentation(0x1F600) ? "yes" : "no") << std::endl; // Added by the script
    ATT_ASSERT(oss.str(), "Grinning face defaults to emoji presentation: yes\n", "mjb_codepoint_is_emoji_presentation test failed"); // Added by the script
    oss.str(""); // Added by the script
    RESET_STATE()
}

{
    // Example for mjb_codepoint_is_emoji_modifier
    MJB_TEST_COVERAGE(mjb_codepoint_is_emoji_modifier); // Added by the script
    // Medium skin tone is an emoji modifier: yes
    // std::cout << "Medium skin tone is an emoji modifier: "
    //     << (mjb::is_emoji_modifier(0x1F3FD) ? "yes" : "no") << std::endl;
    oss << "Medium skin tone is an emoji modifier: " // Added by the script
        << (mjb::is_emoji_modifier(0x1F3FD) ? "yes" : "no") << std::endl; // Added by the script
    ATT_ASSERT(oss.str(), "Medium skin tone is an emoji modifier: yes\n", "mjb_codepoint_is_emoji_modifier test failed"); // Added by the script
    oss.str(""); // Added by the script
    RESET_STATE()
}

{
    // Example for mjb_codepoint_is_emoji_modifier_base
    MJB_TEST_COVERAGE(mjb_codepoint_is_emoji_modifier_base); // Added by the script
    // Waving hand accepts an emoji modifier: yes
    // std::cout << "Waving hand accepts an emoji modifier: "
    //     << (mjb::is_emoji_modifier_base(0x1F44B) ? "yes" : "no") << std::endl;
    oss << "Waving hand accepts an emoji modifier: " // Added by the script
        << (mjb::is_emoji_modifier_base(0x1F44B) ? "yes" : "no") << std::endl; // Added by the script
    ATT_ASSERT(oss.str(), "Waving hand accepts an emoji modifier: yes\n", "mjb_codepoint_is_emoji_modifier_base test failed"); // Added by the script
    oss.str(""); // Added by the script
    RESET_STATE()
}

{
    // Example for mjb_codepoint_is_emoji_component
    MJB_TEST_COVERAGE(mjb_codepoint_is_emoji_component); // Added by the script
    // Zero-width joiner is an emoji component: yes
    // std::cout << "Zero-width joiner is an emoji component: "
    //     << (mjb::is_emoji_component(0x200D) ? "yes" : "no") << std::endl;
    oss << "Zero-width joiner is an emoji component: " // Added by the script
        << (mjb::is_emoji_component(0x200D) ? "yes" : "no") << std::endl; // Added by the script
    ATT_ASSERT(oss.str(), "Zero-width joiner is an emoji component: yes\n", "mjb_codepoint_is_emoji_component test failed"); // Added by the script
    oss.str(""); // Added by the script
    RESET_STATE()
}

{
    // Example for mjb_codepoint_is_extended_pictographic
    MJB_TEST_COVERAGE(mjb_codepoint_is_extended_pictographic); // Added by the script
    // Red heart is Extended_Pictographic: yes
    // std::cout << "Red heart is Extended_Pictographic: "
    //     << (mjb::is_extended_pictographic(0x2764) ? "yes" : "no") << std::endl;
    oss << "Red heart is Extended_Pictographic: " // Added by the script
        << (mjb::is_extended_pictographic(0x2764) ? "yes" : "no") << std::endl; // Added by the script
    ATT_ASSERT(oss.str(), "Red heart is Extended_Pictographic: yes\n", "mjb_codepoint_is_extended_pictographic test failed"); // Added by the script
    oss.str(""); // Added by the script
    RESET_STATE()
}
    // End of generated examples.
#undef RESET_STATE
}
// clang-format on
