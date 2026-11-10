// The Mojibake library
//
// This file is distributed under the MIT License. See LICENSE for details.

use std::fmt;

use mojibake_sys as sys;

/// Location of the first malformed subsequence that an operation found.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct Diagnostic {
    /// Byte offset of the malformed subsequence.
    pub byte_offset: usize,
    /// Byte length of the malformed subsequence.
    pub byte_length: usize,
}

impl From<sys::mjb_diagnostic> for Diagnostic {
    fn from(value: sys::mjb_diagnostic) -> Self {
        Self {
            byte_offset: value.byte_offset,
            byte_length: value.byte_length,
        }
    }
}

/// An error reported by the Mojibake C library.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
#[non_exhaustive]
pub enum Error {
    /// An argument is invalid or inconsistent with the operation, such as a malformed language
    /// tag or a range outside a bidi paragraph.
    InvalidArgument,
    /// The encoding is invalid or lacks byte-order information, such as [`Encoding::Utf16`]
    /// as an output encoding, or UTF-16 input without a byte order mark.
    ///
    /// [`Encoding::Utf16`]: crate::encoding::Encoding::Utf16
    InvalidEncoding,
    /// A value is not a Unicode scalar value.
    InvalidCodepoint,
    /// The normalization form is invalid, as when [`Form`](crate::Form) parses an unknown name.
    InvalidForm,
    /// The operation, conversion, or value is not supported.
    Unsupported,
    /// The C library could not allocate memory.
    NoMemory,
    /// A size exceeds the range the library supports.
    Overflow,
    /// Ill-formed input, with its location when the library reported one.
    MalformedInput(Option<Diagnostic>),
    /// A caller-provided buffer is too small for the complete result.
    OutputTooSmall,
    /// No Unicode data exists for the value, as when a script code or a category name is
    /// unknown.
    NotFound,
    /// The operation needs a Cargo feature that was turned off, such as `collation`.
    FeatureNotEnabled,
    /// A status this version of the bindings does not know.
    Other(i32),
}

/// A `Result` whose error is [`Error`].
pub type Result<T> = std::result::Result<T, Error>;

impl Error {
    pub(crate) fn from_status(status: sys::mjb_status) -> Self {
        match status {
            sys::MJB_STATUS_INVALID_ARGUMENT => Self::InvalidArgument,
            sys::MJB_STATUS_INVALID_ENCODING => Self::InvalidEncoding,
            sys::MJB_STATUS_INVALID_CODEPOINT => Self::InvalidCodepoint,
            sys::MJB_STATUS_INVALID_FORM => Self::InvalidForm,
            sys::MJB_STATUS_UNSUPPORTED => Self::Unsupported,
            sys::MJB_STATUS_NO_MEMORY => Self::NoMemory,
            sys::MJB_STATUS_OVERFLOW => Self::Overflow,
            sys::MJB_STATUS_MALFORMED_INPUT => Self::MalformedInput(None),
            sys::MJB_STATUS_OUTPUT_TOO_SMALL => Self::OutputTooSmall,
            sys::MJB_STATUS_NOT_FOUND => Self::NotFound,
            sys::MJB_STATUS_FEATURE_NOT_ENABLED => Self::FeatureNotEnabled,
            other => Self::Other(other),
        }
    }

    pub(crate) fn status(&self) -> sys::mjb_status {
        match *self {
            Self::InvalidArgument => sys::MJB_STATUS_INVALID_ARGUMENT,
            Self::InvalidEncoding => sys::MJB_STATUS_INVALID_ENCODING,
            Self::InvalidCodepoint => sys::MJB_STATUS_INVALID_CODEPOINT,
            Self::InvalidForm => sys::MJB_STATUS_INVALID_FORM,
            Self::Unsupported => sys::MJB_STATUS_UNSUPPORTED,
            Self::NoMemory => sys::MJB_STATUS_NO_MEMORY,
            Self::Overflow => sys::MJB_STATUS_OVERFLOW,
            Self::MalformedInput(_) => sys::MJB_STATUS_MALFORMED_INPUT,
            Self::OutputTooSmall => sys::MJB_STATUS_OUTPUT_TOO_SMALL,
            Self::NotFound => sys::MJB_STATUS_NOT_FOUND,
            Self::FeatureNotEnabled => sys::MJB_STATUS_FEATURE_NOT_ENABLED,
            Self::Other(status) => status,
        }
    }
}

/// Turn a status into a `Result`, attaching the diagnostic to malformed-input errors.
pub(crate) fn check(
    status: sys::mjb_status,
    diagnostic: Option<&sys::mjb_diagnostic>,
) -> Result<()> {
    match status {
        sys::MJB_STATUS_OK => Ok(()),
        sys::MJB_STATUS_MALFORMED_INPUT => Err(Error::MalformedInput(
            diagnostic.map(|d| Diagnostic::from(*d)),
        )),
        other => Err(Error::from_status(other)),
    }
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        // SAFETY: mjb_status_message returns a static NUL-terminated UTF-8 string.
        let message = unsafe { crate::ffi::static_str(sys::mjb_status_message(self.status())) };
        f.write_str(message.unwrap_or("unknown status"))?;

        if let Self::MalformedInput(Some(diagnostic)) = self {
            write!(f, " at byte {}", diagnostic.byte_offset)?;
        }

        Ok(())
    }
}

impl std::error::Error for Error {}
