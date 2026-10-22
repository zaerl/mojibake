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
    oss.clear();
    oss.copyfmt(std::ostringstream());
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
    oss.clear();
    oss.copyfmt(std::ostringstream());
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
    oss.clear();
    oss.copyfmt(std::ostringstream());
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
    oss.clear();
    oss.copyfmt(std::ostringstream());
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
    oss.clear();
    oss.copyfmt(std::ostringstream());
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
    oss.clear();
    oss.copyfmt(std::ostringstream());
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
    oss.clear();
    oss.copyfmt(std::ostringstream());
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
    oss.clear();
    oss.copyfmt(std::ostringstream());
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
    oss.clear();
    oss.copyfmt(std::ostringstream());
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
    oss.clear();
    oss.copyfmt(std::ostringstream());
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
    oss.clear();
    oss.copyfmt(std::ostringstream());
}
#endif // MJB_FEATURE_IDNA
    // End of generated examples.
}
// clang-format on
