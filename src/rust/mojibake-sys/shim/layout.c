// The Mojibake library
//
// This file is distributed under the MIT License. See LICENSE for details.

// Layout probes for the hand-written Rust declarations in src/lib.rs. The Rust tests compare these
// values with size_of/align_of/offset_of so that a header change cannot silently break the FFI.

#include <stddef.h>

#include "mojibake.h"

// The Rust break states are opaque storage: two leading size_t fields followed by a fixed tail.
#define MJB_RS_BREAK_STATE_TAIL 128

#define MJB_RS_CHECK_BREAK_STATE(TYPE) \
    _Static_assert(sizeof(TYPE) <= 2 * sizeof(size_t) + MJB_RS_BREAK_STATE_TAIL, \
        #TYPE " does not fit the Rust storage"); \
    _Static_assert(_Alignof(TYPE) <= 8, #TYPE " needs a larger alignment"); \
    _Static_assert(offsetof(TYPE, index) == 0, #TYPE " index offset"); \
    _Static_assert(offsetof(TYPE, position) == sizeof(size_t), #TYPE " position offset")

MJB_RS_CHECK_BREAK_STATE(mjb_grapheme_break_state);
MJB_RS_CHECK_BREAK_STATE(mjb_word_break_state);
MJB_RS_CHECK_BREAK_STATE(mjb_sentence_break_state);
MJB_RS_CHECK_BREAK_STATE(mjb_line_break_state);

#define MJB_RS_LAYOUT(NAME, TYPE) \
    size_t mjb_rs_sizeof_##NAME(void); \
    size_t mjb_rs_alignof_##NAME(void); \
    size_t mjb_rs_sizeof_##NAME(void) { \
        return sizeof(TYPE); \
    } \
    size_t mjb_rs_alignof_##NAME(void) { \
        return _Alignof(TYPE); \
    }

#define MJB_RS_OFFSET(NAME, TYPE, MEMBER) \
    size_t mjb_rs_offsetof_##NAME(void); \
    size_t mjb_rs_offsetof_##NAME(void) { \
        return offsetof(TYPE, MEMBER); \
    }

MJB_RS_LAYOUT(text, mjb_text)
MJB_RS_LAYOUT(buffer, mjb_buffer)
MJB_RS_LAYOUT(result, mjb_result)
MJB_RS_LAYOUT(diagnostic, mjb_diagnostic)
MJB_RS_LAYOUT(character, mjb_character)
MJB_RS_LAYOUT(numeric_value, mjb_numeric_value)
MJB_RS_LAYOUT(block_info, mjb_block_info)
MJB_RS_LAYOUT(emoji_properties, mjb_emoji_properties)
MJB_RS_LAYOUT(emoji_sequence, mjb_emoji_sequence)
MJB_RS_LAYOUT(locale_id, mjb_locale_id)
MJB_RS_LAYOUT(idna_info, mjb_idna_info)
MJB_RS_LAYOUT(allocator, mjb_allocator)
MJB_RS_LAYOUT(bidi_char, mjb_bidi_char)
MJB_RS_LAYOUT(bidi_paragraph, mjb_bidi_paragraph)
MJB_RS_LAYOUT(bidi_run, mjb_bidi_run)
MJB_RS_LAYOUT(encoding, mjb_encoding)
MJB_RS_LAYOUT(status, mjb_status)

MJB_RS_OFFSET(character_numeric, mjb_character, numeric)
MJB_RS_OFFSET(character_titlecase, mjb_character, titlecase)
MJB_RS_OFFSET(locale_id_locale, mjb_locale_id, locale)
MJB_RS_OFFSET(bidi_char_byte_offset, mjb_bidi_char, byte_offset)
MJB_RS_OFFSET(bidi_char_level, mjb_bidi_char, level)
MJB_RS_OFFSET(bidi_char_resolved_class, mjb_bidi_char, resolved_class)
MJB_RS_OFFSET(bidi_char_mirroring_glyph, mjb_bidi_char, mirroring_glyph)
MJB_RS_OFFSET(bidi_paragraph_count, mjb_bidi_paragraph, count)
MJB_RS_OFFSET(bidi_paragraph_paragraph_level, mjb_bidi_paragraph, paragraph_level)
MJB_RS_OFFSET(bidi_paragraph_direction, mjb_bidi_paragraph, direction)
MJB_RS_OFFSET(bidi_run_end, mjb_bidi_run, end)
MJB_RS_OFFSET(bidi_run_level, mjb_bidi_run, level)
MJB_RS_OFFSET(bidi_run_direction, mjb_bidi_run, direction)

// The value of a C enumerator, so the tests can compare it with the hand-written Rust constant.
#define MJB_RS_VALUE(NAME) \
    int mjb_rs_value_##NAME(void); \
    int mjb_rs_value_##NAME(void) { \
        return (int)(NAME); \
    }

MJB_RS_VALUE(MJB_FILTER_NORMALIZE)
MJB_RS_VALUE(MJB_FILTER_SPACES)
MJB_RS_VALUE(MJB_FILTER_COLLAPSE_SPACES)
MJB_RS_VALUE(MJB_FILTER_CONTROLS)
MJB_RS_VALUE(MJB_FILTER_NUMERIC)
MJB_RS_VALUE(MJB_FILTER_LIMIT_COMBINING)
MJB_RS_VALUE(MJB_EMOJI_SEQUENCE_BASIC)
MJB_RS_VALUE(MJB_EMOJI_SEQUENCE_KEYCAP)
MJB_RS_VALUE(MJB_EMOJI_SEQUENCE_FLAG)
MJB_RS_VALUE(MJB_EMOJI_SEQUENCE_TAG)
MJB_RS_VALUE(MJB_EMOJI_SEQUENCE_MODIFIER)
MJB_RS_VALUE(MJB_EMOJI_SEQUENCE_ZWJ)
MJB_RS_VALUE(MJB_EMOJI_SEQUENCE_TEXT_VARIATION)
MJB_RS_VALUE(MJB_EMOJI_SEQUENCE_EMOJI_VARIATION)
MJB_RS_VALUE(MJB_EMOJI_QUALIFICATION_COMPONENT)
MJB_RS_VALUE(MJB_EMOJI_QUALIFICATION_FULLY_QUALIFIED)
MJB_RS_VALUE(MJB_EMOJI_QUALIFICATION_MINIMALLY_QUALIFIED)
MJB_RS_VALUE(MJB_EMOJI_QUALIFICATION_UNQUALIFIED)
MJB_RS_VALUE(MJB_SCRIPT_SET_RESOLVED)
MJB_RS_VALUE(MJB_SCRIPT_SET_ALL)
MJB_RS_VALUE(MJB_IDENTIFIER_NFKC)
