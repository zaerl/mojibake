// The Mojibake library
//
// This file is distributed under the MIT License. See LICENSE for details.

//! Raw FFI bindings to the Mojibake C library.
//!
//! These declarations mirror `src/mojibake.h`. C enums are exposed as `c_int` type aliases with
//! constants rather than Rust enums, because receiving an out-of-range value in a Rust enum is
//! undefined behavior. The layout tests at the bottom of this file compare every struct with the
//! C compiler's view through `shim/layout.c`.
//!
//! Use the safe `mojibake` crate unless you need the raw API.

#![no_std]
#![allow(non_camel_case_types)]

use core::ffi::{c_char, c_int, c_uint, c_ushort, c_void};

pub type mjb_codepoint = u32;
pub type mjb_status = c_int;
pub type mjb_encoding = c_int;
pub type mjb_normalization = c_int;
pub type mjb_quick_check_result = c_int;
pub type mjb_map_case_type = c_int;
pub type mjb_caseless_mode = c_int;
pub type mjb_malformed_policy = c_int;
pub type mjb_text_error = c_int;
pub type mjb_break_type = c_int;
pub type mjb_terminal_width_profile = c_int;
pub type mjb_collation_variable_weighting = c_int;
pub type mjb_collation_strength = c_int;
pub type mjb_identifier_profile = c_int;
pub type mjb_emoji_sequence_type = c_int;
pub type mjb_emoji_qualification = c_int;
pub type mjb_locale = c_int;
pub type mjb_category = c_int;
pub type mjb_canonical_combining_class = c_int;
pub type mjb_decomposition = c_int;
pub type mjb_east_asian_width = c_int;
pub type mjb_script = c_int;
pub type mjb_block = c_int;

pub const MJB_NUL_TERMINATED: usize = usize::MAX;

pub const MJB_STATUS_OK: mjb_status = 0;
pub const MJB_STATUS_INVALID_ARGUMENT: mjb_status = 1;
pub const MJB_STATUS_INVALID_ENCODING: mjb_status = 2;
pub const MJB_STATUS_INVALID_CODEPOINT: mjb_status = 3;
pub const MJB_STATUS_INVALID_FORM: mjb_status = 4;
pub const MJB_STATUS_UNSUPPORTED: mjb_status = 5;
pub const MJB_STATUS_NO_MEMORY: mjb_status = 6;
pub const MJB_STATUS_OVERFLOW: mjb_status = 7;
pub const MJB_STATUS_MALFORMED_INPUT: mjb_status = 8;
pub const MJB_STATUS_OUTPUT_TOO_SMALL: mjb_status = 9;
pub const MJB_STATUS_CALLBACK_STOPPED: mjb_status = 10;
pub const MJB_STATUS_NOT_FOUND: mjb_status = 11;
pub const MJB_STATUS_FEATURE_NOT_ENABLED: mjb_status = 12;
pub const MJB_STATUS_END_OF_INPUT: mjb_status = 13;

pub const MJB_ENC_UNKNOWN: mjb_encoding = 0x0;
pub const MJB_ENC_ASCII: mjb_encoding = 0x1;
pub const MJB_ENC_UTF_8: mjb_encoding = 0x2;
pub const MJB_ENC_UTF_16: mjb_encoding = 0x4;
pub const MJB_ENC_UTF_16BE: mjb_encoding = 0x8;
pub const MJB_ENC_UTF_16LE: mjb_encoding = 0x10;
pub const MJB_ENC_UTF_32: mjb_encoding = 0x20;
pub const MJB_ENC_UTF_32BE: mjb_encoding = 0x40;
pub const MJB_ENC_UTF_32LE: mjb_encoding = 0x80;

pub const MJB_NORMALIZATION_NFC: mjb_normalization = 0;
pub const MJB_NORMALIZATION_NFD: mjb_normalization = 1;
pub const MJB_NORMALIZATION_NFKC: mjb_normalization = 2;
pub const MJB_NORMALIZATION_NFKD: mjb_normalization = 3;

pub const MJB_QC_YES: mjb_quick_check_result = 0x0;
pub const MJB_QC_NO: mjb_quick_check_result = 0x1;
pub const MJB_QC_MAYBE: mjb_quick_check_result = 0x2;

pub const MJB_CASE_NONE: mjb_map_case_type = 0;
pub const MJB_CASE_UPPER: mjb_map_case_type = 1;
pub const MJB_CASE_LOWER: mjb_map_case_type = 2;
pub const MJB_CASE_TITLE: mjb_map_case_type = 3;
pub const MJB_CASE_CASEFOLD: mjb_map_case_type = 4;
pub const MJB_CASE_CASEFOLD_SIMPLE: mjb_map_case_type = 5;

