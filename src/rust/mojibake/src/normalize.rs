// The Mojibake library
//
// This file is distributed under the MIT License. See LICENSE for details.

use std::borrow::Cow;

use mojibake_sys as sys;

use crate::error::{check, Result};
use crate::ffi;

/// A Unicode normalization form (UAX #15).
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub enum Form {
    /// Canonical decomposition followed by canonical composition.
    Nfc,
    /// Canonical decomposition.
    Nfd,
    /// Compatibility decomposition followed by canonical composition.
    Nfkc,
    /// Compatibility decomposition.
    Nfkd,
}

impl Form {
    fn raw(self) -> sys::mjb_normalization {
        match self {
            Self::Nfc => sys::MJB_NORMALIZATION_NFC,
            Self::Nfd => sys::MJB_NORMALIZATION_NFD,
            Self::Nfkc => sys::MJB_NORMALIZATION_NFKC,
            Self::Nfkd => sys::MJB_NORMALIZATION_NFKD,
        }
    }
}

/// The answer of a normalization quick check (UAX #15 section 9).
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub enum QuickCheck {
    /// The string is in the form.
    Yes,
    /// The string is not in the form.
    No,
    /// The quick check cannot decide; normalize and compare to be sure.
    Maybe,
}

/// Normalize `s` to `form`. The input is borrowed when it is already normalized.
pub fn normalize(s: &str, form: Form) -> Result<Cow<'_, str>> {
    ffi::transform(s, |text, result, diagnostic| unsafe {
        sys::mjb_normalize(
            text,
            sys::MJB_MALFORMED_STOP,
            form.raw(),
            sys::MJB_ENC_UTF_8,
            result,
            diagnostic,
        )
    })
}

/// Run the normalization quick check of `form` on `s`.
pub fn is_normalized(s: &str, form: Form) -> Result<QuickCheck> {
    let text = ffi::text(s);
    let mut answer = sys::MJB_QC_NO;
    // SAFETY: all pointers are valid for the duration of the call.
    check(
        unsafe { sys::mjb_normalization_quick_check(&text, form.raw(), &mut answer) },
        None,
    )?;

    Ok(match answer {
        sys::MJB_QC_YES => QuickCheck::Yes,
        sys::MJB_QC_MAYBE => QuickCheck::Maybe,
        _ => QuickCheck::No,
    })
}

/// Apply the `NFKC_Casefold` transform, used for identifier and caseless comparison.
pub fn nfkc_casefold(s: &str) -> Result<Cow<'_, str>> {
    ffi::transform(s, |text, result, diagnostic| unsafe {
        sys::mjb_nfkc_casefold(
            text,
            sys::MJB_MALFORMED_STOP,
            sys::MJB_ENC_UTF_8,
            result,
            diagnostic,
        )
    })
}
