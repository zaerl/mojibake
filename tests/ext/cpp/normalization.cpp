/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "../../../src/cpp/mojibake.hpp"
#include "../../test.h"

#include <string>
#include <system_error>

ATT_TEST(cpp_normalization) {
    ATT_ASSERT(mjb::nfc(""), std::string(""), "nfc(\"\")");
    ATT_ASSERT(mjb::nfd(""), std::string(""), "nfd(\"\")");
    ATT_ASSERT(mjb::nfkc(""), std::string(""), "nfkc(\"\")");
    ATT_ASSERT(mjb::nfkd(""), std::string(""), "nfkd(\"\")");

    ATT_ASSERT(mjb::nfc("a"), std::string("a"), "nfc(\"a\")");
    ATT_ASSERT(mjb::nfd("a"), std::string("a"), "nfd(\"a\")");
    ATT_ASSERT(mjb::nfkc("a"), std::string("a"), "nfkc(\"a\")");
    ATT_ASSERT(mjb::nfkd("a"), std::string("a"), "nfkd(\"a\")");
    ATT_ASSERT(mjb::nfkc_casefold("Stra\xC3\x9F" "e\xC2\xAD"), std::string("strasse"),
        "nfkc_casefold");

    ATT_ASSERT((int)mjb::normalization_quick_check("a", mjb::NormalizationForm::NFC),
        MJB_QC_YES, "normalization_quick_check");
    ATT_ASSERT(mjb::filter("a  b", mjb::Filter::CollapseSpaces), std::string("a b"),
        "filter");
    ATT_ASSERT(mjb::filter("a\x01  b", mjb::Filter::Controls | mjb::Filter::CollapseSpaces),
        std::string("a b"), "filter flags");
    ATT_ASSERT(mjb::uppercase("Stra\xC3\x9F" "e"), std::string("STRASSE"), "uppercase");
    ATT_ASSERT(mjb::lowercase("HELLO"), std::string("hello"), "lowercase");
    ATT_ASSERT(mjb::titlecase("hello world"), std::string("Hello World"), "titlecase");
    ATT_ASSERT(mjb::casefold("Stra\xC3\x9F" "e"), std::string("strasse"), "casefold");
    ATT_ASSERT(mjb::casefold_simple("\xE1\xBA\x9E"), std::string("\xC3\x9F"),
        "casefold_simple");
    ATT_ASSERT(mjb::caseless_match("Stra\xC3\x9F" "e", "STRASSE"), true, "caseless_match");
    ATT_ASSERT(mjb::caseless_match("Stra\x80\xC3\x9F" "e", "STRASSE", { .malformed = MJB_MALFORMED_SKIP }),
        true, "caseless_match skips malformed input");
    ATT_ASSERT(mjb::caseless_match("\xC3\x85", "A\xCC\x8A", mjb::CaselessMode::Unnormalized),
        false, "caseless_match unnormalized");
    ATT_ASSERT(mjb::caseless_match("\xC3\x85", "A\xCC\x8A"), true,
        "caseless_match canonical");

    ATT_ASSERT((int)mjb::detect_encoding("abc"), MJB_ENC_ASCII | MJB_ENC_UTF_8,
        "detect_encoding");
    ATT_ASSERT(mjb::is_ascii("abc"), true, "is_ascii");
    ATT_ASSERT(mjb::is_utf8("caf\xC3\xA9"), true, "is_utf8");

    const std::string utf16le("a\0b\0", 4);
    ATT_ASSERT(mjb::is_utf16(utf16le), true, "is_utf16");
    mjb_diagnostic diagnostic;
    ATT_ASSERT_STATUS(mjb::validate_encoding("A\x80", &diagnostic),
        MJB_STATUS_MALFORMED_INPUT, "validate_string malformed input");
    ATT_ASSERT((unsigned int)diagnostic.error,
        (unsigned int)MJB_TEXT_ERROR_UNEXPECTED_CONTINUATION,
        "validate_string diagnostic");

    const std::string decodable("A\x80"
                                "B",
        3);
    size_t offset = 0;
    mjb_codepoint codepoint = MJB_CODEPOINT_NOT_VALID;
    ATT_ASSERT_STATUS(mjb::decode_next(decodable, offset, codepoint), MJB_STATUS_OK,
        "decode_next valid input");
    ATT_ASSERT(codepoint, (mjb_codepoint)'A', "decode_next codepoint");
    ATT_ASSERT_STATUS(mjb::decode_next(decodable, offset, codepoint,
                          { .malformed = MJB_MALFORMED_REPLACE, .diagnostic = &diagnostic }),
        MJB_STATUS_OK, "decode_next replacement policy");
    ATT_ASSERT(codepoint, (mjb_codepoint)MJB_CODEPOINT_REPLACEMENT,
        "decode_next replacement codepoint");
    offset = decodable.size();
    ATT_ASSERT_STATUS(mjb::decode_previous(decodable, offset, codepoint), MJB_STATUS_OK,
        "decode_previous valid input");
    ATT_ASSERT(codepoint, (mjb_codepoint)'B', "decode_previous codepoint");

    ATT_ASSERT(mjb::nfc(decodable, { .malformed = MJB_MALFORMED_REPLACE, .diagnostic = &diagnostic }),
        std::string("A\xEF\xBF\xBD"
                    "B"),
        "nfc replacement policy");
    ATT_ASSERT(diagnostic.byte_offset, (size_t)1, "nfc replacement diagnostic");
    ATT_ASSERT(mjb::nfc(decodable, { .malformed = MJB_MALFORMED_SKIP }),
        std::string("AB"), "nfc skip policy");
    ATT_ASSERT(mjb::nfkc_casefold(decodable, { .malformed = MJB_MALFORMED_SKIP }),
        std::string("ab"), "nfkc_casefold skip policy");
    ATT_ASSERT(mjb::uppercase(decodable, { .malformed = MJB_MALFORMED_REPLACE }),
        std::string("A\xEF\xBF\xBD"
                    "B"),
        "uppercase replacement policy");
    ATT_ASSERT(mjb::grapheme_count(decodable, { .malformed = MJB_MALFORMED_REPLACE }),
        (size_t)3, "grapheme_count replacement policy");
    ATT_ASSERT(mjb::sentence_count(decodable, { .malformed = MJB_MALFORMED_SKIP }), (size_t)1,
        "sentence_count skip policy");
    ATT_ASSERT(mjb::word_count(decodable, { .malformed = MJB_MALFORMED_REPLACE }), (size_t)2,
        "word_count replacement policy");
    ATT_ASSERT(mjb::terminal_width(decodable, MJB_TERMINAL_WIDTH_NARROW,
                   { .malformed = MJB_MALFORMED_SKIP }),
        (size_t)2, "terminal_width skip policy");

    ATT_ASSERT(mjb::codepoint_count("caf\xC3\xA9"), (size_t)4, "codepoint_count");
    ATT_ASSERT(mjb::convert_encoding("\xC3\xA9", MJB_ENC_UTF_16LE),
        std::string("\xE9\0", 2), "convert_encoding");
#if MJB_FEATURE_COLLATION
    ATT_ASSERT(mjb::collation_key(decodable, { .malformed = MJB_MALFORMED_SKIP }),
        mjb::collation_key("AB"), "collation_key skip policy");
    ATT_ASSERT(mjb::collation_compare("a", "b") < 0, true, "compare");
    ATT_ASSERT(mjb::collation_compare("ca\x80t", "cat", { .malformed = MJB_MALFORMED_SKIP }),
        0, "compare skips malformed input");
    ATT_ASSERT(mjb::collation_compare("A", "a", { .strength = mjb::CollationStrength::Secondary }),
        0, "compare secondary ignores case");
    ATT_ASSERT((int)(mjb::collation_compare("A", "a", { .strength = mjb::CollationStrength::Tertiary }) != 0),
        1, "compare tertiary compares case");
    ATT_ASSERT(mjb::collation_compare("", "\xE2\x80\x8B"), 0,
        "compare completely ignorable string with empty");
    ATT_ASSERT(mjb::collation_compare("\xE2\x80\x8B", ""), 0,
        "compare completely ignorable string with empty in reverse order");
    ATT_ASSERT(mjb::collation_compare("", "\xCC\x81", { .strength = mjb::CollationStrength::Primary }),
        0, "compare primary-ignorable accent with empty");
    ATT_ASSERT((int)(mjb::collation_compare("", "\xCC\x81",
                        { .strength = mjb::CollationStrength::Secondary }) != 0),
        1, "compare accent with empty at secondary strength");
#endif

#if MJB_FEATURE_COLLATION
    // Shifted variable weighting ignores punctuation at the first three levels
    ATT_ASSERT(mjb::collation_compare("de-luge", "deluge",
                   { .strength = mjb::CollationStrength::Tertiary,
                       .variable_weighting = mjb::CollationVariableWeighting::Shifted }),
        0, "compare shifted ignores punctuation");
    ATT_ASSERT(mjb::collation_compare("de-luge", "deluge") != 0, true,
        "compare non-ignorable keeps punctuation");
#endif

    // Errors are std::system_error values in the mojibake category
    const std::error_code code = mjb::make_error_code(MJB_STATUS_MALFORMED_INPUT);
    ATT_ASSERT(std::string(code.category().name()), std::string("mojibake"), "error category name");
    ATT_ASSERT(code.message(), std::string(mjb::status_message(MJB_STATUS_MALFORMED_INPUT)),
        "error category message");

    bool caught = false;

    std::string error_message;

    try {
        (void)mjb::Character(static_cast<mjb_codepoint>(MJB_CODEPOINT_NOT_VALID));
    } catch(const mjb::LibraryError &error) {
        caught = error.status() == MJB_STATUS_INVALID_ARGUMENT;
        error_message = error.what();
    }

    ATT_ASSERT(caught, true, "LibraryError preserves mjb_status");
    ATT_ASSERT(error_message,
        std::string("Unable to read codepoint character: ") +
            std::string(mjb::status_message(MJB_STATUS_INVALID_ARGUMENT)),
        "LibraryError appends the library status message");
    ATT_ASSERT(std::string(mjb::status_message(MJB_STATUS_OK)),
        std::string("The operation completed successfully"), "status_message");

    const std::string malformed_utf8("\x80", 1);
    caught = false;

    try {
        (void)mjb::normalization_quick_check(malformed_utf8, mjb::NormalizationForm::NFC);
    } catch(const mjb::LibraryError &error) {
        caught = error.status() == MJB_STATUS_MALFORMED_INPUT;
    }

    ATT_ASSERT(caught, true, "normalization_quick_check preserves malformed-input status");

#if MJB_FEATURE_COLLATION
    caught = false;
    try {
        (void)mjb::collation_compare(malformed_utf8, "a");
    } catch(const mjb::LibraryError &error) {
        caught = error.status() == MJB_STATUS_MALFORMED_INPUT;
    }

    ATT_ASSERT(caught, true, "compare preserves malformed-input status");
#endif

    caught = false;

    try {
        (void)mjb::caseless_match(malformed_utf8, "a");
    } catch(const mjb::LibraryError &error) {
        caught = error.status() == MJB_STATUS_MALFORMED_INPUT;
    }

    ATT_ASSERT(caught, true, "caseless_match preserves malformed-input status");

#if MJB_FEATURE_SECURITY
    caught = false;

    try {
        (void)mjb::confusable_match(malformed_utf8, "A");
    } catch(const mjb::LibraryError &error) {
        caught = error.status() == MJB_STATUS_MALFORMED_INPUT;
    }

    ATT_ASSERT(caught, true, "is_confusable preserves malformed-input status");
#endif
}