pub const MJB_CASELESS_CANONICAL: mjb_caseless_mode = 0;
pub const MJB_CASELESS_UNNORMALIZED: mjb_caseless_mode = 1;
pub const MJB_CASELESS_COMPATIBILITY: mjb_caseless_mode = 2;
pub const MJB_CASELESS_IDENTIFIER: mjb_caseless_mode = 3;

pub const MJB_MALFORMED_STOP: mjb_malformed_policy = 0;
pub const MJB_MALFORMED_REPLACE: mjb_malformed_policy = 1;
pub const MJB_MALFORMED_SKIP: mjb_malformed_policy = 2;

pub const MJB_BT_NOT_SET: mjb_break_type = 0;
pub const MJB_BT_MANDATORY: mjb_break_type = 1;
pub const MJB_BT_NO_BREAK: mjb_break_type = 2;
pub const MJB_BT_ALLOWED: mjb_break_type = 3;

pub const MJB_TERMINAL_WIDTH_NARROW: mjb_terminal_width_profile = 0;
pub const MJB_TERMINAL_WIDTH_EAST_ASIAN: mjb_terminal_width_profile = 1;

pub const MJB_COLLATION_NON_IGNORABLE: mjb_collation_variable_weighting = 0;
pub const MJB_COLLATION_SHIFTED: mjb_collation_variable_weighting = 1;

pub const MJB_COLLATION_PRIMARY: mjb_collation_strength = 0;
pub const MJB_COLLATION_SECONDARY: mjb_collation_strength = 1;
pub const MJB_COLLATION_TERTIARY: mjb_collation_strength = 2;
pub const MJB_COLLATION_QUATERNARY: mjb_collation_strength = 3;

pub const MJB_IDENTIFIER_DEFAULT: mjb_identifier_profile = 0;
pub const MJB_IDENTIFIER_NFKC: mjb_identifier_profile = 1;

pub const MJB_LOCALE_NOT_SET: mjb_locale = 0;

#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct mjb_text {
    pub buffer: *const c_char,
    pub byte_length: usize,
    pub encoding: mjb_encoding,
}

#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct mjb_buffer {
    pub data: *mut c_void,
    pub size: usize,
    pub encoding: mjb_encoding,
}

#[repr(C)]
#[derive(Debug)]
pub struct mjb_result {
    pub output: *mut c_char,
    pub output_size: usize,
    pub capacity: usize,
    pub owned: bool,
}

