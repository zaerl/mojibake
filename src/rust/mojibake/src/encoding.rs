// The Mojibake library
//
// This file is distributed under the MIT License. See LICENSE for details.

//! Validation, decoding, and conversion of UTF-8, UTF-16, and UTF-32 byte sequences.
//!
//! The Rust standard library decodes UTF-8 and, through `u16` slices, UTF-16. This module works
//! on raw bytes in any byte order, converts between every Unicode encoding form, and lets the
//! caller choose what happens to ill-formed input with [`Malformed`].
//!
//! ```
//! use mojibake::encoding::{self, Encoding, Malformed};
//!
//! let utf16 = encoding::encode("héllo", Encoding::Utf16Le)?;
//! assert_eq!(encoding::decode(&utf16, Encoding::Utf16Le, Malformed::Stop)?, "héllo");
//! assert_eq!(encoding::decode(b"a\xffb", Encoding::Utf8, Malformed::Replace)?, "a\u{fffd}b");
//! # Ok::<(), mojibake::Error>(())
//! ```

use mojibake_sys as sys;

use crate::error::{check, Result};
use crate::ffi::Output;

/// A Unicode encoding form and byte order.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub enum Encoding {
    /// UTF-8. A leading byte order mark is kept as U+FEFF.
    Utf8,
    /// UTF-16 whose byte order is given by a leading byte order mark, which is consumed. Input
    /// only: the input must start with a BOM, and [`encode`] rejects it.
    Utf16,
    /// UTF-16, little-endian. A leading byte order mark is kept as U+FEFF.
    Utf16Le,
    /// UTF-16, big-endian. A leading byte order mark is kept as U+FEFF.
    Utf16Be,
    /// UTF-32 whose byte order is given by a leading byte order mark, which is consumed. Input
    /// only: the input must start with a BOM, and [`encode`] rejects it.
    Utf32,
    /// UTF-32, little-endian. A leading byte order mark is kept as U+FEFF.
    Utf32Le,
    /// UTF-32, big-endian. A leading byte order mark is kept as U+FEFF.
    Utf32Be,
}

impl Encoding {
    pub(crate) fn raw(self) -> sys::mjb_encoding {
        match self {
            Self::Utf8 => sys::MJB_ENC_UTF_8,
            Self::Utf16 => sys::MJB_ENC_UTF_16,
            Self::Utf16Le => sys::MJB_ENC_UTF_16LE,
            Self::Utf16Be => sys::MJB_ENC_UTF_16BE,
            Self::Utf32 => sys::MJB_ENC_UTF_32,
            Self::Utf32Le => sys::MJB_ENC_UTF_32LE,
            Self::Utf32Be => sys::MJB_ENC_UTF_32BE,
        }
    }
}

/// What to do with an ill-formed code-unit sequence.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq, Hash)]
pub enum Malformed {
    /// Fail with [`Error::MalformedInput`](crate::Error::MalformedInput), whose
    /// [`Diagnostic`](crate::Diagnostic) gives the byte offset of the first ill-formed
    /// subsequence.
    #[default]
    Stop,
    /// Replace each maximal ill-formed subsequence with U+FFFD REPLACEMENT CHARACTER.
    Replace,
    /// Drop each ill-formed subsequence.
    Skip,
}

impl Malformed {
    pub(crate) fn raw(self) -> sys::mjb_malformed_policy {
        match self {
            Self::Stop => sys::MJB_MALFORMED_STOP,
            Self::Replace => sys::MJB_MALFORMED_REPLACE,
            Self::Skip => sys::MJB_MALFORMED_SKIP,
        }
    }
}

fn text(bytes: &[u8], encoding: Encoding) -> sys::mjb_text {
    sys::mjb_text {
        buffer: bytes.as_ptr().cast(),
        byte_length: bytes.len(),
        encoding: encoding.raw(),
    }
}

