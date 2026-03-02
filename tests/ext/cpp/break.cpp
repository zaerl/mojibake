/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "../../test.h"
#include "../../../ext/cpp/mojibake.hpp"

void *test_cpp_break(void *arg) {
    mjb::BreakResult r1{1, 0x41, MJB_BT_MANDATORY};
    ATT_ASSERT(r1.is_mandatory(), true, "BreakResult::is_mandatory");
    ATT_ASSERT(r1.is_allowed(), false, "BreakResult::is_mandatory => !is_allowed");
    ATT_ASSERT(r1.is_no_break(), false, "BreakResult::is_mandatory => !is_no_break");
    ATT_ASSERT(r1.is_break(), true, "BreakResult::is_mandatory => is_break");

    mjb::BreakResult r2{1, 0x41, MJB_BT_ALLOWED};
    ATT_ASSERT(r2.is_allowed(), true, "BreakResult::is_allowed");
    ATT_ASSERT(r2.is_mandatory(), false, "BreakResult::is_allowed => !is_mandatory");
    ATT_ASSERT(r2.is_no_break(), false, "BreakResult::is_allowed => !is_no_break");
    ATT_ASSERT(r2.is_break(), true, "BreakResult::is_allowed => is_break");

    mjb::BreakResult r3{1, 0x41, MJB_BT_NO_BREAK};
    ATT_ASSERT(r3.is_no_break(), true, "BreakResult::is_no_break");
    ATT_ASSERT(r3.is_mandatory(), false, "BreakResult::is_no_break => !is_mandatory");
    ATT_ASSERT(r3.is_allowed(), false, "BreakResult::is_no_break => !is_allowed");
    ATT_ASSERT(r3.is_break(), false, "BreakResult::is_no_break => !is_break");

    mjb::WordBreaker wb("Hello world");
    ATT_ASSERT(wb.is_done(), false, "WordBreaker::is_done initial");

    int count = 0;
    while(wb.next()) { ++count; }

    ATT_ASSERT(wb.is_done(), true, "WordBreaker::is_done after exhaustion");
    ATT_ASSERT(wb.next().has_value(), false, "WordBreaker::next after done returns nullopt");

    wb.reset();
    ATT_ASSERT(wb.is_done(), false, "WordBreaker::is_done after reset");

    int count2 = 0;
    wb.for_each([&](const mjb::BreakResult&) { ++count2; });
    ATT_ASSERT(count, count2, "WordBreaker: for_each gives same count as manual next()");

    // Test SentenceBreaker
    mjb::SentenceBreaker sb("Hello world. Goodbye.");
    count = 0;
    while(sb.next()) { ++count; }
    ATT_ASSERT(sb.is_done(), true, "SentenceBreaker::is_done after exhaustion");

    sb.reset();
    count2 = 0;
    sb.for_each([&](const mjb::BreakResult&) { ++count2; });
    ATT_ASSERT(count, count2, "SentenceBreaker: for_each gives same count as manual next()");

    // Test LineBreaker
    mjb::LineBreaker lb("foo bar");
    count = 0;
    while(lb.next()) { ++count; }
    ATT_ASSERT(lb.is_done(), true, "LineBreaker::is_done after exhaustion");

    lb.reset();
    count2 = 0;
    lb.for_each([&](const mjb::BreakResult&) { ++count2; });
    ATT_ASSERT(count, count2, "LineBreaker: for_each gives same count as manual next()");

    // Test GraphemeBreaker
    mjb::GraphemeBreaker gb("abc");
    count = 0;
    while(gb.next()) { ++count; }
    ATT_ASSERT(gb.is_done(), true, "GraphemeBreaker::is_done after exhaustion");

    gb.reset();
    count2 = 0;
    gb.for_each([&](const mjb::BreakResult&) { ++count2; });
    ATT_ASSERT(count, count2, "GraphemeBreaker: for_each gives same count as manual next()");

    // Test for_each continues from current position (does not reset)
    mjb::WordBreaker wb2("Hello");
    (void)wb2.next(); // advance one step

    int after_one = 0;
    wb2.for_each([&](const mjb::BreakResult&) { ++after_one; });

    wb2.reset();
    int total = 0;
    wb2.for_each([&](const mjb::BreakResult&) { ++total; });

    ATT_ASSERT(after_one + 1, total, "WordBreaker::for_each continues from current position");

    // Test codepoint and index fields for a single ASCII character
    mjb::GraphemeBreaker gb2("a");
    auto result = gb2.next();
    ATT_ASSERT(result.has_value(), true, "GraphemeBreaker::next returns value for single char");
    ATT_ASSERT(result->codepoint, (mjb_codepoint)0x61, "GraphemeBreaker: codepoint 'a'");
    ATT_ASSERT(gb2.next().has_value(), false, "GraphemeBreaker: exhausted after single char");

    // Test empty string
    mjb::WordBreaker wb3("");
    ATT_ASSERT(wb3.next().has_value(), false, "WordBreaker::next on empty string returns nullopt");
    ATT_ASSERT(wb3.is_done(), true, "WordBreaker::is_done on empty string");

    return NULL;
}
