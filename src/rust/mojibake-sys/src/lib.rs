// The Mojibake library
//
// This file is distributed under the MIT License. See LICENSE for details.

//! Raw FFI bindings to the Mojibake C library.
//!
//! These declarations mirror `src/mojibake.h`. The functions are generated from
//! `utils/generate/functions.ts` into `src/functions.rs`; the types and constants are written by
//! hand. C enums are exposed as `c_int` type aliases with constants rather than Rust enums,
//! because receiving an out-of-range value in a Rust enum is undefined behavior. The layout tests at the bottom of this file compare every struct with the
//! C compiler's view through `shim/layout.c`.
//!
//! Use the safe `mojibake` crate unless you need the raw API.

#![no_std]
#![allow(non_camel_case_types)]

use core::ffi::{c_char, c_int, c_ushort, c_void};

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
pub type mjb_plane = c_int;
pub type mjb_property = c_int;
pub type mjb_filter_flags = c_int;
pub type mjb_character_position = c_int;
pub type mjb_direction = c_int;
pub type mjb_bidi_class = c_int;
pub type mjb_script_set_kind = c_int;

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

pub const MJB_FILTER_NONE: mjb_filter_flags = 0x0;
pub const MJB_FILTER_NORMALIZE: mjb_filter_flags = 0x1;
pub const MJB_FILTER_SPACES: mjb_filter_flags = 0x2;
pub const MJB_FILTER_COLLAPSE_SPACES: mjb_filter_flags = 0x4;
pub const MJB_FILTER_CONTROLS: mjb_filter_flags = 0x8;
pub const MJB_FILTER_NUMERIC: mjb_filter_flags = 0x10;
pub const MJB_FILTER_LIMIT_COMBINING: mjb_filter_flags = 0x20;

pub const MJB_EMOJI_SEQUENCE_NONE: mjb_emoji_sequence_type = 0;
pub const MJB_EMOJI_SEQUENCE_BASIC: mjb_emoji_sequence_type = 1;
pub const MJB_EMOJI_SEQUENCE_KEYCAP: mjb_emoji_sequence_type = 2;
pub const MJB_EMOJI_SEQUENCE_FLAG: mjb_emoji_sequence_type = 3;
pub const MJB_EMOJI_SEQUENCE_TAG: mjb_emoji_sequence_type = 4;
pub const MJB_EMOJI_SEQUENCE_MODIFIER: mjb_emoji_sequence_type = 5;
pub const MJB_EMOJI_SEQUENCE_ZWJ: mjb_emoji_sequence_type = 6;
pub const MJB_EMOJI_SEQUENCE_TEXT_VARIATION: mjb_emoji_sequence_type = 7;
pub const MJB_EMOJI_SEQUENCE_EMOJI_VARIATION: mjb_emoji_sequence_type = 8;

pub const MJB_EMOJI_QUALIFICATION_NONE: mjb_emoji_qualification = 0;
pub const MJB_EMOJI_QUALIFICATION_COMPONENT: mjb_emoji_qualification = 1;
pub const MJB_EMOJI_QUALIFICATION_FULLY_QUALIFIED: mjb_emoji_qualification = 2;
pub const MJB_EMOJI_QUALIFICATION_MINIMALLY_QUALIFIED: mjb_emoji_qualification = 3;
pub const MJB_EMOJI_QUALIFICATION_UNQUALIFIED: mjb_emoji_qualification = 4;

pub const MJB_SCRIPT_SET_EMPTY: mjb_script_set_kind = 0;
pub const MJB_SCRIPT_SET_RESOLVED: mjb_script_set_kind = 1;
pub const MJB_SCRIPT_SET_ALL: mjb_script_set_kind = 2;

pub const MJB_NUMBER_NOT_VALID: c_int = -1;

pub type mjb_idna_error = u32;

