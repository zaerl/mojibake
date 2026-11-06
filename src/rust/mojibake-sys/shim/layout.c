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
MJB_RS_LAYOUT(encoding, mjb_encoding)
MJB_RS_LAYOUT(status, mjb_status)

MJB_RS_OFFSET(character_numeric, mjb_character, numeric)
MJB_RS_OFFSET(character_titlecase, mjb_character, titlecase)
MJB_RS_OFFSET(locale_id_locale, mjb_locale_id, locale)