impl Default for mjb_result {
    fn default() -> Self {
        Self {
            output: core::ptr::null_mut(),
            output_size: 0,
            capacity: 0,
            owned: false,
        }
    }
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct mjb_diagnostic {
    pub error: mjb_text_error,
    pub byte_offset: usize,
    pub byte_length: usize,
    pub code_unit_offset: usize,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct mjb_character {
    pub codepoint: mjb_codepoint,
    pub name: [c_char; 128],
    pub category: mjb_category,
    pub combining: mjb_canonical_combining_class,
    pub bidirectional: c_ushort,
    pub decomposition: mjb_decomposition,
    pub decimal: c_int,
    pub digit: c_int,
    pub numeric: [c_char; 16],
    pub mirrored: bool,
    pub uppercase: mjb_codepoint,
    pub lowercase: mjb_codepoint,
    pub titlecase: mjb_codepoint,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct mjb_numeric_value {
    pub decimal: c_int,
    pub digit: c_int,
    pub numeric: [c_char; 16],
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct mjb_block_info {
    pub id: mjb_block,
    pub name: [c_char; 128],
    pub start: u32,
    pub end: u32,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct mjb_emoji_properties {
    pub codepoint: mjb_codepoint,
    pub emoji: bool,
    pub presentation: bool,
    pub modifier: bool,
    pub modifier_base: bool,
    pub component: bool,
    pub extended_pictographic: bool,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct mjb_emoji_sequence {
    pub type_: mjb_emoji_sequence_type,
    pub qualification: mjb_emoji_qualification,
    pub codepoint_count: usize,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct mjb_locale_id {
    pub language: [c_char; 9],
    pub extlang: [c_char; 12],
    pub script: [c_char; 5],
    pub region: [c_char; 4],
    pub variant: [c_char; 32],
    pub extensions: [c_char; 128],
    pub private_use: [c_char; 128],
    pub grandfathered: [c_char; 32],
    pub code: [c_char; 3],
    pub locale: mjb_locale,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct mjb_idna_info {
    pub errors: u32,
}

/// Opaque storage for the `mjb_*_break_state` structs.
///
/// Only the two leading `size_t` fields are public in Rust. `shim/layout.c` statically asserts
/// that every C break state fits this storage and has a compatible alignment.
#[repr(C, align(8))]
#[derive(Clone, Copy)]
pub struct mjb_break_state {
    pub index: usize,
    /// Byte offset of the position the last returned break type refers to.
    pub position: usize,
    _tail: [u8; 128],
}

impl Default for mjb_break_state {
    fn default() -> Self {
        Self {
            index: 0,
            position: 0,
            _tail: [0; 128],
        }
    }
}

pub type mjb_grapheme_break_state = mjb_break_state;
pub type mjb_word_break_state = mjb_break_state;
pub type mjb_sentence_break_state = mjb_break_state;
pub type mjb_line_break_state = mjb_break_state;

extern "C" {
    // Library information
    pub fn mjb_version() -> *const c_char;
    pub fn mjb_version_number() -> c_uint;
    pub fn mjb_unicode_version() -> *const c_char;
    pub fn mjb_status_message(status: mjb_status) -> *const c_char;
    pub fn mjb_result_free(result: *mut mjb_result) -> mjb_status;

    // Encoding
    pub fn mjb_validate_encoding(
        text: *const mjb_text,
        diagnostic: *mut mjb_diagnostic,
    ) -> mjb_status;
    pub fn mjb_convert_encoding(
        text: *const mjb_text,
        malformed_policy: mjb_malformed_policy,
        output_encoding: mjb_encoding,
        result: *mut mjb_result,
        diagnostic: *mut mjb_diagnostic,
    ) -> mjb_status;
    pub fn mjb_codepoint_count(
        text: *const mjb_text,
        malformed_policy: mjb_malformed_policy,
        count: *mut usize,
        diagnostic: *mut mjb_diagnostic,
    ) -> mjb_status;

    // Normalization
    pub fn mjb_normalize(
        text: *const mjb_text,
        malformed_policy: mjb_malformed_policy,
        form: mjb_normalization,
        output_encoding: mjb_encoding,
        result: *mut mjb_result,
        diagnostic: *mut mjb_diagnostic,
    ) -> mjb_status;
    pub fn mjb_normalization_quick_check(
        text: *const mjb_text,
        form: mjb_normalization,
        quick_check: *mut mjb_quick_check_result,
    ) -> mjb_status;
    pub fn mjb_nfkc_casefold(
        text: *const mjb_text,
        malformed_policy: mjb_malformed_policy,
        output_encoding: mjb_encoding,
        result: *mut mjb_result,
        diagnostic: *mut mjb_diagnostic,
    ) -> mjb_status;

    // Casing
    pub fn mjb_map_case(
        text: *const mjb_text,
        malformed_policy: mjb_malformed_policy,
        type_: mjb_map_case_type,
        locale: mjb_locale,
        output_encoding: mjb_encoding,
        result: *mut mjb_result,
        diagnostic: *mut mjb_diagnostic,
    ) -> mjb_status;
    pub fn mjb_caseless_match(
        first: *const mjb_text,
        second: *const mjb_text,
        malformed_policy: mjb_malformed_policy,
        mode: mjb_caseless_mode,
        matches: *mut bool,
        first_diagnostic: *mut mjb_diagnostic,
        second_diagnostic: *mut mjb_diagnostic,
    ) -> mjb_status;

    // Locales
    pub fn mjb_locale_parse(id: *const mjb_text, locale: *mut mjb_locale_id) -> mjb_status;
    pub fn mjb_locale_code(locale: mjb_locale) -> *const c_char;

    // Segmentation
    pub fn mjb_grapheme_break_next(
        text: *const mjb_text,
        state: *mut mjb_grapheme_break_state,
    ) -> mjb_break_type;
    pub fn mjb_word_break_next(
        text: *const mjb_text,
        state: *mut mjb_word_break_state,
    ) -> mjb_break_type;
    pub fn mjb_sentence_break_next(
        text: *const mjb_text,
        state: *mut mjb_sentence_break_state,
    ) -> mjb_break_type;
    pub fn mjb_line_break_next(
        text: *const mjb_text,
        state: *mut mjb_line_break_state,
    ) -> mjb_break_type;
    pub fn mjb_grapheme_count(
        text: *const mjb_text,
        malformed_policy: mjb_malformed_policy,
        count: *mut usize,
        diagnostic: *mut mjb_diagnostic,
    ) -> mjb_status;
    pub fn mjb_word_count(
        text: *const mjb_text,
        malformed_policy: mjb_malformed_policy,
        count: *mut usize,
        diagnostic: *mut mjb_diagnostic,
    ) -> mjb_status;
    pub fn mjb_sentence_count(
        text: *const mjb_text,
        malformed_policy: mjb_malformed_policy,
        count: *mut usize,
        diagnostic: *mut mjb_diagnostic,
    ) -> mjb_status;

    // Width
    pub fn mjb_terminal_width(
        text: *const mjb_text,
        malformed_policy: mjb_malformed_policy,
        profile: mjb_terminal_width_profile,
        width: *mut usize,
        diagnostic: *mut mjb_diagnostic,
    ) -> mjb_status;
    pub fn mjb_grapheme_prefix_length_for_width(
        text: *const mjb_text,
        profile: mjb_terminal_width_profile,
        max_columns: usize,
    ) -> usize;
    pub fn mjb_codepoint_east_asian_width(
        codepoint: mjb_codepoint,
        width: *mut mjb_east_asian_width,
    ) -> mjb_status;

    // Character properties
    pub fn mjb_codepoint_info(
        codepoint: mjb_codepoint,
        character: *mut mjb_character,
    ) -> mjb_status;
    pub fn mjb_codepoint_numeric_value(
        codepoint: mjb_codepoint,
        value: *mut mjb_numeric_value,
    ) -> mjb_status;
    pub fn mjb_codepoint_block(codepoint: mjb_codepoint, block: *mut mjb_block_info) -> mjb_status;
    pub fn mjb_codepoint_script(codepoint: mjb_codepoint) -> mjb_script;
    pub fn mjb_script_code(script: mjb_script) -> *const c_char;
    pub fn mjb_codepoint_is_graphic(codepoint: mjb_codepoint) -> bool;
    pub fn mjb_codepoint_is_combining(codepoint: mjb_codepoint) -> bool;
    pub fn mjb_codepoint_is_id_start(codepoint: mjb_codepoint) -> bool;
    pub fn mjb_codepoint_is_id_continue(codepoint: mjb_codepoint) -> bool;
    pub fn mjb_codepoint_is_xid_start(codepoint: mjb_codepoint) -> bool;
    pub fn mjb_codepoint_is_xid_continue(codepoint: mjb_codepoint) -> bool;
    pub fn mjb_is_identifier(text: *const mjb_text, profile: mjb_identifier_profile) -> bool;

    // Emoji
    pub fn mjb_codepoint_emoji_properties(
        codepoint: mjb_codepoint,
        emoji: *mut mjb_emoji_properties,
    ) -> mjb_status;
    pub fn mjb_emoji_sequence_info(
        text: *const mjb_text,
        emoji: *mut mjb_emoji_sequence,
    ) -> mjb_status;
    pub fn mjb_is_rgi_emoji(text: *const mjb_text) -> bool;

    // Collation
    pub fn mjb_collation_compare(
        first: *const mjb_text,
        second: *const mjb_text,
        malformed_policy: mjb_malformed_policy,
        variable_weighting: mjb_collation_variable_weighting,
        strength: mjb_collation_strength,
        order: *mut c_int,
        first_diagnostic: *mut mjb_diagnostic,
        second_diagnostic: *mut mjb_diagnostic,
    ) -> mjb_status;
    pub fn mjb_collation_key(
        text: *const mjb_text,
        malformed_policy: mjb_malformed_policy,
        variable_weighting: mjb_collation_variable_weighting,
        strength: mjb_collation_strength,
        result: *mut mjb_result,
        diagnostic: *mut mjb_diagnostic,
    ) -> mjb_status;

    // IDNA
    pub fn mjb_idna_to_ascii(
        text: *const mjb_text,
        output_encoding: mjb_encoding,
        info: *mut mjb_idna_info,
        result: *mut mjb_result,
    ) -> mjb_status;
    pub fn mjb_idna_to_unicode(
        text: *const mjb_text,
        output_encoding: mjb_encoding,
        info: *mut mjb_idna_info,
        result: *mut mjb_result,
    ) -> mjb_status;

    // Security
    pub fn mjb_confusable_skeleton(
        text: *const mjb_text,
        output_encoding: mjb_encoding,
        result: *mut mjb_result,
    ) -> mjb_status;
    pub fn mjb_confusable_match(
        first: *const mjb_text,
        second: *const mjb_text,
        malformed_policy: mjb_malformed_policy,
        confusable: *mut bool,
        first_diagnostic: *mut mjb_diagnostic,
        second_diagnostic: *mut mjb_diagnostic,
    ) -> mjb_status;
}

#[cfg(test)]
mod layout {
    use super::*;
    use core::mem::{align_of, offset_of, size_of};

    macro_rules! probes {
        ($($name:ident),* $(,)?) => {
            extern "C" {
                $(
                    fn $name() -> usize;
                )*
            }
        };
    }

    probes!(
        mjb_rs_sizeof_text,
        mjb_rs_alignof_text,
        mjb_rs_sizeof_buffer,
        mjb_rs_alignof_buffer,
        mjb_rs_sizeof_result,
        mjb_rs_alignof_result,
        mjb_rs_sizeof_diagnostic,
        mjb_rs_alignof_diagnostic,
        mjb_rs_sizeof_character,
        mjb_rs_alignof_character,
        mjb_rs_sizeof_numeric_value,
        mjb_rs_alignof_numeric_value,
        mjb_rs_sizeof_block_info,
        mjb_rs_alignof_block_info,
        mjb_rs_sizeof_emoji_properties,
        mjb_rs_alignof_emoji_properties,
        mjb_rs_sizeof_emoji_sequence,
        mjb_rs_alignof_emoji_sequence,
        mjb_rs_sizeof_locale_id,
        mjb_rs_alignof_locale_id,
        mjb_rs_sizeof_idna_info,
        mjb_rs_alignof_idna_info,
        mjb_rs_sizeof_encoding,
        mjb_rs_sizeof_status,
        mjb_rs_offsetof_character_numeric,
        mjb_rs_offsetof_character_titlecase,
        mjb_rs_offsetof_locale_id_locale,
    );

    macro_rules! check {
        ($ty:ty, $size:ident, $align:ident) => {
            unsafe {
                assert_eq!(
                    size_of::<$ty>(),
                    $size(),
                    concat!("size of ", stringify!($ty))
                );
                assert_eq!(
                    align_of::<$ty>(),
                    $align(),
                    concat!("align of ", stringify!($ty))
                );
            }
        };
    }

    #[test]
    fn structs_match_c() {
        check!(mjb_text, mjb_rs_sizeof_text, mjb_rs_alignof_text);
        check!(mjb_buffer, mjb_rs_sizeof_buffer, mjb_rs_alignof_buffer);
        check!(mjb_result, mjb_rs_sizeof_result, mjb_rs_alignof_result);
        check!(
            mjb_diagnostic,
            mjb_rs_sizeof_diagnostic,
            mjb_rs_alignof_diagnostic
        );
        check!(
            mjb_character,
            mjb_rs_sizeof_character,
            mjb_rs_alignof_character
        );
        check!(
            mjb_numeric_value,
            mjb_rs_sizeof_numeric_value,
            mjb_rs_alignof_numeric_value
        );
        check!(
            mjb_block_info,
            mjb_rs_sizeof_block_info,
            mjb_rs_alignof_block_info
        );
        check!(
            mjb_emoji_properties,
            mjb_rs_sizeof_emoji_properties,
            mjb_rs_alignof_emoji_properties
        );
        check!(
            mjb_emoji_sequence,
            mjb_rs_sizeof_emoji_sequence,
            mjb_rs_alignof_emoji_sequence
        );
        check!(
            mjb_locale_id,
            mjb_rs_sizeof_locale_id,
            mjb_rs_alignof_locale_id
        );
        check!(
            mjb_idna_info,
            mjb_rs_sizeof_idna_info,
            mjb_rs_alignof_idna_info
        );
    }

    #[test]
    fn enums_and_offsets_match_c() {
        unsafe {
            assert_eq!(size_of::<mjb_encoding>(), mjb_rs_sizeof_encoding());
            assert_eq!(size_of::<mjb_status>(), mjb_rs_sizeof_status());
            assert_eq!(
                offset_of!(mjb_character, numeric),
                mjb_rs_offsetof_character_numeric()
            );
            assert_eq!(
                offset_of!(mjb_character, titlecase),
                mjb_rs_offsetof_character_titlecase()
            );
            assert_eq!(
                offset_of!(mjb_locale_id, locale),
                mjb_rs_offsetof_locale_id_locale()
            );
        }
    }
}
