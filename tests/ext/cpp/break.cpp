/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "../../../src/cpp/mojibake.hpp"
#include "../../test.h"

#include <algorithm>
#include <iterator>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

// Collect the segments of a range as std::basic_string, which Attractor can compare.
template <typename Range> static std::vector<std::string> collect(const Range &range) {
    std::vector<std::string> segments;

    for(const auto segment : range) {
        segments.emplace_back(segment.data(), segment.size());
    }

    return segments;
}

static bool same(const std::vector<std::string> &actual, const std::vector<std::string> &expected) {
    return actual == expected;
}

// A range or a prefix over a temporary string would dangle, so it is rejected.
template <typename T>
concept can_segment = requires(T &&input) { mjb::graphemes(std::forward<T>(input)); };

template <typename T>
concept can_truncate = requires(T &&input) { mjb::grapheme_truncate(std::forward<T>(input), 1); };

static_assert(!can_segment<std::string>, "graphemes rejects a temporary std::string");
static_assert(!can_segment<std::u16string>, "graphemes rejects a temporary std::u16string");
static_assert(can_segment<std::string &>, "graphemes accepts a std::string lvalue");
static_assert(can_segment<std::string_view>, "graphemes accepts a std::string_view");
static_assert(can_segment<const char (&)[4]>, "graphemes accepts a literal");
static_assert(!can_truncate<std::u32string>, "grapheme_truncate rejects a temporary string");
static_assert(can_truncate<const std::string &>, "grapheme_truncate accepts a std::string lvalue");

// The ranges are C++20 views whose iterators outlive the range
static_assert(std::ranges::view<mjb::GraphemeRange<char>>, "GraphemeRange is a view");
static_assert(std::ranges::forward_range<mjb::WordRange<char16_t>>, "WordRange is forward");
static_assert(std::ranges::borrowed_range<mjb::SentenceRange<char32_t>>,
    "SentenceRange is borrowed");
static_assert(std::ranges::view<mjb::CodepointRange>, "CodepointRange is a view");
static_assert(std::ranges::forward_range<mjb::LineBreaks>, "LineBreaks is forward");

template <typename Range> static size_t count_of(const Range &range) {
    return static_cast<size_t>(std::ranges::distance(range));
}

