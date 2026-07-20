/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "../../../src/cpp/mojibake.hpp"
#include "../../test.h"

#include <utility>

int test_cpp_break(void *arg) {
    mjb::BreakResult r1{ 1, 0x41, MJB_BT_MANDATORY };
    ATT_ASSERT(r1.is_mandatory(), true, "BreakResult::is_mandatory")
    ATT_ASSERT(r1.is_allowed(), false, "BreakResult::is_mandatory => !is_allowed")
    ATT_ASSERT(r1.is_no_break(), false, "BreakResult::is_mandatory => !is_no_break")
    ATT_ASSERT(r1.is_break(), true, "BreakResult::is_mandatory => is_break")

    mjb::BreakResult r2{ 1, 0x41, MJB_BT_ALLOWED };
    ATT_ASSERT(r2.is_allowed(), true, "BreakResult::is_allowed")
    ATT_ASSERT(r2.is_mandatory(), false, "BreakResult::is_allowed => !is_mandatory")
    ATT_ASSERT(r2.is_no_break(), false, "BreakResult::is_allowed => !is_no_break")
    ATT_ASSERT(r2.is_break(), true, "BreakResult::is_allowed => is_break")

    mjb::BreakResult r3{ 1, 0x41, MJB_BT_NO_BREAK };
    ATT_ASSERT(r3.is_no_break(), true, "BreakResult::is_no_break")
    ATT_ASSERT(r3.is_mandatory(), false, "BreakResult::is_no_break => !is_mandatory")
    ATT_ASSERT(r3.is_allowed(), false, "BreakResult::is_no_break => !is_allowed")
    ATT_ASSERT(r3.is_break(), false, "BreakResult::is_no_break => !is_break")

    mjb::WordBreaker wb("Hello world");
    ATT_ASSERT(wb.is_done(), false, "WordBreaker::is_done initial")

    int count = 0;

    while(wb.next()) {
        ++count;
    }

    ATT_ASSERT(wb.is_done(), true, "WordBreaker::is_done after exhaustion")
    ATT_ASSERT(wb.next().has_value(), false, "WordBreaker::next after done returns nullopt")

    wb.reset();
    ATT_ASSERT(wb.is_done(), false, "WordBreaker::is_done after reset")

    int count2 = 0;
    wb.for_each([&](const mjb::BreakResult &) { ++count2; });
    ATT_ASSERT(count, count2, "WordBreaker: for_each gives same count as manual next()")

    // Test SentenceBreaker
    mjb::SentenceBreaker sb("Hello world. Goodbye.");
    count = 0;
    while(sb.next()) {
        ++count;
    }
    ATT_ASSERT(sb.is_done(), true, "SentenceBreaker::is_done after exhaustion")

    sb.reset();
    count2 = 0;
    sb.for_each([&](const mjb::BreakResult &) { ++count2; });
    ATT_ASSERT(count, count2, "SentenceBreaker: for_each gives same count as manual next()")

    // Test LineBreaker
    mjb::LineBreaker lb("foo bar");
    count = 0;

    while(lb.next()) {
        ++count;
    }

    ATT_ASSERT(lb.is_done(), true, "LineBreaker::is_done after exhaustion")

    lb.reset();
    count2 = 0;
    lb.for_each([&](const mjb::BreakResult &) { ++count2; });
    ATT_ASSERT(count, count2, "LineBreaker: for_each gives same count as manual next()")

    // Test GraphemeBreaker
    mjb::GraphemeBreaker gb("abc");
    count = 0;
    while(gb.next()) {
        ++count;
    }
    ATT_ASSERT(gb.is_done(), true, "GraphemeBreaker::is_done after exhaustion")

    gb.reset();
    count2 = 0;
    gb.for_each([&](const mjb::BreakResult &) { ++count2; });

    ATT_ASSERT(count, count2, "GraphemeBreaker: for_each gives same count as manual next()")

    // Test for_each continues from current position (does not reset)
    mjb::WordBreaker wb2("Hello");
    (void)wb2.next(); // advance one step

    int after_one = 0;
    wb2.for_each([&](const mjb::BreakResult &) { ++after_one; });

    wb2.reset();
    int total = 0;
    wb2.for_each([&](const mjb::BreakResult &) { ++total; });

    ATT_ASSERT(after_one + 1, total, "WordBreaker::for_each continues from current position")

    // Test codepoint and index fields for a single ASCII character
    mjb::GraphemeBreaker gb2("a");
    auto result = gb2.next();
    ATT_ASSERT(result.has_value(), true, "GraphemeBreaker::next returns value for single char")
    ATT_ASSERT(result->codepoint, (mjb_codepoint)0x61, "GraphemeBreaker: codepoint 'a'")
    ATT_ASSERT(gb2.next().has_value(), false, "GraphemeBreaker: exhausted after single char")

    // Test empty string
    mjb::WordBreaker wb3("");
    ATT_ASSERT(wb3.next().has_value(), false, "WordBreaker::next on empty string returns nullopt")
    ATT_ASSERT(wb3.is_done(), true, "WordBreaker::is_done on empty string")

    ATT_ASSERT(std::string(wb.input()), std::string("Hello world"), "Breaker::input")
    ATT_ASSERT((int)wb.input_encoding(), MJB_ENC_UTF_8, "Breaker::input_encoding")

    mjb::BidiParagraph paragraph("abc");
    ATT_ASSERT(paragraph.empty(), false, "BidiParagraph::empty")
    ATT_ASSERT(paragraph.size(), 3u, "BidiParagraph::size")
    ATT_ASSERT((int)paragraph.direction(), MJB_DIRECTION_LTR, "BidiParagraph::direction")
    ATT_ASSERT(paragraph[0].codepoint, (mjb_codepoint)'a', "BidiParagraph::operator[]")
    ATT_ASSERT(paragraph.at(2).codepoint, (mjb_codepoint)'c', "BidiParagraph::at")

    const auto order = paragraph.visual_order();
    ATT_ASSERT(order.size(), 3u, "BidiParagraph::visual_order size")
    ATT_ASSERT(order[0], 0u, "BidiParagraph::visual_order first")

    const auto runs = paragraph.line_runs(order);
    ATT_ASSERT(runs.size(), 1u, "BidiParagraph::line_runs size")
    ATT_ASSERT((int)runs[0].direction, MJB_DIRECTION_LTR, "BidiParagraph::line_runs direction")

    mjb::BidiParagraph moved(std::move(paragraph));
    ATT_ASSERT(paragraph.empty(), true, "BidiParagraph moved-from state")
    ATT_ASSERT(moved.size(), 3u, "BidiParagraph move constructor")

    const std::string hebrew("\xD7\x90\xD7\x91\xD7\x92");
    mjb::BidiParagraph rtl(hebrew);
    const auto rtl_order = rtl.visual_order();
    ATT_ASSERT((int)rtl.direction(), MJB_DIRECTION_RTL, "BidiParagraph RTL direction")
    ATT_ASSERT(rtl_order[0], 2u, "BidiParagraph RTL visual order")

    return 0;
}
