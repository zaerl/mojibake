// The Mojibake library
//
// This file is distributed under the MIT License. See LICENSE for details.

//! UTS #46 nontransitional IDNA processing.

use std::fmt;
use std::ops::{BitOr, BitOrAssign};

use mojibake_sys as sys;

use crate::error::{self, Result};
use crate::ffi::{self, Output};

/// The UTS #46 checks that failed during an IDNA conversion. Several can fail at once.
#[derive(Clone, Copy, Default, PartialEq, Eq, Hash)]
pub struct IdnaErrors(sys::mjb_idna_error);

const ERROR_NAMES: [(IdnaErrors, &str); 11] = [
    (IdnaErrors::PUNYCODE, "PUNYCODE"),
    (IdnaErrors::EMPTY_LABEL, "EMPTY_LABEL"),
    (IdnaErrors::HYPHEN, "HYPHEN"),
    (IdnaErrors::NOT_NFC, "NOT_NFC"),
    (IdnaErrors::LEADING_MARK, "LEADING_MARK"),
    (IdnaErrors::DISALLOWED, "DISALLOWED"),
    (IdnaErrors::STD3, "STD3"),
    (IdnaErrors::CONTEXTJ, "CONTEXTJ"),
    (IdnaErrors::BIDI, "BIDI"),
    (IdnaErrors::LABEL_LENGTH, "LABEL_LENGTH"),
    (IdnaErrors::DOMAIN_LENGTH, "DOMAIN_LENGTH"),
];

impl IdnaErrors {
    /// No check failed.
    pub const NONE: Self = Self(sys::MJB_IDNA_ERROR_NONE);
    /// An `xn--` label is not valid Punycode, or does not decode to a valid label.
    pub const PUNYCODE: Self = Self(sys::MJB_IDNA_ERROR_PUNYCODE);
    /// A label is empty, as in `"a..b"`.
    pub const EMPTY_LABEL: Self = Self(sys::MJB_IDNA_ERROR_EMPTY_LABEL);
    /// A label starts or ends with a hyphen, or has hyphens in its third and fourth positions.
    pub const HYPHEN: Self = Self(sys::MJB_IDNA_ERROR_HYPHEN);
    /// A label is not in NFC.
    pub const NOT_NFC: Self = Self(sys::MJB_IDNA_ERROR_NOT_NFC);
    /// A label starts with a combining mark.
    pub const LEADING_MARK: Self = Self(sys::MJB_IDNA_ERROR_LEADING_MARK);
    /// A label contains a disallowed character.
    pub const DISALLOWED: Self = Self(sys::MJB_IDNA_ERROR_DISALLOWED);
    /// A label contains an ASCII character that STD3 rules disallow, such as `_`.
    pub const STD3: Self = Self(sys::MJB_IDNA_ERROR_STD3);
    /// A ZERO WIDTH JOINER or NON-JOINER appears where the CONTEXTJ rules (RFC 5892) forbid it.
    pub const CONTEXTJ: Self = Self(sys::MJB_IDNA_ERROR_CONTEXTJ);
    /// A label of a bidi domain name fails the Bidi Rule (RFC 5893).
    pub const BIDI: Self = Self(sys::MJB_IDNA_ERROR_BIDI);
    /// An ASCII label is empty or longer than 63 bytes (`to_ascii` only).
    pub const LABEL_LENGTH: Self = Self(sys::MJB_IDNA_ERROR_LABEL_LENGTH);
    /// The ASCII domain name is empty or longer than 253 bytes (`to_ascii` only).
    pub const DOMAIN_LENGTH: Self = Self(sys::MJB_IDNA_ERROR_DOMAIN_LENGTH);

    /// True if every check in `other` failed.
    #[must_use]
    pub const fn contains(self, other: Self) -> bool {
        self.0 & other.0 == other.0
    }

