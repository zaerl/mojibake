// The Mojibake library
//
// This file is distributed under the MIT License. See LICENSE for details.

use mojibake_sys as sys;

use crate::ffi;

/// How East Asian Width Ambiguous characters are measured (UAX #11).
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash, Default)]
pub enum WidthProfile {
    /// Ambiguous characters take one terminal cell.
    #[default]
    Narrow,
    /// Ambiguous characters take two terminal cells, as in CJK terminals.
    EastAsian,
}

impl WidthProfile {
    fn raw(self) -> sys::mjb_terminal_width_profile {
        match self {
            Self::Narrow => sys::MJB_TERMINAL_WIDTH_NARROW,
            Self::EastAsian => sys::MJB_TERMINAL_WIDTH_EAST_ASIAN,
        }
    }
}

/// The estimated terminal-cell width of printable, single-line text.
pub fn terminal_width(s: &str, profile: WidthProfile) -> usize {
    let text = ffi::text(s);
    let mut width = 0;

    // SAFETY: all pointers are valid for the duration of the call; a NULL diagnostic is allowed.
    let status = unsafe {
        sys::mjb_terminal_width(
            &text,
            sys::MJB_MALFORMED_REPLACE,
            profile.raw(),
            &mut width,
            std::ptr::null_mut(),
        )
    };
    debug_assert_eq!(status, sys::MJB_STATUS_OK);

    width
}

/// The longest prefix of `s` that fits in `max_columns` cells without splitting a grapheme.
pub fn truncate_to_width(s: &str, profile: WidthProfile, max_columns: usize) -> &str {
    let text = ffi::text(s);
    // SAFETY: `text` borrows `s` for the duration of the call.
    let length =
        unsafe { sys::mjb_grapheme_prefix_length_for_width(&text, profile.raw(), max_columns) };

    // The library returns a grapheme boundary, which is always a char boundary.
    s.get(..length.min(s.len())).unwrap_or("")
}
