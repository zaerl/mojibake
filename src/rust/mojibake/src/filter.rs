// The Mojibake library
//
// This file is distributed under the MIT License. See LICENSE for details.

use std::borrow::Cow;
use std::fmt;
use std::ops::{BitOr, BitOrAssign};

use mojibake_sys as sys;

use crate::error::Result;
use crate::ffi;

/// A set of text filters for [`filter`]. Combine them with `|`.
#[derive(Clone, Copy, Default, PartialEq, Eq, Hash)]
pub struct Filter(sys::mjb_filter_flags);

impl Filter {
    /// No filtering: the text is returned unchanged.
    pub const NONE: Self = Self(sys::MJB_FILTER_NONE);
    /// Normalize to NFC.
    pub const NORMALIZE: Self = Self(sys::MJB_FILTER_NORMALIZE);
    /// Replace every whitespace character with U+0020 SPACE.
    pub const SPACES: Self = Self(sys::MJB_FILTER_SPACES);
    /// Replace each run of whitespace with a single U+0020 SPACE.
    pub const COLLAPSE_SPACES: Self = Self(sys::MJB_FILTER_COLLAPSE_SPACES);
    /// Remove `Cc` control characters other than tab, line feed, vertical tab, form feed, and
    /// carriage return.
    pub const CONTROLS: Self = Self(sys::MJB_FILTER_CONTROLS);
    /// Replace decimal digits of every script with ASCII `0`-`9`.
    pub const NUMERIC: Self = Self(sys::MJB_FILTER_NUMERIC);
    /// Keep at most four consecutive combining marks in a run, which defuses Zalgo-style text
    /// while keeping ordinary accents and stacked marks.
    pub const LIMIT_COMBINING: Self = Self(sys::MJB_FILTER_LIMIT_COMBINING);

    /// True if every filter in `other` is also in `self`.
    #[must_use]
    pub const fn contains(self, other: Self) -> bool {
        self.0 & other.0 == other.0
    }

    /// True if no filter is set.
    #[must_use]
    pub const fn is_empty(self) -> bool {
        self.0 == 0
    }
}

impl BitOr for Filter {
    type Output = Self;

    fn bitor(self, rhs: Self) -> Self {
        Self(self.0 | rhs.0)
    }
}

impl BitOrAssign for Filter {
    fn bitor_assign(&mut self, rhs: Self) {
        self.0 |= rhs.0;
    }
}

impl fmt::Debug for Filter {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        const NAMES: [(Filter, &str); 6] = [
            (Filter::NORMALIZE, "NORMALIZE"),
            (Filter::SPACES, "SPACES"),
            (Filter::COLLAPSE_SPACES, "COLLAPSE_SPACES"),
            (Filter::CONTROLS, "CONTROLS"),
            (Filter::NUMERIC, "NUMERIC"),
            (Filter::LIMIT_COMBINING, "LIMIT_COMBINING"),
        ];

        if self.is_empty() {
            return f.write_str("Filter(NONE)");
        }

        f.write_str("Filter(")?;
        let mut first = true;

        for (flag, name) in NAMES {
            if self.contains(flag) {
                if !first {
                    f.write_str(" | ")?;
                }

                f.write_str(name)?;
                first = false;
            }
        }

        f.write_str(")")
    }
}

/// Apply `filters` to `s`: `filter("a \t b", Filter::COLLAPSE_SPACES)` is `"a b"`.
///
/// The input is borrowed when no filter changes it.
///
/// # Errors
///
/// [`Error::NoMemory`](crate::Error::NoMemory) if the library cannot allocate memory.
pub fn filter(s: &str, filters: Filter) -> Result<Cow<'_, str>> {
    ffi::transform(s, |text, result, diagnostic| unsafe {
        // SAFETY: all pointers are valid for the duration of the call.
        sys::mjb_filter(
            text,
            sys::MJB_MALFORMED_STOP,
            filters.0,
            sys::MJB_ENC_UTF_8,
            result,
            diagnostic,
        )
    })
}
