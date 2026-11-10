// The Mojibake library
//
// This file is distributed under the MIT License. See LICENSE for details.

use mojibake_sys as sys;

use crate::ffi;

/// The UAX #31 identifier profile used by [`is_identifier`].
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq, Hash)]
pub enum IdentifierProfile {
    /// `ID_Start` followed by `ID_Continue`, for NFC text.
    #[default]
    Default,
    /// `XID_Start` followed by `XID_Continue`, which are closed under NFKC.
    Nfkc,
}

/// True if `s` is a UAX #31 identifier: a start character followed by continue characters.
/// The empty string is not an identifier.
///
/// ```
/// use mojibake::{is_identifier, IdentifierProfile};
///
/// assert!(is_identifier("größe_1", IdentifierProfile::Nfkc));
/// assert!(!is_identifier("1st", IdentifierProfile::Nfkc));
/// ```
#[must_use]
pub fn is_identifier(s: &str, profile: IdentifierProfile) -> bool {
    let text = ffi::text(s);
    let profile = match profile {
        IdentifierProfile::Default => sys::MJB_IDENTIFIER_DEFAULT,
        IdentifierProfile::Nfkc => sys::MJB_IDENTIFIER_NFKC,
    };

    // SAFETY: the text is valid for the duration of the call.
    unsafe { sys::mjb_is_identifier(&text, profile) }
}
