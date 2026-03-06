/**
 * The Mojibake library
 *
 * UAX#31 identifier API tests.
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "test.h"

void *test_identifier(void *arg) {
    /* mjb_codepoint_is_id_start */
    ATT_ASSERT(mjb_codepoint_is_id_start(0x41), true, "ID_Start: U+0041 'A'")
    ATT_ASSERT(mjb_codepoint_is_id_start(0x5F), false, "ID_Start: U+005F '_' (continue only)")
    ATT_ASSERT(mjb_codepoint_is_id_start(0x00E9), true, "ID_Start: U+00E9 'é'")
    ATT_ASSERT(mjb_codepoint_is_id_start(0x30), false, "ID_Start: U+0030 '0' (not start)")
    ATT_ASSERT(mjb_codepoint_is_id_start(0x20), false, "ID_Start: U+0020 space (not start)")
    ATT_ASSERT(mjb_codepoint_is_id_start(0x21), false, "ID_Start: U+0021 '!' (not start)")

    /* mjb_codepoint_is_id_continue */
    ATT_ASSERT(mjb_codepoint_is_id_continue(0x41), true, "ID_Continue: U+0041 'A'")
    ATT_ASSERT(mjb_codepoint_is_id_continue(0x30), true, "ID_Continue: U+0030 '0'")
    ATT_ASSERT(mjb_codepoint_is_id_continue(0x5F), true, "ID_Continue: U+005F '_'")
    ATT_ASSERT(mjb_codepoint_is_id_continue(0x20), false, "ID_Continue: U+0020 space (not continue)")
    ATT_ASSERT(mjb_codepoint_is_id_continue(0x21), false, "ID_Continue: U+0021 '!' (not continue)")

    /* mjb_codepoint_is_xid_start / xid_continue */
    ATT_ASSERT(mjb_codepoint_is_xid_start(0x41), true, "XID_Start: U+0041 'A'")
    ATT_ASSERT(mjb_codepoint_is_xid_start(0x30), false, "XID_Start: U+0030 '0' (not start)")
    ATT_ASSERT(mjb_codepoint_is_xid_continue(0x30), true, "XID_Continue: U+0030 '0'")
    ATT_ASSERT(mjb_codepoint_is_xid_continue(0x20), false, "XID_Continue: U+0020 space")

    /* mjb_codepoint_is_pattern_syntax */
    ATT_ASSERT(mjb_codepoint_is_pattern_syntax(0x21), true, "Pattern_Syntax: U+0021 '!'")
    ATT_ASSERT(mjb_codepoint_is_pattern_syntax(0x2B), true, "Pattern_Syntax: U+002B '+'")
    ATT_ASSERT(mjb_codepoint_is_pattern_syntax(0x41), false, "Pattern_Syntax: U+0041 'A' (not syntax)")

    /* mjb_codepoint_is_pattern_white_space */
    ATT_ASSERT(mjb_codepoint_is_pattern_white_space(0x20), true, "Pattern_White_Space: U+0020 space")
    ATT_ASSERT(mjb_codepoint_is_pattern_white_space(0x09), true, "Pattern_White_Space: U+0009 tab")
    ATT_ASSERT(mjb_codepoint_is_pattern_white_space(0x41), false, "Pattern_White_Space: U+0041 'A'")

    /* mjb_string_is_identifier — DEFAULT profile */
    ATT_ASSERT(mjb_string_is_identifier("", 0, MJB_ENCODING_UTF_8, MJB_IDENTIFIER_DEFAULT), false, "Identifier: empty string")
    ATT_ASSERT(mjb_string_is_identifier("hello", 5, MJB_ENCODING_UTF_8, MJB_IDENTIFIER_DEFAULT), true, "Identifier: 'hello'")
    ATT_ASSERT(mjb_string_is_identifier("_x", 2, MJB_ENCODING_UTF_8, MJB_IDENTIFIER_DEFAULT), false, "Identifier: '_x' (_ not ID_Start)")
    ATT_ASSERT(mjb_string_is_identifier("hello123", 8, MJB_ENCODING_UTF_8, MJB_IDENTIFIER_DEFAULT), true, "Identifier: 'hello123'")
    ATT_ASSERT(mjb_string_is_identifier("hello_world", 11, MJB_ENCODING_UTF_8, MJB_IDENTIFIER_DEFAULT), true, "Identifier: 'hello_world'")
    ATT_ASSERT(mjb_string_is_identifier("123abc", 6, MJB_ENCODING_UTF_8, MJB_IDENTIFIER_DEFAULT), false, "Identifier: '123abc' (digit start)")
    ATT_ASSERT(mjb_string_is_identifier("hello world", 11, MJB_ENCODING_UTF_8, MJB_IDENTIFIER_DEFAULT), false, "Identifier: 'hello world' (space)")
    ATT_ASSERT(mjb_string_is_identifier("a+b", 3, MJB_ENCODING_UTF_8, MJB_IDENTIFIER_DEFAULT), false, "Identifier: 'a+b' (syntax char)")

    /* Multi-byte UTF-8: "café" = c a f U+00E9 (4 codepoints, 5 bytes) */
    ATT_ASSERT(mjb_string_is_identifier("caf\xC3\xA9", 5, MJB_ENCODING_UTF_8, MJB_IDENTIFIER_DEFAULT), true, "Identifier: 'café'")

    /* mjb_string_is_identifier — NFKC profile */
    ATT_ASSERT(mjb_string_is_identifier("hello", 5, MJB_ENCODING_UTF_8, MJB_IDENTIFIER_NFKC), true, "Identifier NFKC: 'hello'")
    ATT_ASSERT(mjb_string_is_identifier("123", 3, MJB_ENCODING_UTF_8, MJB_IDENTIFIER_NFKC), false, "Identifier NFKC: '123' (digit start)")

    return NULL;
}