/// Convert `bytes` from encoding `from` to encoding `to`.
///
/// # Errors
///
/// [`Error::MalformedInput`](crate::Error::MalformedInput) with the location of the first
/// ill-formed subsequence when `malformed` is [`Malformed::Stop`], and
/// [`Error::InvalidEncoding`](crate::Error::InvalidEncoding) when `to` is [`Encoding::Utf16`] or
/// [`Encoding::Utf32`], or when `from` is one of them and `bytes` has no byte order mark.
pub fn convert(
    bytes: &[u8],
    from: Encoding,
    to: Encoding,
    malformed: Malformed,
) -> Result<Vec<u8>> {
    let text = text(bytes, from);
    let mut output = Output::new();
    let mut diagnostic = sys::mjb_diagnostic::default();
    // SAFETY: all pointers are valid for the duration of the call, and `text` borrows `bytes`.
    let status = unsafe {
        sys::mjb_convert_encoding(
            &text,
            malformed.raw(),
            to.raw(),
            &mut output.0,
            &mut diagnostic,
        )
    };

    check(status, Some(&diagnostic))?;
    Ok(output.into_vec())
}

/// Decode `bytes` in `encoding` into a `String`.
///
/// # Errors
///
/// See [`convert`].
pub fn decode(bytes: &[u8], encoding: Encoding, malformed: Malformed) -> Result<String> {
    let utf8 = convert(bytes, encoding, Encoding::Utf8, malformed)?;

    // The library only emits well-formed UTF-8, so this never fails in practice.
    String::from_utf8(utf8).map_err(|_| crate::Error::InvalidEncoding)
}

/// Encode `s` in `encoding`. No byte order mark is added.
///
/// # Errors
///
/// [`Error::InvalidEncoding`](crate::Error::InvalidEncoding) when `encoding` is
/// [`Encoding::Utf16`] or [`Encoding::Utf32`], which do not name a byte order.
pub fn encode(s: &str, encoding: Encoding) -> Result<Vec<u8>> {
    convert(s.as_bytes(), Encoding::Utf8, encoding, Malformed::Stop)
}

/// Check that `bytes` is well-formed in `encoding`.
///
/// # Errors
///
/// [`Error::MalformedInput`](crate::Error::MalformedInput) with the location of the first
/// ill-formed subsequence.
pub fn validate(bytes: &[u8], encoding: Encoding) -> Result<()> {
    let text = text(bytes, encoding);
    let mut diagnostic = sys::mjb_diagnostic::default();
    // SAFETY: all pointers are valid for the duration of the call.
    let status = unsafe { sys::mjb_validate_encoding(&text, &mut diagnostic) };

    check(status, Some(&diagnostic))
}

/// Count the codepoints of `bytes` in `encoding`. A consumed byte order mark is not counted.
///
/// With [`Malformed::Replace`] each ill-formed subsequence counts as one codepoint; with
/// [`Malformed::Skip`] it does not count.
///
/// # Errors
///
/// See [`convert`].
pub fn codepoint_count(bytes: &[u8], encoding: Encoding, malformed: Malformed) -> Result<usize> {
    let text = text(bytes, encoding);
    let mut count = 0;
    let mut diagnostic = sys::mjb_diagnostic::default();
    // SAFETY: all pointers are valid for the duration of the call.
    let status =
        unsafe { sys::mjb_codepoint_count(&text, malformed.raw(), &mut count, &mut diagnostic) };

    check(status, Some(&diagnostic))?;
    Ok(count)
}

/// Guess the encoding of `bytes`.
///
/// A byte order mark wins: a UTF-16 or UTF-32 BOM gives [`Encoding::Utf16`] or
/// [`Encoding::Utf32`], so that decoding with the answer consumes the BOM. Note that a UTF-32LE
/// BOM is also a UTF-16LE BOM followed by U+0000; this reports UTF-32. Without a BOM, well-formed
/// UTF-8 gives [`Encoding::Utf8`]. Anything else, and empty input, gives `None`.
pub fn detect(bytes: &[u8]) -> Option<Encoding> {
    if bytes.is_empty() {
        return None;
    }

    // SAFETY: `bytes` is valid for `bytes.len()` bytes, and its length is never
    // MJB_NUL_TERMINATED because a slice cannot span the whole address space.
    let detected = unsafe { sys::mjb_detect_encoding(bytes.as_ptr().cast(), bytes.len()) };

    if detected & sys::MJB_ENC_UTF_32 != 0 {
        Some(Encoding::Utf32)
    } else if detected & sys::MJB_ENC_UTF_16 != 0 {
        Some(Encoding::Utf16)
    } else if detected & (sys::MJB_ENC_UTF_8 | sys::MJB_ENC_ASCII) != 0 {
        Some(Encoding::Utf8)
    } else {
        None
    }
}