pub const MJB_IDNA_ERROR_NONE: mjb_idna_error = 0x0;
pub const MJB_IDNA_ERROR_PUNYCODE: mjb_idna_error = 0x1;
pub const MJB_IDNA_ERROR_EMPTY_LABEL: mjb_idna_error = 0x2;
pub const MJB_IDNA_ERROR_HYPHEN: mjb_idna_error = 0x4;
pub const MJB_IDNA_ERROR_NOT_NFC: mjb_idna_error = 0x8;
pub const MJB_IDNA_ERROR_LEADING_MARK: mjb_idna_error = 0x10;
pub const MJB_IDNA_ERROR_DISALLOWED: mjb_idna_error = 0x20;
pub const MJB_IDNA_ERROR_STD3: mjb_idna_error = 0x40;
pub const MJB_IDNA_ERROR_CONTEXTJ: mjb_idna_error = 0x80;
pub const MJB_IDNA_ERROR_BIDI: mjb_idna_error = 0x100;
pub const MJB_IDNA_ERROR_LABEL_LENGTH: mjb_idna_error = 0x200;
pub const MJB_IDNA_ERROR_DOMAIN_LENGTH: mjb_idna_error = 0x400;

pub const MJB_PR_BIDI_CLASS_NOT_SET: mjb_bidi_class = 0;
pub const MJB_PR_BIDI_CLASS_L: mjb_bidi_class = 1;
pub const MJB_PR_BIDI_CLASS_R: mjb_bidi_class = 2;
pub const MJB_PR_BIDI_CLASS_AL: mjb_bidi_class = 3;
pub const MJB_PR_BIDI_CLASS_EN: mjb_bidi_class = 4;
pub const MJB_PR_BIDI_CLASS_ES: mjb_bidi_class = 5;
pub const MJB_PR_BIDI_CLASS_ET: mjb_bidi_class = 6;
pub const MJB_PR_BIDI_CLASS_AN: mjb_bidi_class = 7;
pub const MJB_PR_BIDI_CLASS_CS: mjb_bidi_class = 8;
pub const MJB_PR_BIDI_CLASS_NSM: mjb_bidi_class = 9;
pub const MJB_PR_BIDI_CLASS_BN: mjb_bidi_class = 10;
pub const MJB_PR_BIDI_CLASS_B: mjb_bidi_class = 11;
pub const MJB_PR_BIDI_CLASS_S: mjb_bidi_class = 12;
pub const MJB_PR_BIDI_CLASS_WS: mjb_bidi_class = 13;
pub const MJB_PR_BIDI_CLASS_ON: mjb_bidi_class = 14;
pub const MJB_PR_BIDI_CLASS_LRE: mjb_bidi_class = 15;
pub const MJB_PR_BIDI_CLASS_LRO: mjb_bidi_class = 16;
pub const MJB_PR_BIDI_CLASS_RLE: mjb_bidi_class = 17;
pub const MJB_PR_BIDI_CLASS_RLO: mjb_bidi_class = 18;
pub const MJB_PR_BIDI_CLASS_PDF: mjb_bidi_class = 19;
pub const MJB_PR_BIDI_CLASS_LRI: mjb_bidi_class = 20;
pub const MJB_PR_BIDI_CLASS_RLI: mjb_bidi_class = 21;
pub const MJB_PR_BIDI_CLASS_FSI: mjb_bidi_class = 22;
pub const MJB_PR_BIDI_CLASS_PDI: mjb_bidi_class = 23;

pub const MJB_DECOMPOSITION_NONE: mjb_decomposition = 0;
pub const MJB_DECOMPOSITION_CANONICAL: mjb_decomposition = 1;
pub const MJB_DECOMPOSITION_CIRCLE: mjb_decomposition = 2;
pub const MJB_DECOMPOSITION_COMPAT: mjb_decomposition = 3;
pub const MJB_DECOMPOSITION_FINAL: mjb_decomposition = 4;
pub const MJB_DECOMPOSITION_FONT: mjb_decomposition = 5;
pub const MJB_DECOMPOSITION_FRACTION: mjb_decomposition = 6;
pub const MJB_DECOMPOSITION_INITIAL: mjb_decomposition = 7;
pub const MJB_DECOMPOSITION_ISOLATED: mjb_decomposition = 8;
pub const MJB_DECOMPOSITION_MEDIAL: mjb_decomposition = 9;
pub const MJB_DECOMPOSITION_NARROW: mjb_decomposition = 10;
pub const MJB_DECOMPOSITION_NOBREAK: mjb_decomposition = 11;
pub const MJB_DECOMPOSITION_SMALL: mjb_decomposition = 12;
pub const MJB_DECOMPOSITION_SQUARE: mjb_decomposition = 13;
pub const MJB_DECOMPOSITION_SUB: mjb_decomposition = 14;
pub const MJB_DECOMPOSITION_SUPER: mjb_decomposition = 15;
pub const MJB_DECOMPOSITION_VERTICAL: mjb_decomposition = 16;
pub const MJB_DECOMPOSITION_WIDE: mjb_decomposition = 17;

