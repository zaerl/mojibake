/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "../../../src/cpp/mojibake.hpp"
#include "../../test.h"

#include <iterator>
#include <string>
#include <string_view>
#include <type_traits>
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

// A range over a temporary string would dangle in a range-based for loop, so it is rejected.
template <typename T, typename = void> struct can_segment : std::false_type {};

template <typename T>
struct can_segment<T, std::void_t<decltype(mjb::graphemes(std::declval<T>()))>> : std::true_type {};

static_assert(!can_segment<std::string>::value, "graphemes rejects a temporary std::string");
static_assert(!can_segment<std::u16string>::value, "graphemes rejects a temporary std::u16string");
static_assert(can_segment<std::string &>::value, "graphemes accepts a std::string lvalue");
static_assert(can_segment<std::string_view>::value, "graphemes accepts a std::string_view");
static_assert(can_segment<const char *>::value, "graphemes accepts a literal");

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

    mjb::WordBreaker wb("Hello world");
    ATT_ASSERT(wb.is_done(), false, "WordBreaker::is_done initial");

    int count = 0;

    while(wb.next()) {
        ++count;
    }

    ATT_ASSERT(wb.is_done(), true, "WordBreaker::is_done after exhaustion");
    ATT_ASSERT(wb.next().has_value(), false, "WordBreaker::next after done returns nullopt");

    wb.reset();
    ATT_ASSERT(wb.is_done(), false, "WordBreaker::is_done after reset");

    int count2 = 0;
    wb.for_each([&](const mjb::BreakResult &) { ++count2; });
    ATT_ASSERT(count, count2, "WordBreaker: for_each gives same count as manual next()");

    // Test SentenceBreaker
    mjb::SentenceBreaker sb("Hello world. Goodbye.");
    count = 0;
    while(sb.next()) {
        ++count;
    }
    ATT_ASSERT(sb.is_done(), true, "SentenceBreaker::is_done after exhaustion");

    sb.reset();
    count2 = 0;
    sb.for_each([&](const mjb::BreakResult &) { ++count2; });
    ATT_ASSERT(count, count2, "SentenceBreaker: for_each gives same count as manual next()");

    // Test LineBreaker
    mjb::LineBreaker lb("foo bar");
    count = 0;

    while(lb.next()) {
        ++count;
    }

    ATT_ASSERT(lb.is_done(), true, "LineBreaker::is_done after exhaustion");

    lb.reset();
    count2 = 0;
    lb.for_each([&](const mjb::BreakResult &) { ++count2; });
    ATT_ASSERT(count, count2, "LineBreaker: for_each gives same count as manual next()");

    // Test GraphemeBreaker
    mjb::GraphemeBreaker gb("abc");
    count = 0;
    while(gb.next()) {
        ++count;
    }
    ATT_ASSERT(gb.is_done(), true, "GraphemeBreaker::is_done after exhaustion");

    gb.reset();
    count2 = 0;
    gb.for_each([&](const mjb::BreakResult &) { ++count2; });

    ATT_ASSERT(count, count2, "GraphemeBreaker: for_each gives same count as manual next()");

    // The codepoint is the one that starts at the position, also after a rule absorbs U+200B
    const std::string_view zwsp("a\xE2\x80\x8B"
                                "b");
    mjb::SentenceBreaker zwsp_sentences(zwsp);
    auto zwsp_first = zwsp_sentences.next();
    ATT_ASSERT(zwsp_first.has_value() && zwsp_first->position == 1, true,
        "SentenceBreaker: first position before U+200B");
    ATT_ASSERT(zwsp_first->codepoint, (mjb_codepoint)0x200B,
        "SentenceBreaker: codepoint after SB5 absorption");
    auto zwsp_second = zwsp_sentences.next();
    ATT_ASSERT(zwsp_second.has_value() && zwsp_second->position == 4, true,
        "SentenceBreaker: second position before b");
    ATT_ASSERT(zwsp_second->codepoint, (mjb_codepoint)'b', "SentenceBreaker: codepoint b");
    auto zwsp_end = zwsp_sentences.next();
    ATT_ASSERT(zwsp_end.has_value() && zwsp_end->position == zwsp.size(), true,
        "SentenceBreaker: end position");
    ATT_ASSERT(zwsp_end->codepoint, MJB_CODEPOINT_NOT_VALID,
        "SentenceBreaker: no codepoint at the end");

    mjb::WordBreaker extend_words("e\xCC\x81x");
    auto extend_first = extend_words.next();
    ATT_ASSERT(extend_first.has_value() && extend_first->position == 1, true,
        "WordBreaker: first position before U+0301");
    ATT_ASSERT(extend_first->codepoint, (mjb_codepoint)0x0301,
        "WordBreaker: codepoint after WB4 absorption");

    // Positions are bytes in the input's encoding, here UTF-16
    mjb::GraphemeBreaker utf16_graphemes(u"A\U0001F642");
    auto utf16_first = utf16_graphemes.next();
    ATT_ASSERT(utf16_first.has_value() && utf16_first->position == 2, true,
        "GraphemeBreaker: UTF-16 position in bytes");
    ATT_ASSERT(utf16_first->codepoint, (mjb_codepoint)0x1F642,
        "GraphemeBreaker: UTF-16 surrogate pair codepoint");

    mjb::GraphemeBreaker malformed_graphemes("a\x80"
                                             "b");
    auto malformed_first = malformed_graphemes.next();
    ATT_ASSERT(malformed_first.has_value() && malformed_first->codepoint == 0xFFFD, true,
        "GraphemeBreaker: U+FFFD for a malformed byte");

    // Test for_each continues from current position (does not reset)
    mjb::WordBreaker wb2("Hello");
    (void)wb2.next(); // advance one step

    int after_one = 0;
    wb2.for_each([&](const mjb::BreakResult &) { ++after_one; });

    wb2.reset();
    int total = 0;
    wb2.for_each([&](const mjb::BreakResult &) { ++total; });

    ATT_ASSERT(after_one + 1, total, "WordBreaker::for_each continues from current position");

    // A single character has one position, the end of the text, where no codepoint starts
    mjb::GraphemeBreaker gb2("a");
    auto result = gb2.next();
    ATT_ASSERT(result.has_value(), true, "GraphemeBreaker::next returns value for single char");
    ATT_ASSERT(result->position, (size_t)1, "GraphemeBreaker: end position after 'a'");
    ATT_ASSERT(result->codepoint, MJB_CODEPOINT_NOT_VALID, "GraphemeBreaker: no codepoint at end");
    ATT_ASSERT(gb2.next().has_value(), false, "GraphemeBreaker: exhausted after single char");

    // Test empty string
    mjb::WordBreaker wb3("");
    ATT_ASSERT(wb3.next().has_value(), false, "WordBreaker::next on empty string returns nullopt");
    ATT_ASSERT(wb3.is_done(), true, "WordBreaker::is_done on empty string");

    ATT_ASSERT(std::string(wb.input()), std::string("Hello world"), "Breaker::input");
    ATT_ASSERT((int)wb.input_encoding(), MJB_ENC_UTF_8, "Breaker::input_encoding");

    mjb::BidiParagraph paragraph("abc");
    ATT_ASSERT(paragraph.empty(), false, "BidiParagraph::empty");
    ATT_ASSERT(paragraph.size(), 3u, "BidiParagraph::size");
    ATT_ASSERT((int)paragraph.direction(), MJB_DIRECTION_LTR, "BidiParagraph::direction");
    ATT_ASSERT(paragraph[0].codepoint, (mjb_codepoint)'a', "BidiParagraph::operator[]");
    ATT_ASSERT(paragraph.at(2).codepoint, (mjb_codepoint)'c', "BidiParagraph::at");

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

    // BreakResult reports the byte offset of each position
    mjb::GraphemeBreaker grapheme_breaker("a\xC3\xA9"
                                          "b");
    const auto first_position = grapheme_breaker.next();
    const auto second_position = grapheme_breaker.next();
    ATT_ASSERT(first_position.has_value() && first_position->position == 1, true,
        "BreakResult position after the first codepoint");
    ATT_ASSERT(second_position.has_value() && second_position->position == 3, true,
        "BreakResult position after a two-byte codepoint");

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
    ATT_ASSERT((size_t)std::distance(clusters.begin(), clusters.end()), mjb::grapheme_count(owned),
        "graphemes works with standard algorithms");

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