    /// True if no check failed.
    #[must_use]
    pub const fn is_empty(self) -> bool {
        self.0 == 0
    }

    /// The raw `mjb_idna_error` bits.
    #[must_use]
    pub const fn bits(self) -> u32 {
        self.0
    }

    /// The failed checks, one flag at a time.
    pub fn iter(self) -> impl Iterator<Item = Self> {
        ERROR_NAMES
            .into_iter()
            .map(|(flag, _)| flag)
            .filter(move |&flag| self.contains(flag))
    }
}

impl BitOr for IdnaErrors {
    type Output = Self;

    fn bitor(self, rhs: Self) -> Self {
        Self(self.0 | rhs.0)
    }
}

impl BitOrAssign for IdnaErrors {
    fn bitor_assign(&mut self, rhs: Self) {
        self.0 |= rhs.0;
    }
}

impl fmt::Debug for IdnaErrors {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str("IdnaErrors(")?;
        let mut first = true;

        for (flag, name) in ERROR_NAMES {
            if self.contains(flag) {
                if !first {
                    f.write_str(" | ")?;
                }

                f.write_str(name)?;
                first = false;
            }
        }

        // Bits from a newer C library that these bindings do not name.
        let known = ERROR_NAMES.iter().fold(0, |bits, (flag, _)| bits | flag.0);
        let unknown = self.0 & !known;

        if unknown != 0 {
            if !first {
                f.write_str(" | ")?;
            }

            write!(f, "0x{unknown:x}")?;
            first = false;
        }

        if first {
            f.write_str("NONE")?;
        }

        f.write_str(")")
    }
}

/// UTS #46 processing failed. The best-effort result is kept, but must not be used for DNS.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct IdnaError {
    /// The checks that failed.
    pub errors: IdnaErrors,
    /// The best-effort conversion.
    pub output: String,
}

impl fmt::Display for IdnaError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "IDNA processing failed: {:?}", self.errors)
    }
}

impl std::error::Error for IdnaError {}

/// The outcome of an IDNA conversion.
pub type IdnaResult = std::result::Result<String, IdnaError>;

type Convert = unsafe extern "C" fn(
    *const sys::mjb_text,
    sys::mjb_encoding,
    *mut sys::mjb_idna_info,
    *mut sys::mjb_result,
) -> sys::mjb_status;

fn convert(domain: &str, call: Convert) -> Result<IdnaResult> {
    let text = ffi::text(domain);
    let mut info = sys::mjb_idna_info::default();
    let mut output = Output::new();

    // SAFETY: all pointers are valid for the duration of the call.
    let status = unsafe { call(&text, sys::MJB_ENC_UTF_8, &mut info, &mut output.0) };
    error::check(status, None)?;
    let output = output.into_string()?;

    Ok(if info.errors == 0 {
        Ok(output)
    } else {
        Err(IdnaError {
            errors: IdnaErrors(info.errors),
            output,
        })
    })
}

/// Convert a domain name to its ASCII form: `"bücher.de"` becomes `"xn--bcher-kva.de"`.
///
/// The outer `Result` reports library errors; the inner one reports UTS #46 processing errors.
///
/// # Errors
///
/// [`Error::NoMemory`](crate::Error::NoMemory) if the library cannot allocate memory. UTS #46
/// processing errors are reported by the inner [`IdnaResult`] instead.
pub fn to_ascii(domain: &str) -> Result<IdnaResult> {
    convert(domain, sys::mjb_idna_to_ascii)
}

/// Convert a domain name to its Unicode form: `"xn--bcher-kva.de"` becomes `"bücher.de"`.
///
/// # Errors
///
/// [`Error::NoMemory`](crate::Error::NoMemory) if the library cannot allocate memory. UTS #46
/// processing errors are reported by the inner [`IdnaResult`] instead.
pub fn to_unicode(domain: &str) -> Result<IdnaResult> {
    convert(domain, sys::mjb_idna_to_unicode)
}