pub const MJB_LOCALE_NOT_SET: mjb_locale = 0;

pub const MJB_DIRECTION_LTR: mjb_direction = 0;
pub const MJB_DIRECTION_RTL: mjb_direction = 1;
pub const MJB_DIRECTION_AUTO: mjb_direction = 2;

pub type mjb_alloc_fn =
    Option<unsafe extern "C" fn(context: *mut c_void, size: usize) -> *mut c_void>;
pub type mjb_realloc_fn = Option<
    unsafe extern "C" fn(
        context: *mut c_void,
        ptr: *mut c_void,
        old_size: usize,
        new_size: usize,
    ) -> *mut c_void,
>;
pub type mjb_free_fn =
    Option<unsafe extern "C" fn(context: *mut c_void, ptr: *mut c_void, size: usize)>;

#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct mjb_allocator {
    pub context: *mut c_void,
    pub alloc: mjb_alloc_fn,
    pub realloc: mjb_realloc_fn,
    pub free: mjb_free_fn,
}

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

pub type mjb_for_each_codepoint_fn = Option<
    unsafe extern "C" fn(
        character: *const mjb_character,
        position: mjb_character_position,
        context: *mut c_void,
    ) -> bool,
>;

#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct mjb_bidi_char {
    pub codepoint: mjb_codepoint,
    pub byte_offset: usize,
    pub level: u8,
    pub resolved_class: mjb_bidi_class,
    pub mirroring_glyph: mjb_codepoint,
}

#[repr(C)]
#[derive(Debug)]
pub struct mjb_bidi_paragraph {
    pub chars: *mut mjb_bidi_char,
    pub count: usize,
    pub paragraph_level: u8,
    pub direction: mjb_direction,
}

