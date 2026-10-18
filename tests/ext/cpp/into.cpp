/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "../../../src/cpp/mojibake.hpp"
#include "../../test.h"

#include <string>

// The _into transforms write the same text as the functions that return a new string, replace
// the content of out, and reuse its storage
ATT_TEST(cpp_into) {
    std::string out("previous content that must be replaced");

    mjb::nfc_into("Cafe\xCC\x81", out);
    ATT_ASSERT(out, std::string("Caf\xC3\xA9"), "nfc_into replaces out");
    mjb::nfd_into("Caf\xC3\xA9", out);
    ATT_ASSERT(out, mjb::nfd("Caf\xC3\xA9"), "nfd_into");
    mjb::nfkc_into("\xEF\xAC\x81", out);
    ATT_ASSERT(out, std::string("fi"), "nfkc_into");
    mjb::nfkd_into("\xC3\x85", out);
    ATT_ASSERT(out, std::string("A\xCC\x8A"), "nfkd_into");
    mjb::normalize_into("e\xCC\x81", mjb::NormalizationForm::NFC, out);
    ATT_ASSERT(out, std::string("\xC3\xA9"), "normalize_into");
    mjb::nfc_into("", out);
    ATT_ASSERT(out, std::string(""), "nfc_into of empty input");

    // A std::string input selects the UTF-8 overload, not the UTF-16 and UTF-32 template
    const std::string owned("Cafe\xCC\x81");
    mjb::nfc_into(owned, out);
    ATT_ASSERT(out, std::string("Caf\xC3\xA9"), "nfc_into from a std::string writes UTF-8");

    mjb::filter_into("a  b", MJB_FILTER_COLLAPSE_SPACES, out);
    ATT_ASSERT(out, std::string("a b"), "filter_into");
    mjb::filter_into("a\x01  b", mjb::Filter::Controls | mjb::Filter::CollapseSpaces, out);
    ATT_ASSERT(out, std::string("a b"), "filter_into with Filter flags");
    mjb::nfkc_casefold_into("Stra\xC3\x9F"
                            "e\xC2\xAD",
        out);
    ATT_ASSERT(out, std::string("strasse"), "nfkc_casefold_into");

    mjb::case_map_into("hello", MJB_CASE_UPPER, out);
    ATT_ASSERT(out, std::string("HELLO"), "case_map_into");
    mjb::uppercase_into("Stra\xC3\x9F"
                        "e",
        out);
    ATT_ASSERT(out, std::string("STRASSE"), "uppercase_into grows the output");
    mjb::uppercase_into("istanbul", out, MJB_LOCALE_TR);
    ATT_ASSERT(out, std::string("\xC4\xB0STANBUL"), "uppercase_into with a locale");
    mjb::lowercase_into("HELLO", out);
    ATT_ASSERT(out, std::string("hello"), "lowercase_into");
    mjb::titlecase_into("hello world", out);
    ATT_ASSERT(out, std::string("Hello World"), "titlecase_into");
    mjb::casefold_into("Stra\xC3\x9F"
                       "e",
        out);
    ATT_ASSERT(out, std::string("strasse"), "casefold_into");
    mjb::casefold_simple_into("\xE1\xBA\x9E", out);
    ATT_ASSERT(out, std::string("\xC3\x9F"), "casefold_simple_into");

    // Another output encoding, written as bytes
    mjb::convert_encoding_into("AB", MJB_ENC_UTF_8, MJB_ENC_UTF_16BE, out);
    ATT_ASSERT(out, std::string("\0A\0B", 4), "convert_encoding_into");
    mjb::convert_encoding_into("A", MJB_ENC_UTF_8, MJB_ENC_UTF_32LE, out);
    ATT_ASSERT(out, std::string("A\0\0\0", 4), "convert_encoding_into grows past the hint");
    mjb::nfc_into("e\xCC\x81", out, MJB_ENC_UTF_8, MJB_ENC_UTF_16LE);
    ATT_ASSERT(out, std::string("\xE9\0", 2), "nfc_into with an output encoding");

    // Output that outgrows the first attempt takes a second pass: Hangul NFD triples UTF-8
    std::string hangul;
    for(mjb_codepoint codepoint = 0xAC00; codepoint < 0xAC40; ++codepoint) {
        hangul += mjb::encode(codepoint);
    }

    std::string grown;
    mjb::nfd_into(hangul, grown);
    ATT_ASSERT(grown, mjb::nfd(hangul), "nfd_into of Hangul outgrows the first attempt");
    ATT_ASSERT(grown.size() > hangul.size() * 3 / 2, true, "nfd_into of Hangul needs a second pass");

    // Enough capacity is reused without reallocating
    std::string reused;
    reused.reserve(256);
    const char *storage = reused.data();
    mjb::nfc_into("Cafe\xCC\x81", reused);
    mjb::uppercase_into("hello world", reused);
    ATT_ASSERT(reused.data() == storage, true, "_into reuses the capacity of out");
    ATT_ASSERT(reused, std::string("HELLO WORLD"), "_into into reused storage");

    // On failure out is left empty and the status is preserved
    std::string failed("unchanged?");
    mjb_status status = MJB_STATUS_OK;

    try {
        mjb::nfc_into("a\x80", failed);
    } catch(const mjb::LibraryError &error) {
        status = error.status();
    }

    ATT_ASSERT_STATUS(status, MJB_STATUS_MALFORMED_INPUT, "nfc_into throws on malformed input");
    ATT_ASSERT(failed.empty(), true, "nfc_into leaves out empty on failure");
    mjb::nfc_into("a\x80", failed, MJB_ENC_UTF_8, MJB_ENC_UTF_8, MJB_MALFORMED_REPLACE);
    ATT_ASSERT(failed, std::string("a\xEF\xBF\xBD"), "nfc_into with the replace policy");

#if MJB_FEATURE_SECURITY
    mjb::confusable_skeleton_into("h\xD0\xB5llo", out);
    ATT_ASSERT(out, mjb::confusable_skeleton("hello"), "confusable_skeleton_into");
#endif

#if MJB_FEATURE_IDNA
    ATT_ASSERT(mjb::idna_to_ascii_into("b\xC3\xBC" "cher.example", out), (uint32_t)MJB_IDNA_ERROR_NONE,
        "idna_to_ascii_into errors");
    ATT_ASSERT(out, std::string("xn--bcher-kva.example"), "idna_to_ascii_into");
    ATT_ASSERT(mjb::idna_to_unicode_into("xn--bcher-kva.example", out),
        (uint32_t)MJB_IDNA_ERROR_NONE, "idna_to_unicode_into errors");
    ATT_ASSERT(out, std::string("b\xC3\xBC" "cher.example"), "idna_to_unicode_into");
    ATT_ASSERT((mjb::idna_to_ascii_into("a..b", out) & MJB_IDNA_ERROR_EMPTY_LABEL) != 0, true,
        "idna_to_ascii_into reports error flags");
#endif

#if MJB_FEATURE_COLLATION
    mjb::collation_key_into("apple", out);
    ATT_ASSERT(out, mjb::collation_key("apple"), "collation_key_into");
    mjb::collation_key_into(u"apple", out);
    ATT_ASSERT(out, mjb::collation_key("apple"), "collation_key_into from UTF-16");
    mjb::collation_key_into("", out);
    ATT_ASSERT(out.empty(), true, "collation_key_into of empty input");
#endif

    // UTF-16 and UTF-32 write into a string of the input's type. Attractor does not print
    // char16_t or char32_t strings, so results are compared with == and asserted as booleans.
    std::u16string out16(u"previous");
    std::u32string out32;

    mjb::nfc_into(u"e\u0301", out16);
    ATT_ASSERT(out16 == u"\u00E9", true, "UTF-16 nfc_into");
    mjb::nfd_into(U"\u00E9", out32);
    ATT_ASSERT(out32 == U"e\u0301", true, "UTF-32 nfd_into");
    mjb::nfkc_into(u"\uFB01", out16);
    ATT_ASSERT(out16 == u"fi", true, "UTF-16 nfkc_into");
    mjb::nfkd_into(U"\u00C5", out32);
    ATT_ASSERT(out32 == U"A\u030A", true, "UTF-32 nfkd_into");
    mjb::normalize_into(u"e\u0301", mjb::NormalizationForm::NFC, out16);
    ATT_ASSERT(out16 == u"\u00E9", true, "UTF-16 normalize_into");
    mjb::uppercase_into(u"stra\u00DFe", out16);
    ATT_ASSERT(out16 == u"STRASSE", true, "UTF-16 uppercase_into");
    mjb::uppercase_into(U"istanbul", out32, MJB_LOCALE_TR);
    ATT_ASSERT(out32 == U"\u0130STANBUL", true, "UTF-32 uppercase_into with a locale");
    mjb::lowercase_into(u"HELLO", out16);
    ATT_ASSERT(out16 == u"hello", true, "UTF-16 lowercase_into");
    mjb::titlecase_into(U"hello world", out32);
    ATT_ASSERT(out32 == U"Hello World", true, "UTF-32 titlecase_into");
    mjb::casefold_into(u"Stra\u00DFe", out16);
    ATT_ASSERT(out16 == u"strasse", true, "UTF-16 casefold_into");
    mjb::casefold_simple_into(U"\u1E9E", out32);
    ATT_ASSERT(out32 == U"\u00DF", true, "UTF-32 casefold_simple_into");
    mjb::case_map_into(u"abc", MJB_CASE_UPPER, out16);
    ATT_ASSERT(out16 == u"ABC", true, "UTF-16 case_map_into");
    mjb::filter_into(u"e\u0301", mjb::Filter::Normalize, out16);
    ATT_ASSERT(out16 == u"\u00E9", true, "UTF-16 filter_into");
    mjb::filter_into(U"a  b", MJB_FILTER_COLLAPSE_SPACES, out32);
    ATT_ASSERT(out32 == U"a b", true, "UTF-32 filter_into");
    mjb::nfkc_casefold_into(u"Stra\u00DFe\u00AD", out16);
    ATT_ASSERT(out16 == u"strasse", true, "UTF-16 nfkc_casefold_into");
    mjb::nfc_into(u"A\U0001F642", out16);
    ATT_ASSERT(out16 == u"A\U0001F642", true, "UTF-16 nfc_into keeps surrogate pairs");
#if MJB_FEATURE_SECURITY
    mjb::confusable_skeleton_into(u"h\u0435llo", out16);
    ATT_ASSERT(out16 == mjb::confusable_skeleton(u"hello"), true,
        "UTF-16 confusable_skeleton_into");
#endif

    std::u16string failed16(u"unchanged?");

    try {
        mjb::nfc_into(std::u16string(1, static_cast<char16_t>(0xD800)), failed16);
    } catch(const mjb::LibraryError &error) {
        status = error.status();
    }

    ATT_ASSERT(failed16.empty(), true, "UTF-16 nfc_into leaves out empty on failure");
}
