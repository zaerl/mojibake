/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "../../../src/cpp/mojibake.hpp"
#include "../../test.h"

#include <string>
#include <string_view>
#include <type_traits>

// mjb::Text converts implicitly from UTF-8, UTF-16, and UTF-32 strings, and from nothing else.
static_assert(std::is_convertible_v<const char *, mjb::Text>, "Text from a literal");
static_assert(std::is_convertible_v<std::string, mjb::Text>, "Text from std::string");
static_assert(std::is_convertible_v<std::string_view, mjb::Text>, "Text from std::string_view");
static_assert(std::is_convertible_v<std::u16string, mjb::Text>, "Text from std::u16string");
static_assert(std::is_convertible_v<const char32_t *, mjb::Text>, "Text from a UTF-32 literal");
static_assert(!std::is_convertible_v<int, mjb::Text>, "No Text from an integer");
static_assert(!std::is_convertible_v<std::wstring, mjb::Text>, "No Text from std::wstring");

// UTF-16 and UTF-32 strings in the platform's byte order. Attractor does not print char16_t or
// char32_t strings, so results are compared with == and asserted as booleans.
ATT_TEST(cpp_strings) {
    ATT_ASSERT(mjb::native_utf16_encoding == MJB_ENC_UTF_16LE ||
            mjb::native_utf16_encoding == MJB_ENC_UTF_16BE,
        true, "native UTF-16 encoding has a byte order");
    ATT_ASSERT(mjb::native_utf32_encoding == MJB_ENC_UTF_32LE ||
            mjb::native_utf32_encoding == MJB_ENC_UTF_32BE,
        true, "native UTF-32 encoding has a byte order");

    // mjb::Text records the bytes and the encoding of its input
    const mjb::Text utf16_text(u"abc");
    ATT_ASSERT(utf16_text.encoding() == mjb::native_utf16_encoding, true,
        "Text from UTF-16 uses the native byte order");
    ATT_ASSERT(utf16_text.bytes().size(), (size_t)6, "Text from UTF-16 counts bytes");
    ATT_ASSERT(mjb::Text("abc").encoding() == MJB_ENC_UTF_8, true, "Text from char is UTF-8");

    // An explicit encoding covers bytes and code units in the other byte order
    const std::string_view utf16be_bytes("\0A\0B", 4);
    ATT_ASSERT(mjb::codepoint_count(mjb::Text(utf16be_bytes, MJB_ENC_UTF_16BE)), (size_t)2,
        "Text with an explicit encoding");
    const mjb_encoding swapped_utf16 = mjb::native_utf16_encoding == MJB_ENC_UTF_16LE ?
        MJB_ENC_UTF_16BE :
        MJB_ENC_UTF_16LE;
    const char16_t swapped[] = { static_cast<char16_t>(0x4100), static_cast<char16_t>(0x4200) };
    ATT_ASSERT(mjb::caseless_match(mjb::Text(std::u16string_view(swapped, 2), swapped_utf16), "ab"),
        true, "Text with the non-native UTF-16 byte order");
    ATT_ASSERT(mjb::grapheme_count(std::string("A\xCC\x81")), (size_t)1,
        "Text from a temporary std::string");

    // Transforms return the input's string type
    ATT_ASSERT(mjb::nfc(u"e\u0301") == u"\u00E9", true, "UTF-16 nfc");
    ATT_ASSERT(mjb::nfd(U"\u00E9") == U"e\u0301", true, "UTF-32 nfd");
    ATT_ASSERT(mjb::nfkc(u"\uFB01") == u"fi", true, "UTF-16 nfkc");
    ATT_ASSERT(mjb::nfkd(U"\u00C5") == U"A\u030A", true, "UTF-32 nfkd");
    ATT_ASSERT(mjb::uppercase(u"stra\u00DFe") == u"STRASSE", true, "UTF-16 uppercase");
    ATT_ASSERT(mjb::uppercase(U"istanbul", MJB_LOCALE_TR) == U"\u0130STANBUL", true,
        "UTF-32 uppercase with a locale");
    ATT_ASSERT(mjb::lowercase(u"HELLO") == u"hello", true, "UTF-16 lowercase");
    ATT_ASSERT(mjb::titlecase(U"hello world") == U"Hello World", true, "UTF-32 titlecase");
    ATT_ASSERT(mjb::casefold(u"Stra\u00DFe") == u"strasse", true, "UTF-16 casefold");
    ATT_ASSERT(mjb::casefold_simple(U"\u1E9E") == U"\u00DF", true, "UTF-32 casefold_simple");
    ATT_ASSERT(mjb::nfkc_casefold(u"Stra\u00DFe\u00AD") == u"strasse", true,
        "UTF-16 nfkc_casefold");
    ATT_ASSERT(mjb::filter(u"e\u0301", mjb::Filter::Normalize) == u"\u00E9", true,
        "UTF-16 filter");
#if MJB_FEATURE_SECURITY
    ATT_ASSERT(mjb::confusable_skeleton(u"h\u0435llo") == mjb::confusable_skeleton(u"hello"), true,
        "UTF-16 confusable_skeleton");
#endif

    const std::u16string owned_input(u"e\u0301");
    ATT_ASSERT(mjb::nfc(owned_input) == u"\u00E9", true, "UTF-16 nfc from a std::u16string");

    // Supplementary codepoints are surrogate pairs in UTF-16 and single units in UTF-32
    ATT_ASSERT(mjb::codepoint_count(u"\U0001F642"), (size_t)1, "UTF-16 codepoint_count");
    ATT_ASSERT(mjb::codepoint_count(U"\U0001F642"), (size_t)1, "UTF-32 codepoint_count");
    ATT_ASSERT(mjb::grapheme_count(U"A\U0001F1EE\U0001F1F9"), (size_t)2, "UTF-32 grapheme_count");
    ATT_ASSERT(mjb::word_count(u"Hello, world!"), (size_t)2, "UTF-16 word_count");
    ATT_ASSERT(mjb::sentence_count(U"Hi. Bye."), (size_t)2, "UTF-32 sentence_count");
    ATT_ASSERT(mjb::terminal_width(u"A\u754C"), (size_t)3, "UTF-16 terminal_width");

    const std::u16string lone_surrogate(1, static_cast<char16_t>(0xD800));
    ATT_ASSERT_STATUS(mjb::validate_encoding(u"abc"), MJB_STATUS_OK, "UTF-16 validate_encoding");
    ATT_ASSERT_STATUS(mjb::validate_encoding(lone_surrogate), MJB_STATUS_MALFORMED_INPUT,
        "UTF-16 validate_encoding rejects a lone surrogate");

    std::u32string visited;
    ATT_ASSERT_STATUS(mjb::for_each_codepoint(u"A\U0001F642",
                          [&visited](const mjb_character &each, mjb_character_position) {
                              visited += static_cast<char32_t>(each.codepoint);

                              return true;
                          }),
        MJB_STATUS_OK, "UTF-16 for_each_codepoint");
    ATT_ASSERT(visited == U"A\U0001F642", true, "UTF-16 for_each_codepoint decodes surrogates");

    // Predicates and lookups
    ATT_ASSERT(mjb::is_identifier(u"delta_2"), true, "UTF-16 is_identifier");
    ATT_ASSERT(mjb::is_identifier(U"2delta"), false, "UTF-32 is_identifier");
    ATT_ASSERT(mjb::is_rgi_emoji(U"\U0001F1EE\U0001F1F9"), true, "UTF-32 is_rgi_emoji");
    ATT_ASSERT(mjb::is_emoji_sequence(u"1\uFE0F\u20E3"), true, "UTF-16 is_emoji_sequence");

    const auto sequence = mjb::emoji_sequence(u"\U0001F642");
    ATT_ASSERT(sequence.has_value() && sequence->size() == 1, true, "UTF-16 emoji_sequence");
    ATT_ASSERT((int)mjb::normalization_quick_check(u"caf\u00E9", mjb::NormalizationForm::NFC),
        (int)MJB_QC_YES, "UTF-16 normalization_quick_check");

#if MJB_FEATURE_SECURITY
    const auto scripts = mjb::resolved_script_set(U"\u306D\u30AC");
    ATT_ASSERT(scripts.kind == MJB_SCRIPT_SET_RESOLVED && scripts.scripts.size() == 1 &&
            scripts.scripts[0] == MJB_SC_JPAN,
        true, "UTF-32 resolved_script_set");
#endif
    ATT_ASSERT(std::string(mjb::locale_parse(u"sr-Latn-RS").region()), std::string("RS"),
        "UTF-16 locale_parse");

    // Comparisons, including two different string types
    ATT_ASSERT(mjb::caseless_match(u"Stra\u00DFe", U"STRASSE"), true,
        "caseless_match of UTF-16 and UTF-32");
#if MJB_FEATURE_SECURITY
    ATT_ASSERT(mjb::confusable_match(u"h\u0435llo", u"hello"), true, "UTF-16 confusable_match");
#endif
#if MJB_FEATURE_COLLATION
    ATT_ASSERT(mjb::collation_compare(U"apple", U"banana") < 0, true, "UTF-32 collation_compare");
    ATT_ASSERT(mjb::collation_key(u"apple") == mjb::collation_key("apple"), true,
        "UTF-16 collation_key matches the UTF-8 key");
#endif

    // Prefixes are counted in code units
    const std::u16string_view flags(u"A\U0001F1EE\U0001F1F9Z");
    ATT_ASSERT(mjb::grapheme_truncate(flags, 2) == u"A\U0001F1EE\U0001F1F9", true,
        "UTF-16 grapheme_truncate keeps whole surrogate pairs");
    ATT_ASSERT(mjb::grapheme_truncate(flags, 2).size(), (size_t)5,
        "UTF-16 grapheme_truncate size is in code units");
    ATT_ASSERT(mjb::grapheme_truncate_width(U"A\u754C", 2) == U"A", true,
        "UTF-32 grapheme_truncate_width");
    ATT_ASSERT(mjb::word_truncate(u"Hello world", 1) == u"Hello", true, "UTF-16 word_truncate");
    ATT_ASSERT(mjb::word_truncate_width(U"Hello world", 6) == U"Hello ", true,
        "UTF-32 word_truncate_width");
}