impl Default for mjb_bidi_paragraph {
    fn default() -> Self {
        Self {
            chars: core::ptr::null_mut(),
            count: 0,
            paragraph_level: 0,
            direction: MJB_DIRECTION_LTR,
        }
    }
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct mjb_bidi_run {
    pub start: usize,
    pub end: usize,
    pub level: u8,
    pub direction: mjb_direction,
}

pub type mjb_grapheme_break_state = mjb_break_state;
pub type mjb_word_break_state = mjb_break_state;
pub type mjb_sentence_break_state = mjb_break_state;
pub type mjb_line_break_state = mjb_break_state;

mod functions;

pub use functions::*;

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
        mjb_rs_sizeof_allocator,
        mjb_rs_alignof_allocator,
        mjb_rs_sizeof_bidi_char,
        mjb_rs_alignof_bidi_char,
        mjb_rs_sizeof_bidi_paragraph,
        mjb_rs_alignof_bidi_paragraph,
        mjb_rs_sizeof_bidi_run,
        mjb_rs_alignof_bidi_run,
        mjb_rs_sizeof_encoding,
        mjb_rs_sizeof_status,
        mjb_rs_offsetof_character_numeric,
        mjb_rs_offsetof_character_titlecase,
        mjb_rs_offsetof_locale_id_locale,
        mjb_rs_offsetof_bidi_char_byte_offset,
        mjb_rs_offsetof_bidi_char_level,
        mjb_rs_offsetof_bidi_char_resolved_class,
        mjb_rs_offsetof_bidi_char_mirroring_glyph,
        mjb_rs_offsetof_bidi_paragraph_count,
        mjb_rs_offsetof_bidi_paragraph_paragraph_level,
        mjb_rs_offsetof_bidi_paragraph_direction,
        mjb_rs_offsetof_bidi_run_end,
        mjb_rs_offsetof_bidi_run_level,
        mjb_rs_offsetof_bidi_run_direction,
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
        check!(
            mjb_allocator,
            mjb_rs_sizeof_allocator,
            mjb_rs_alignof_allocator
        );
        check!(
            mjb_bidi_char,
            mjb_rs_sizeof_bidi_char,
            mjb_rs_alignof_bidi_char
        );
        check!(
            mjb_bidi_paragraph,
            mjb_rs_sizeof_bidi_paragraph,
            mjb_rs_alignof_bidi_paragraph
        );
        check!(
            mjb_bidi_run,
            mjb_rs_sizeof_bidi_run,
            mjb_rs_alignof_bidi_run
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

    macro_rules! check_offsets {
        ($($ty:ident . $field:ident => $probe:ident),* $(,)?) => {
            $(
                assert_eq!(
                    offset_of!($ty, $field),
                    unsafe { $probe() },
                    concat!("offset of ", stringify!($ty), ".", stringify!($field))
                );
            )*
        };
    }

    macro_rules! check_values {
        ($($name:expr => $probe:ident),* $(,)?) => {
            extern "C" {
                $(
                    fn $probe() -> c_int;
                )*
            }

            $(
                assert_eq!($name, unsafe { $probe() }, stringify!($name));
            )*
        };
    }

    #[test]
    fn constants_match_c() {
        check_values!(
            MJB_FILTER_NORMALIZE => mjb_rs_value_MJB_FILTER_NORMALIZE,
            MJB_FILTER_SPACES => mjb_rs_value_MJB_FILTER_SPACES,
            MJB_FILTER_COLLAPSE_SPACES => mjb_rs_value_MJB_FILTER_COLLAPSE_SPACES,
            MJB_FILTER_CONTROLS => mjb_rs_value_MJB_FILTER_CONTROLS,
            MJB_FILTER_NUMERIC => mjb_rs_value_MJB_FILTER_NUMERIC,
            MJB_FILTER_LIMIT_COMBINING => mjb_rs_value_MJB_FILTER_LIMIT_COMBINING,
            MJB_EMOJI_SEQUENCE_BASIC => mjb_rs_value_MJB_EMOJI_SEQUENCE_BASIC,
            MJB_EMOJI_SEQUENCE_KEYCAP => mjb_rs_value_MJB_EMOJI_SEQUENCE_KEYCAP,
            MJB_EMOJI_SEQUENCE_FLAG => mjb_rs_value_MJB_EMOJI_SEQUENCE_FLAG,
            MJB_EMOJI_SEQUENCE_TAG => mjb_rs_value_MJB_EMOJI_SEQUENCE_TAG,
            MJB_EMOJI_SEQUENCE_MODIFIER => mjb_rs_value_MJB_EMOJI_SEQUENCE_MODIFIER,
            MJB_EMOJI_SEQUENCE_ZWJ => mjb_rs_value_MJB_EMOJI_SEQUENCE_ZWJ,
            MJB_EMOJI_SEQUENCE_TEXT_VARIATION => mjb_rs_value_MJB_EMOJI_SEQUENCE_TEXT_VARIATION,
            MJB_EMOJI_SEQUENCE_EMOJI_VARIATION => mjb_rs_value_MJB_EMOJI_SEQUENCE_EMOJI_VARIATION,
            MJB_EMOJI_QUALIFICATION_COMPONENT => mjb_rs_value_MJB_EMOJI_QUALIFICATION_COMPONENT,
            MJB_EMOJI_QUALIFICATION_FULLY_QUALIFIED =>
                mjb_rs_value_MJB_EMOJI_QUALIFICATION_FULLY_QUALIFIED,
            MJB_EMOJI_QUALIFICATION_MINIMALLY_QUALIFIED =>
                mjb_rs_value_MJB_EMOJI_QUALIFICATION_MINIMALLY_QUALIFIED,
            MJB_EMOJI_QUALIFICATION_UNQUALIFIED => mjb_rs_value_MJB_EMOJI_QUALIFICATION_UNQUALIFIED,
            MJB_SCRIPT_SET_RESOLVED => mjb_rs_value_MJB_SCRIPT_SET_RESOLVED,
            MJB_SCRIPT_SET_ALL => mjb_rs_value_MJB_SCRIPT_SET_ALL,
            MJB_IDENTIFIER_NFKC => mjb_rs_value_MJB_IDENTIFIER_NFKC,
            MJB_NUMBER_NOT_VALID => mjb_rs_value_MJB_NUMBER_NOT_VALID,
            MJB_IDNA_ERROR_PUNYCODE as c_int => mjb_rs_value_MJB_IDNA_ERROR_PUNYCODE,
            MJB_IDNA_ERROR_EMPTY_LABEL as c_int => mjb_rs_value_MJB_IDNA_ERROR_EMPTY_LABEL,
            MJB_IDNA_ERROR_HYPHEN as c_int => mjb_rs_value_MJB_IDNA_ERROR_HYPHEN,
            MJB_IDNA_ERROR_NOT_NFC as c_int => mjb_rs_value_MJB_IDNA_ERROR_NOT_NFC,
            MJB_IDNA_ERROR_LEADING_MARK as c_int => mjb_rs_value_MJB_IDNA_ERROR_LEADING_MARK,
            MJB_IDNA_ERROR_DISALLOWED as c_int => mjb_rs_value_MJB_IDNA_ERROR_DISALLOWED,
            MJB_IDNA_ERROR_STD3 as c_int => mjb_rs_value_MJB_IDNA_ERROR_STD3,
            MJB_IDNA_ERROR_CONTEXTJ as c_int => mjb_rs_value_MJB_IDNA_ERROR_CONTEXTJ,
            MJB_IDNA_ERROR_BIDI as c_int => mjb_rs_value_MJB_IDNA_ERROR_BIDI,
            MJB_IDNA_ERROR_LABEL_LENGTH as c_int => mjb_rs_value_MJB_IDNA_ERROR_LABEL_LENGTH,
            MJB_IDNA_ERROR_DOMAIN_LENGTH as c_int => mjb_rs_value_MJB_IDNA_ERROR_DOMAIN_LENGTH,
            MJB_PR_BIDI_CLASS_L => mjb_rs_value_MJB_PR_BIDI_CLASS_L,
            MJB_PR_BIDI_CLASS_R => mjb_rs_value_MJB_PR_BIDI_CLASS_R,
            MJB_PR_BIDI_CLASS_AL => mjb_rs_value_MJB_PR_BIDI_CLASS_AL,
            MJB_PR_BIDI_CLASS_EN => mjb_rs_value_MJB_PR_BIDI_CLASS_EN,
            MJB_PR_BIDI_CLASS_ES => mjb_rs_value_MJB_PR_BIDI_CLASS_ES,
            MJB_PR_BIDI_CLASS_ET => mjb_rs_value_MJB_PR_BIDI_CLASS_ET,
            MJB_PR_BIDI_CLASS_AN => mjb_rs_value_MJB_PR_BIDI_CLASS_AN,
            MJB_PR_BIDI_CLASS_CS => mjb_rs_value_MJB_PR_BIDI_CLASS_CS,
            MJB_PR_BIDI_CLASS_NSM => mjb_rs_value_MJB_PR_BIDI_CLASS_NSM,
            MJB_PR_BIDI_CLASS_BN => mjb_rs_value_MJB_PR_BIDI_CLASS_BN,
            MJB_PR_BIDI_CLASS_B => mjb_rs_value_MJB_PR_BIDI_CLASS_B,
            MJB_PR_BIDI_CLASS_S => mjb_rs_value_MJB_PR_BIDI_CLASS_S,
            MJB_PR_BIDI_CLASS_WS => mjb_rs_value_MJB_PR_BIDI_CLASS_WS,
            MJB_PR_BIDI_CLASS_ON => mjb_rs_value_MJB_PR_BIDI_CLASS_ON,
            MJB_PR_BIDI_CLASS_LRE => mjb_rs_value_MJB_PR_BIDI_CLASS_LRE,
            MJB_PR_BIDI_CLASS_LRO => mjb_rs_value_MJB_PR_BIDI_CLASS_LRO,
            MJB_PR_BIDI_CLASS_RLE => mjb_rs_value_MJB_PR_BIDI_CLASS_RLE,
            MJB_PR_BIDI_CLASS_RLO => mjb_rs_value_MJB_PR_BIDI_CLASS_RLO,
            MJB_PR_BIDI_CLASS_PDF => mjb_rs_value_MJB_PR_BIDI_CLASS_PDF,
            MJB_PR_BIDI_CLASS_LRI => mjb_rs_value_MJB_PR_BIDI_CLASS_LRI,
            MJB_PR_BIDI_CLASS_RLI => mjb_rs_value_MJB_PR_BIDI_CLASS_RLI,
            MJB_PR_BIDI_CLASS_FSI => mjb_rs_value_MJB_PR_BIDI_CLASS_FSI,
            MJB_PR_BIDI_CLASS_PDI => mjb_rs_value_MJB_PR_BIDI_CLASS_PDI,
            MJB_DECOMPOSITION_CANONICAL => mjb_rs_value_MJB_DECOMPOSITION_CANONICAL,
            MJB_DECOMPOSITION_CIRCLE => mjb_rs_value_MJB_DECOMPOSITION_CIRCLE,
            MJB_DECOMPOSITION_COMPAT => mjb_rs_value_MJB_DECOMPOSITION_COMPAT,
            MJB_DECOMPOSITION_FINAL => mjb_rs_value_MJB_DECOMPOSITION_FINAL,
            MJB_DECOMPOSITION_FONT => mjb_rs_value_MJB_DECOMPOSITION_FONT,
            MJB_DECOMPOSITION_FRACTION => mjb_rs_value_MJB_DECOMPOSITION_FRACTION,
            MJB_DECOMPOSITION_INITIAL => mjb_rs_value_MJB_DECOMPOSITION_INITIAL,
            MJB_DECOMPOSITION_ISOLATED => mjb_rs_value_MJB_DECOMPOSITION_ISOLATED,
            MJB_DECOMPOSITION_MEDIAL => mjb_rs_value_MJB_DECOMPOSITION_MEDIAL,
            MJB_DECOMPOSITION_NARROW => mjb_rs_value_MJB_DECOMPOSITION_NARROW,
            MJB_DECOMPOSITION_NOBREAK => mjb_rs_value_MJB_DECOMPOSITION_NOBREAK,
            MJB_DECOMPOSITION_SMALL => mjb_rs_value_MJB_DECOMPOSITION_SMALL,
            MJB_DECOMPOSITION_SQUARE => mjb_rs_value_MJB_DECOMPOSITION_SQUARE,
            MJB_DECOMPOSITION_SUB => mjb_rs_value_MJB_DECOMPOSITION_SUB,
            MJB_DECOMPOSITION_SUPER => mjb_rs_value_MJB_DECOMPOSITION_SUPER,
            MJB_DECOMPOSITION_VERTICAL => mjb_rs_value_MJB_DECOMPOSITION_VERTICAL,
            MJB_DECOMPOSITION_WIDE => mjb_rs_value_MJB_DECOMPOSITION_WIDE,
        );
    }

    #[test]
    fn bidi_offsets_match_c() {
        check_offsets!(
            mjb_bidi_char.byte_offset => mjb_rs_offsetof_bidi_char_byte_offset,
            mjb_bidi_char.level => mjb_rs_offsetof_bidi_char_level,
            mjb_bidi_char.resolved_class => mjb_rs_offsetof_bidi_char_resolved_class,
            mjb_bidi_char.mirroring_glyph => mjb_rs_offsetof_bidi_char_mirroring_glyph,
            mjb_bidi_paragraph.count => mjb_rs_offsetof_bidi_paragraph_count,
            mjb_bidi_paragraph.paragraph_level => mjb_rs_offsetof_bidi_paragraph_paragraph_level,
            mjb_bidi_paragraph.direction => mjb_rs_offsetof_bidi_paragraph_direction,
            mjb_bidi_run.end => mjb_rs_offsetof_bidi_run_end,
            mjb_bidi_run.level => mjb_rs_offsetof_bidi_run_level,
            mjb_bidi_run.direction => mjb_rs_offsetof_bidi_run_direction,
        );
    }
}
