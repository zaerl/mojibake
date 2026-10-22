/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "../../../src/cpp/mojibake.hpp"
#include "../../test.h"

#include <string>

// Transforms called with an output write the same text as the forms that return a new string,
// replace the content of the output, and reuse its storage
ATT_TEST(cpp_into) {
    std::string out("previous content that must be replaced");

    mjb::nfc("Cafe\xCC\x81", out);
    ATT_ASSERT(out, std::string("Caf\xC3\xA9"), "nfc into replaces out");
    mjb::nfd("Caf\xC3\xA9", out);
    ATT_ASSERT(out, mjb::nfd("Caf\xC3\xA9"), "nfd into");
    mjb::nfkc("\xEF\xAC\x81", out);
    ATT_ASSERT(out, std::string("fi"), "nfkc into");
    mjb::nfkd("\xC3\x85", out);
    ATT_ASSERT(out, std::string("A\xCC\x8A"), "nfkd into");
    mjb::normalize("e\xCC\x81", mjb::NormalizationForm::NFC, out);
    ATT_ASSERT(out, std::string("\xC3\xA9"), "normalize into");
    mjb::nfc("", out);
    ATT_ASSERT(out, std::string(""), "nfc into of empty input");

    const std::string owned("Cafe\xCC\x81");
    mjb::nfc(owned, out);
    ATT_ASSERT(out, std::string("Caf\xC3\xA9"), "nfc into from a std::string");

    mjb::filter("a  b", mjb::Filter::CollapseSpaces, out);
    ATT_ASSERT(out, std::string("a b"), "filter into");
    mjb::filter("a\x01  b", mjb::Filter::Controls | mjb::Filter::CollapseSpaces, out);
    ATT_ASSERT(out, std::string("a b"), "filter into with combined flags");
    mjb::nfkc_casefold("Stra\xC3\x9F"
                       "e\xC2\xAD",
        out);
    ATT_ASSERT(out, std::string("strasse"), "nfkc_casefold into");

    mjb::case_map("hello", MJB_CASE_UPPER, out);
    ATT_ASSERT(out, std::string("HELLO"), "case_map into");
    mjb::uppercase("Stra\xC3\x9F"
                   "e",
        out);
    ATT_ASSERT(out, std::string("STRASSE"), "uppercase into grows the output");
    mjb::uppercase("istanbul", out, { .locale = MJB_LOCALE_TR });
    ATT_ASSERT(out, std::string("\xC4\xB0STANBUL"), "uppercase into with a locale");
    mjb::lowercase("HELLO", out);
    ATT_ASSERT(out, std::string("hello"), "lowercase into");
    mjb::titlecase("hello world", out);
    ATT_ASSERT(out, std::string("Hello World"), "titlecase into");
    mjb::casefold("Stra\xC3\x9F"
                  "e",
        out);
    ATT_ASSERT(out, std::string("strasse"), "casefold into");
    mjb::casefold_simple("\xE1\xBA\x9E", out);
    ATT_ASSERT(out, std::string("\xC3\x9F"), "casefold_simple into");

    // Another output encoding, written as bytes
    mjb::convert_encoding("AB", mjb::encoded(out, MJB_ENC_UTF_16BE));
    ATT_ASSERT(out, std::string("\0A\0B", 4), "convert_encoding into bytes");
    mjb::convert_encoding("A", mjb::encoded(out, MJB_ENC_UTF_32LE));
    ATT_ASSERT(out, std::string("A\0\0\0", 4), "convert_encoding grows past the hint");
    mjb::nfc("e\xCC\x81", mjb::encoded(out, MJB_ENC_UTF_16LE));
    ATT_ASSERT(out, std::string("\xE9\0", 2), "nfc into bytes of another encoding");

    // The output's string type selects its encoding, whatever the input's
    std::u16string converted16;
    mjb::convert_encoding("A\xC3\xA9", converted16);
    ATT_ASSERT(converted16 == u"A\u00E9", true, "convert_encoding into a std::u16string");
    mjb::nfc(U"e\u0301", out);
    ATT_ASSERT(out, std::string("\xC3\xA9"), "nfc of UTF-32 into a UTF-8 std::string");

    // Output that outgrows the first attempt takes a second pass: Hangul NFD triples UTF-8
    std::string hangul;
    for(mjb_codepoint codepoint = 0xAC00; codepoint < 0xAC40; ++codepoint) {
        hangul += mjb::encode(codepoint);
    }

    std::string grown;
    mjb::nfd(hangul, grown);
    ATT_ASSERT(grown, mjb::nfd(hangul), "nfd into of Hangul outgrows the first attempt");
    ATT_ASSERT(grown.size() > hangul.size() * 3 / 2, true, "nfd into of Hangul needs a second pass");

    // Enough capacity is reused without reallocating
    std::string reused;
    reused.reserve(256);
    const char *storage = reused.data();
    mjb::nfc("Cafe\xCC\x81", reused);
    mjb::uppercase("hello world", reused);
    ATT_ASSERT(reused.data() == storage, true, "transforms reuse the capacity of out");
    ATT_ASSERT(reused, std::string("HELLO WORLD"), "transform into reused storage");

    // On failure out is left empty and the status is preserved
    std::string failed("unchanged?");
    mjb_status status = MJB_STATUS_OK;

    try {
        mjb::nfc("a\x80", failed);
    } catch(const mjb::LibraryError &error) {
        status = error.status();
    }

    ATT_ASSERT_STATUS(status, MJB_STATUS_MALFORMED_INPUT, "nfc into throws on malformed input");
    ATT_ASSERT(failed.empty(), true, "nfc into leaves out empty on failure");
    mjb::nfc("a\x80", failed, { .malformed = MJB_MALFORMED_REPLACE });
    ATT_ASSERT(failed, std::string("a\xEF\xBF\xBD"), "nfc into with the replace policy");

#if MJB_FEATURE_SECURITY
    mjb::confusable_skeleton("h\xD0\xB5llo", out);
    ATT_ASSERT(out, mjb::confusable_skeleton("hello"), "confusable_skeleton into");
#endif

#if MJB_FEATURE_IDNA
    ATT_ASSERT(mjb::idna_to_ascii("b\xC3\xBC"
                                  "cher.example",
                   out),
        (uint32_t)MJB_IDNA_ERROR_NONE, "idna_to_ascii into errors");
    ATT_ASSERT(out, std::string("xn--bcher-kva.example"), "idna_to_ascii into");
    ATT_ASSERT(mjb::idna_to_unicode("xn--bcher-kva.example", out), (uint32_t)MJB_IDNA_ERROR_NONE,
        "idna_to_unicode into errors");
    ATT_ASSERT(out, std::string("b\xC3\xBC"
                                "cher.example"),
        "idna_to_unicode into");
    ATT_ASSERT((mjb::idna_to_ascii("a..b", out) & MJB_IDNA_ERROR_EMPTY_LABEL) != 0, true,
        "idna_to_ascii into reports error flags");
#endif

#if MJB_FEATURE_COLLATION
    mjb::collation_key("apple", out);
    ATT_ASSERT(out, mjb::collation_key("apple"), "collation_key into");
    mjb::collation_key(u"apple", out);
    ATT_ASSERT(out, mjb::collation_key("apple"), "collation_key into from UTF-16");
    mjb::collation_key("", out);
    ATT_ASSERT(out.empty(), true, "collation_key into of empty input");
#endif

    // UTF-16 and UTF-32 write into a string of the input's type. Attractor does not print
    // char16_t or char32_t strings, so results are compared with == and asserted as booleans.
    std::u16string out16(u"previous");
    std::u32string out32;

    mjb::nfc(u"e\u0301", out16);
    ATT_ASSERT(out16 == u"\u00E9", true, "UTF-16 nfc into");
    mjb::nfd(U"\u00E9", out32);
    ATT_ASSERT(out32 == U"e\u0301", true, "UTF-32 nfd into");
    mjb::nfkc(u"\uFB01", out16);
    ATT_ASSERT(out16 == u"fi", true, "UTF-16 nfkc into");
    mjb::nfkd(U"\u00C5", out32);
    ATT_ASSERT(out32 == U"A\u030A", true, "UTF-32 nfkd into");
    mjb::normalize(u"e\u0301", mjb::NormalizationForm::NFC, out16);
    ATT_ASSERT(out16 == u"\u00E9", true, "UTF-16 normalize into");
    mjb::uppercase(u"stra\u00DFe", out16);
    ATT_ASSERT(out16 == u"STRASSE", true, "UTF-16 uppercase into");
    mjb::uppercase(U"istanbul", out32, { .locale = MJB_LOCALE_TR });
    ATT_ASSERT(out32 == U"\u0130STANBUL", true, "UTF-32 uppercase into with a locale");
    mjb::lowercase(u"HELLO", out16);
    ATT_ASSERT(out16 == u"hello", true, "UTF-16 lowercase into");
    mjb::titlecase(U"hello world", out32);
    ATT_ASSERT(out32 == U"Hello World", true, "UTF-32 titlecase into");
    mjb::casefold(u"Stra\u00DFe", out16);
    ATT_ASSERT(out16 == u"strasse", true, "UTF-16 casefold into");
    mjb::casefold_simple(U"\u1E9E", out32);
    ATT_ASSERT(out32 == U"\u00DF", true, "UTF-32 casefold_simple into");
    mjb::case_map(u"abc", MJB_CASE_UPPER, out16);
    ATT_ASSERT(out16 == u"ABC", true, "UTF-16 case_map into");
    mjb::filter(u"e\u0301", mjb::Filter::Normalize, out16);
    ATT_ASSERT(out16 == u"\u00E9", true, "UTF-16 filter into");
    mjb::filter(U"a  b", mjb::Filter::CollapseSpaces, out32);
    ATT_ASSERT(out32 == U"a b", true, "UTF-32 filter into");
    mjb::nfkc_casefold(u"Stra\u00DFe\u00AD", out16);
    ATT_ASSERT(out16 == u"strasse", true, "UTF-16 nfkc_casefold into");
    mjb::nfc(u"A\U0001F642", out16);
    ATT_ASSERT(out16 == u"A\U0001F642", true, "UTF-16 nfc into keeps surrogate pairs");
#if MJB_FEATURE_SECURITY
    mjb::confusable_skeleton(u"h\u0435llo", out16);
    ATT_ASSERT(out16 == mjb::confusable_skeleton(u"hello"), true, "UTF-16 confusable_skeleton into");
#endif

    std::u8string out8;
    mjb::nfc(u8"Cafe\u0301", out8);
    ATT_ASSERT(out8 == u8"Caf\u00E9", true, "char8_t nfc into a std::u8string");

    std::u16string failed16(u"unchanged?");

    try {
        mjb::nfc(std::u16string(1, static_cast<char16_t>(0xD800)), failed16);
    } catch(const mjb::LibraryError &error) {
        status = error.status();
    }

    ATT_ASSERT(failed16.empty(), true, "UTF-16 nfc into leaves out empty on failure");
}
