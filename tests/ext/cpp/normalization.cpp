/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "../../../src/cpp/mojibake.hpp"
#include "../../test.h"

#include <utility>

int test_cpp_normalization(void *arg) {
    ATT_ASSERT(mjb::nfc(""), std::string(""), "nfc(\"\")")
    ATT_ASSERT(mjb::nfd(""), std::string(""), "nfd(\"\")")
    ATT_ASSERT(mjb::nfkc(""), std::string(""), "nfkc(\"\")")
    ATT_ASSERT(mjb::nfkd(""), std::string(""), "nfkd(\"\")")

    ATT_ASSERT(mjb::nfc("a"), std::string("a"), "nfc(\"a\")")
    ATT_ASSERT(mjb::nfd("a"), std::string("a"), "nfd(\"a\")")
    ATT_ASSERT(mjb::nfkc("a"), std::string("a"), "nfkc(\"a\")")
    ATT_ASSERT(mjb::nfkd("a"), std::string("a"), "nfkd(\"a\")")
    ATT_ASSERT(mjb::nfkc_casefold("Stra\xC3\x9F" "e\xC2\xAD"), std::string("strasse"),
        "nfkc_casefold")

    ATT_ASSERT((int)mjb::normalization_quick_check("a", mjb::NormalizationForm::NFC),
        MJB_QC_YES, "normalization_quick_check")
    ATT_ASSERT(mjb::filter("a  b", MJB_FILTER_COLLAPSE_SPACES), std::string("a b"),
        "filter")
    ATT_ASSERT(mjb::filter("a\x01  b", mjb::Filter::Controls | mjb::Filter::CollapseSpaces),
        std::string("a b"), "filter flags")
    ATT_ASSERT(mjb::uppercase("Stra\xC3\x9F" "e"), std::string("STRASSE"), "uppercase")
    ATT_ASSERT(mjb::lowercase("HELLO"), std::string("hello"), "lowercase")
    ATT_ASSERT(mjb::titlecase("hello world"), std::string("Hello World"), "titlecase")
    ATT_ASSERT(mjb::casefold("Stra\xC3\x9F" "e"), std::string("strasse"), "casefold")
    ATT_ASSERT(mjb::casefold_simple("\xE1\xBA\x9E"), std::string("\xC3\x9F"),
        "casefold_simple")

    ATT_ASSERT((int)mjb::detect_encoding("abc"), MJB_ENC_ASCII | MJB_ENC_UTF_8,
        "detect_encoding")
    ATT_ASSERT(mjb::is_ascii("abc"), true, "is_ascii")
    ATT_ASSERT(mjb::is_utf8("caf\xC3\xA9"), true, "is_utf8")

    const std::string utf16le("a\0b\0", 4);
    ATT_ASSERT(mjb::is_utf16(utf16le), true, "is_utf16")
    ATT_ASSERT(mjb::length("caf\xC3\xA9"), 4u, "length")
    ATT_ASSERT(mjb::convert_encoding("\xC3\xA9", MJB_ENC_UTF_8, MJB_ENC_UTF_16LE),
        std::string("\xE9\0", 2), "convert_encoding")
    ATT_ASSERT(mjb::compare("a", "b") < 0, true, "compare")

    const std::string already_normalized("plain");
    auto view_result = mjb::normalize_result(already_normalized, mjb::NormalizationForm::NFC);
    ATT_ASSERT(view_result.transformed(), false, "TextResult zero-copy result")
    ATT_ASSERT(view_result.view().data() == already_normalized.data(), true,
        "TextResult aliases untransformed input")
    ATT_ASSERT(view_result.str(), already_normalized, "TextResult::str")

    auto owned_result = mjb::normalize_result("e\xCC\x81", mjb::NormalizationForm::NFC);
    ATT_ASSERT(owned_result.transformed(), true, "TextResult owns transformed result")
    ATT_ASSERT(owned_result.str(), std::string("\xC3\xA9"), "TextResult transformed output")

    auto moved_result = std::move(owned_result);
    ATT_ASSERT(owned_result.empty(), true, "TextResult moved-from state")
    ATT_ASSERT(moved_result.str(), std::string("\xC3\xA9"), "TextResult move constructor")

    bool caught = false;

    try {
        (void)mjb::Character(static_cast<mjb_codepoint>(MJB_CODEPOINT_NOT_VALID));
    } catch(const mjb::LibraryError &error) {
        caught = error.status() == MJB_STATUS_INVALID_ARGUMENT;
    }

    ATT_ASSERT(caught, true, "LibraryError preserves mjb_status")

    const std::string malformed_utf8("\x80", 1);
    caught = false;

    try {
        (void)mjb::normalization_quick_check(malformed_utf8, mjb::NormalizationForm::NFC);
    } catch(const mjb::LibraryError &error) {
        caught = error.status() == MJB_STATUS_MALFORMED_INPUT;
    }

    ATT_ASSERT(caught, true, "normalization_quick_check preserves malformed-input status")

    caught = false;

    try {
        (void)mjb::compare(malformed_utf8, "a");
    } catch(const mjb::LibraryError &error) {
        caught = error.status() == MJB_STATUS_MALFORMED_INPUT;
    }

    ATT_ASSERT(caught, true, "compare preserves malformed-input status")

    caught = false;

    try {
        (void)mjb::is_confusable(malformed_utf8, "A");
    } catch(const mjb::LibraryError &error) {
        caught = error.status() == MJB_STATUS_MALFORMED_INPUT;
    }

    ATT_ASSERT(caught, true, "is_confusable preserves malformed-input status")

    return 0;
}
