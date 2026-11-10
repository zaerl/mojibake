// The Mojibake library
//
// This file is distributed under the MIT License. See LICENSE for details.

use std::borrow::Cow;
use std::fmt;
use std::str::FromStr;

use mojibake_sys as sys;

use crate::error::{check, Error, Result};
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

    /// Run the normalization quick check of this form on `s` (UAX #15 section 9).
    ///
    /// It is faster than [`is_normalized`], but can answer [`QuickCheck::Maybe`]. Use
    /// [`is_normalized`] for a definite answer.
    ///
    /// ```
    /// use mojibake::{Form, QuickCheck};
    ///
    /// assert_eq!(Form::Nfc.quick_check("abc")?, QuickCheck::Yes);
    /// assert_eq!(Form::Nfd.quick_check("\u{e9}")?, QuickCheck::No);
    /// assert_eq!(Form::Nfc.quick_check("e\u{301}")?, QuickCheck::Maybe);
    /// # Ok::<(), mojibake::Error>(())
    /// ```
    ///
    /// # Errors
    ///
    /// [`Error::InvalidArgument`](crate::Error::InvalidArgument) only on an internal error;
    /// a valid Rust string always gets an answer.
    pub fn quick_check(self, s: &str) -> Result<QuickCheck> {
        let text = ffi::text(s);
        let mut answer = sys::MJB_QC_NO;
        // SAFETY: all pointers are valid for the duration of the call.
        check(
            unsafe { sys::mjb_normalization_quick_check(&text, self.raw(), &mut answer) },
            None,
        )?;

        Ok(match answer {
            sys::MJB_QC_YES => QuickCheck::Yes,
            sys::MJB_QC_MAYBE => QuickCheck::Maybe,
            _ => QuickCheck::No,
        })
    }
}

impl Form {
    fn name(self) -> &'static str {
        match self {
            Self::Nfc => "NFC",
            Self::Nfd => "NFD",
            Self::Nfkc => "NFKC",
            Self::Nfkd => "NFKD",
        }
    }
}

impl fmt::Display for Form {
    /// Writes the form name, such as `NFC`.
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str(self.name())
    }
}

impl FromStr for Form {
    type Err = Error;

    /// Parse a form name such as `"NFC"`, ignoring ASCII case. Returns [`Error::InvalidForm`] for
    /// any other string.
    fn from_str(s: &str) -> Result<Self> {
        [Self::Nfc, Self::Nfd, Self::Nfkc, Self::Nfkd]
            .into_iter()
            .find(|form| form.name().eq_ignore_ascii_case(s))
            .ok_or(Error::InvalidForm)
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
///
/// # Errors
///
/// [`Error::NoMemory`](crate::Error::NoMemory) if the library cannot allocate memory.
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

/// Return true if `s` is in normalization form `form`.
///
/// It runs the quick check first and normalizes only when the quick check cannot decide.
///
/// ```
/// use mojibake::{is_normalized, Form};
///
/// assert!(is_normalized("\u{e9}", Form::Nfc)?);
/// assert!(!is_normalized("e\u{301}", Form::Nfc)?);
/// # Ok::<(), mojibake::Error>(())
/// ```
///
/// # Errors
///
/// [`Error::NoMemory`](crate::Error::NoMemory) if the library cannot allocate memory.
pub fn is_normalized(s: &str, form: Form) -> Result<bool> {
    match form.quick_check(s)? {
        QuickCheck::Yes => Ok(true),
        QuickCheck::No => Ok(false),
        QuickCheck::Maybe => Ok(normalize(s, form)? == s),
    }
}

/// Apply the `NFKC_Casefold` transform, used for identifier and caseless comparison.
///
/// # Errors
///
/// [`Error::NoMemory`](crate::Error::NoMemory) if the library cannot allocate memory.
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