ATT_TEST(cpp_break) {
    mjb::BreakResult r1{ 1, 0x41, MJB_BT_MANDATORY };
    ATT_ASSERT(r1.is_mandatory(), true, "BreakResult::is_mandatory");
    ATT_ASSERT(r1.is_allowed(), false, "BreakResult::is_mandatory => !is_allowed");
    ATT_ASSERT(r1.is_no_break(), false, "BreakResult::is_mandatory => !is_no_break");
    ATT_ASSERT(r1.is_break(), true, "BreakResult::is_mandatory => is_break");

    mjb::BreakResult r2{ 1, 0x41, MJB_BT_ALLOWED };
    ATT_ASSERT(r2.is_allowed(), true, "BreakResult::is_allowed");
    ATT_ASSERT(r2.is_mandatory(), false, "BreakResult::is_allowed => !is_mandatory");
    ATT_ASSERT(r2.is_no_break(), false, "BreakResult::is_allowed => !is_no_break");
    ATT_ASSERT(r2.is_break(), true, "BreakResult::is_allowed => is_break");

    mjb::BreakResult r3{ 1, 0x41, MJB_BT_NO_BREAK };
    ATT_ASSERT(r3.is_no_break(), true, "BreakResult::is_no_break");
    ATT_ASSERT(r3.is_mandatory(), false, "BreakResult::is_no_break => !is_mandatory");
    ATT_ASSERT(r3.is_allowed(), false, "BreakResult::is_no_break => !is_allowed");
    ATT_ASSERT(r3.is_break(), false, "BreakResult::is_no_break => !is_break");
    ATT_ASSERT(r1 == r1 && !(r1 == r2), true, "BreakResult equality");

    // Every codepoint after the first reports a position, the last one at the end of the text
    ATT_ASSERT(count_of(mjb::word_breaks("Hello world")), (size_t)11, "word_breaks count");
    ATT_ASSERT(count_of(mjb::sentence_breaks("Hello world. Goodbye.")), (size_t)21,
        "sentence_breaks count");
    ATT_ASSERT(count_of(mjb::line_breaks("foo bar")), (size_t)7, "line_breaks count");
    ATT_ASSERT(count_of(mjb::grapheme_breaks("abc")), (size_t)3, "grapheme_breaks count");
    ATT_ASSERT(mjb::word_breaks("").empty(), true, "word_breaks of empty text");

    // A range can be iterated again from the start
    const auto words_breaks = mjb::word_breaks("Hello world");
    ATT_ASSERT(count_of(words_breaks), count_of(words_breaks), "break ranges are multi-pass");

    const auto line_opportunities = mjb::line_breaks("foo bar");
    const auto allowed = std::ranges::count_if(line_opportunities,
        [](const mjb::BreakResult &result) { return result.is_break(); });
    ATT_ASSERT((size_t)allowed, (size_t)2, "line_breaks works with standard algorithms");

    // The codepoint is the one that starts at the position, also after a rule absorbs U+200B
    const std::string_view zwsp("a\xE2\x80\x8B"
                                "b");
    auto zwsp_sentences = mjb::sentence_breaks(zwsp).begin();
    ATT_ASSERT(zwsp_sentences->position, (size_t)1, "sentence_breaks: first position before U+200B");
    ATT_ASSERT(zwsp_sentences->codepoint, (mjb_codepoint)0x200B,
        "sentence_breaks: codepoint after SB5 absorption");
    ++zwsp_sentences;
    ATT_ASSERT(zwsp_sentences->position, (size_t)4, "sentence_breaks: second position before b");
    ATT_ASSERT(zwsp_sentences->codepoint, (mjb_codepoint)'b', "sentence_breaks: codepoint b");
    ++zwsp_sentences;
    ATT_ASSERT(zwsp_sentences->position, zwsp.size(), "sentence_breaks: end position");
    ATT_ASSERT(zwsp_sentences->codepoint, MJB_CODEPOINT_NOT_VALID,
        "sentence_breaks: no codepoint at the end");
    ++zwsp_sentences;
    ATT_ASSERT(zwsp_sentences == std::default_sentinel, true, "sentence_breaks: exhausted");

    const auto extend_first = *mjb::word_breaks("e\xCC\x81x").begin();
    ATT_ASSERT(extend_first.position, (size_t)1, "word_breaks: first position before U+0301");
    ATT_ASSERT(extend_first.codepoint, (mjb_codepoint)0x0301,
        "word_breaks: codepoint after WB4 absorption");

    // Positions are bytes in the input's encoding, here UTF-16
    const auto utf16_first = *mjb::grapheme_breaks(u"A\U0001F642").begin();
    ATT_ASSERT(utf16_first.position, (size_t)2, "grapheme_breaks: UTF-16 position in bytes");
    ATT_ASSERT(utf16_first.codepoint, (mjb_codepoint)0x1F642,
        "grapheme_breaks: UTF-16 surrogate pair codepoint");

    const auto malformed_first = *mjb::grapheme_breaks("a\x80"
                                                       "b")
                                      .begin();
    ATT_ASSERT(malformed_first.codepoint, (mjb_codepoint)0xFFFD,
        "grapheme_breaks: U+FFFD for a malformed byte");

    // A single character has one position, the end of the text, where no codepoint starts
    auto single = mjb::grapheme_breaks("a").begin();
    ATT_ASSERT(single->position, (size_t)1, "grapheme_breaks: end position after 'a'");
    ATT_ASSERT(single->codepoint, MJB_CODEPOINT_NOT_VALID, "grapheme_breaks: no codepoint at end");
    ATT_ASSERT(++single == std::default_sentinel, true,
        "grapheme_breaks: exhausted after a single character");

    // BreakResult reports the byte offset of each position
    auto two_byte = mjb::grapheme_breaks("a\xC3\xA9"
                                         "b")
                        .begin();
    ATT_ASSERT(two_byte->position, (size_t)1, "BreakResult position after the first codepoint");
    ++two_byte;
    ATT_ASSERT(two_byte->position, (size_t)3, "BreakResult position after a two-byte codepoint");

    mjb::BidiParagraph paragraph("abc");
    ATT_ASSERT(paragraph.empty(), false, "BidiParagraph::empty");
    ATT_ASSERT(paragraph.size(), 3u, "BidiParagraph::size");
    ATT_ASSERT((int)paragraph.direction(), MJB_DIRECTION_LTR, "BidiParagraph::direction");
    ATT_ASSERT(paragraph[0].codepoint, (mjb_codepoint)'a', "BidiParagraph::operator[]");
    ATT_ASSERT(paragraph.at(2).codepoint, (mjb_codepoint)'c', "BidiParagraph::at");
    ATT_ASSERT(paragraph.chars().size(), (size_t)3, "BidiParagraph::chars");

    size_t bidi_chars = 0;

    for(const mjb_bidi_char &bidi_char : paragraph) {
        bidi_chars += bidi_char.level == 0 ? 1 : 0;
    }

    ATT_ASSERT(bidi_chars, (size_t)3, "BidiParagraph iterates its characters");

    const auto order = paragraph.visual_order();
    ATT_ASSERT(order.size(), 3u, "BidiParagraph::visual_order size");
    ATT_ASSERT(order[0], 0u, "BidiParagraph::visual_order first");

    const auto runs = paragraph.line_runs(order);
    ATT_ASSERT(runs.size(), 1u, "BidiParagraph::line_runs size");
    ATT_ASSERT((int)runs[0].direction, MJB_DIRECTION_LTR, "BidiParagraph::line_runs direction");

    mjb::BidiParagraph moved(std::move(paragraph));
    ATT_ASSERT(paragraph.empty(), true, "BidiParagraph moved-from state");
    ATT_ASSERT(moved.size(), 3u, "BidiParagraph move constructor");

    const std::string hebrew("\xD7\x90\xD7\x91\xD7\x92");
    mjb::BidiParagraph rtl(hebrew);
    const auto rtl_order = rtl.visual_order();
    ATT_ASSERT((int)rtl.direction(), MJB_DIRECTION_RTL, "BidiParagraph RTL direction");
    ATT_ASSERT(rtl_order[0], 2u, "BidiParagraph RTL visual order");

    mjb::BidiParagraph utf16_paragraph(u"\u05D0b");
    ATT_ASSERT(utf16_paragraph.size(), (size_t)2, "BidiParagraph of UTF-16 text");

    // Segment ranges
    ATT_ASSERT((same(collect(mjb::graphemes("Ae\xCC\x81\xF0\x9F\x87\xAE\xF0\x9F\x87\xB9")),
                   { "A", "e\xCC\x81", "\xF0\x9F\x87\xAE\xF0\x9F\x87\xB9" })),
        true, "graphemes keeps combining marks and flags together");
    ATT_ASSERT((same(collect(mjb::words("Hello, world!")), { "Hello", ",", " ", "world", "!" })),
        true, "words includes the text between words");
    ATT_ASSERT((same(collect(mjb::sentences("Hi. Bye.")), { "Hi. ", "Bye." })), true, "sentences");
    ATT_ASSERT((same(collect(mjb::line_segments("Hello big world")), { "Hello ", "big ", "world" })),
        true, "line_segments keep their trailing spaces");
    ATT_ASSERT(collect(mjb::graphemes("")).empty(), true, "graphemes of empty text");
    ATT_ASSERT((same(collect(mjb::graphemes("ab\xE2\x82")), { "a", "b", "\xE2\x82" })), true,
        "graphemes keeps an incomplete tail as the last segment");

    const std::string owned("x\xCC\x81y");
    size_t owned_count = 0;

    for(const std::string_view cluster : mjb::graphemes(owned)) {
        owned_count += cluster.empty() ? 0 : 1;
    }

    ATT_ASSERT(owned_count, (size_t)2, "graphemes over a std::string lvalue");

    const auto clusters = mjb::graphemes(owned);
    ATT_ASSERT(count_of(clusters), mjb::grapheme_count(owned),
        "graphemes works with standard algorithms");

    // Views compose with the standard range adaptors
    std::vector<std::string> first_words;

    for(const std::string_view word : mjb::words("one two three") | std::views::take(3)) {
        first_words.emplace_back(word);
    }

    ATT_ASSERT((same(first_words, { "one", " ", "two" })), true, "words with std::views::take");

    // A borrowed range's iterator stays valid after the range is gone
    const std::string_view flag_text("x\xF0\x9F\x87\xAE\xF0\x9F\x87\xB9");
    const auto flag = std::ranges::find_if(mjb::graphemes(flag_text),
        [](std::string_view cluster) { return cluster.size() > 1; });
    ATT_ASSERT(std::string(*flag), std::string("\xF0\x9F\x87\xAE\xF0\x9F\x87\xB9"),
        "graphemes iterator outlives a temporary range");

    std::vector<std::u16string> utf16_clusters;

    for(const std::u16string_view cluster : mjb::graphemes(u"Ae\u0301\U0001F642")) {
        utf16_clusters.emplace_back(cluster);
    }

    ATT_ASSERT((utf16_clusters == std::vector<std::u16string>{ u"A", u"e\u0301", u"\U0001F642" }),
        true, "graphemes of UTF-16 gives UTF-16 views");

    const std::string_view utf16be("\0A\0B", 4);
    ATT_ASSERT((same(collect(mjb::graphemes(mjb::Text(utf16be, MJB_ENC_UTF_16BE))),
                   { std::string("\0A", 2), std::string("\0B", 2) })),
        true, "graphemes of an explicit encoding gives byte views");

    // Codepoint range
    std::u32string decoded;

    for(const char32_t codepoint : mjb::codepoints(u"A\U0001F642")) {
        decoded += codepoint;
    }

    ATT_ASSERT(decoded == U"A\U0001F642", true, "codepoints decodes UTF-16 surrogate pairs");

    const std::string malformed("a\x80"
                                "b");
    std::u32string replaced, stopped, skipped;

    for(const char32_t codepoint : mjb::codepoints(malformed)) {
        replaced += codepoint;
    }

    for(const char32_t codepoint : mjb::codepoints(malformed, MJB_MALFORMED_STOP)) {
        stopped += codepoint;
    }

    for(const char32_t codepoint : mjb::codepoints(malformed, MJB_MALFORMED_SKIP)) {
        skipped += codepoint;
    }

    ATT_ASSERT(replaced == U"a\uFFFDb", true, "codepoints replaces malformed input by default");
    ATT_ASSERT(stopped == U"a", true, "codepoints stops at malformed input");
    ATT_ASSERT(skipped == U"ab", true, "codepoints skips malformed input");
}
