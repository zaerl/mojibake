// The Mojibake library
//
// This file is distributed under the MIT License. See LICENSE for details.

use std::borrow::Cow;

use mojibake_sys as sys;

use crate::error::{check, Result};
use crate::ffi;

/// A casing locale, parsed from a BCP 47 language tag.
///
/// Locales change casing only where Unicode's `SpecialCasing.txt` says so (for example Turkish,
/// Azerbaijani, and Lithuanian). Every other valid tag behaves like [`Locale::ROOT`].
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash, Default)]
pub struct Locale(sys::mjb_locale);

impl Locale {
    /// Locale-independent casing.
    pub const ROOT: Self = Self(sys::MJB_LOCALE_NOT_SET);

    /// Parse a BCP 47 language tag such as `"tr"` or `"sr-Latn-RS"`.
    pub fn parse(tag: &str) -> Result<Self> {
        let text = ffi::text(tag);
        // SAFETY: mjb_locale_id is plain data; all-zero is a valid value.
        let mut id: sys::mjb_locale_id = unsafe { std::mem::zeroed() };
        // SAFETY: all pointers are valid for the duration of the call.
        check(unsafe { sys::mjb_locale_parse(&text, &mut id) }, None)?;

        Ok(Self(id.locale))
    }

    /// The ISO 639-1 code of the matched locale, or `None` for [`Locale::ROOT`].
    pub fn code(self) -> Option<&'static str> {
        // SAFETY: mjb_locale_code returns NULL or a static string.
        unsafe { ffi::static_str(sys::mjb_locale_code(self.0)) }.filter(|code| !code.is_empty())
    }
}

fn map_case(s: &str, kind: sys::mjb_map_case_type, locale: Locale) -> Result<Cow<'_, str>> {
    ffi::transform(s, |text, result, diagnostic| unsafe {
        sys::mjb_map_case(
            text,
            sys::MJB_MALFORMED_STOP,
            kind,
            locale.0,
            sys::MJB_ENC_UTF_8,
            result,
            diagnostic,
        )
    })
}

/// Full uppercase mapping, with locale tailoring.
pub fn to_uppercase(s: &str, locale: Locale) -> Result<Cow<'_, str>> {
    map_case(s, sys::MJB_CASE_UPPER, locale)
}

/// Full lowercase mapping, with locale tailoring and the Greek final sigma rule.
pub fn to_lowercase(s: &str, locale: Locale) -> Result<Cow<'_, str>> {
    map_case(s, sys::MJB_CASE_LOWER, locale)
}

/// Titlecase each word (UAX #29 word boundaries), with locale tailoring.
pub fn to_titlecase(s: &str, locale: Locale) -> Result<Cow<'_, str>> {
    map_case(s, sys::MJB_CASE_TITLE, locale)
}

/// Full case folding (`CaseFolding.txt` statuses C and F). The result may be longer.
pub fn casefold(s: &str) -> Result<Cow<'_, str>> {
    map_case(s, sys::MJB_CASE_CASEFOLD, Locale::ROOT)
}

/// Simple case folding (statuses C and S). Each character folds to one character.
pub fn casefold_simple(s: &str) -> Result<Cow<'_, str>> {
    map_case(s, sys::MJB_CASE_CASEFOLD_SIMPLE, Locale::ROOT)
}

/// A caseless matching relation from the Unicode Core Specification (D144-D147).
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash, Default)]
pub enum CaselessMode {
    /// D145, canonical caseless match: `NFD(toCasefold(NFD(X)))`.
    #[default]
    Canonical,
    /// D144, default caseless match: `toCasefold(X)`.
    Unnormalized,
    /// D146, compatibility caseless match.
    Compatibility,
    /// D147, identifier caseless match: `toNFKC_Casefold(NFD(X))`.
    Identifier,
}

impl CaselessMode {
    fn raw(self) -> sys::mjb_caseless_mode {
        match self {
            Self::Canonical => sys::MJB_CASELESS_CANONICAL,
            Self::Unnormalized => sys::MJB_CASELESS_UNNORMALIZED,
            Self::Compatibility => sys::MJB_CASELESS_COMPATIBILITY,
            Self::Identifier => sys::MJB_CASELESS_IDENTIFIER,
        }
    }
}

/// Return true if `a` and `b` match under the caseless relation `mode`.
pub fn caseless_eq(a: &str, b: &str, mode: CaselessMode) -> bool {
    let first = ffi::text(a);
    let second = ffi::text(b);
    let mut matches = false;

    // SAFETY: all pointers are valid for the duration of the call; NULL diagnostics are allowed.
    let status = unsafe {
        sys::mjb_caseless_match(
            &first,
            &second,
            sys::MJB_MALFORMED_REPLACE,
            mode.raw(),
            &mut matches,
            std::ptr::null_mut(),
            std::ptr::null_mut(),
        )
    };

    status == sys::MJB_STATUS_OK && matches
}
