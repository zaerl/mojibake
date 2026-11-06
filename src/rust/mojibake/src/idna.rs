// The Mojibake library
//
// This file is distributed under the MIT License. See LICENSE for details.

//! UTS #46 nontransitional IDNA processing.

use std::fmt;

use mojibake_sys as sys;

use crate::error::{self, Result};
use crate::ffi::{self, Output};

/// UTS #46 processing failed. The best-effort result is kept, but must not be used for DNS.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct IdnaError {
    /// The `mjb_idna_error` flags that were set.
    pub errors: u32,
    /// The best-effort conversion.
    pub output: String,
}

impl fmt::Display for IdnaError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "IDNA processing failed (errors 0x{:x})", self.errors)
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
            errors: info.errors,
            output,
        })
    })
}

/// Convert a domain name to its ASCII form: `"bücher.de"` becomes `"xn--bcher-kva.de"`.
///
/// The outer `Result` reports library errors; the inner one reports UTS #46 processing errors.
pub fn to_ascii(domain: &str) -> Result<IdnaResult> {
    convert(domain, sys::mjb_idna_to_ascii)
}

/// Convert a domain name to its Unicode form: `"xn--bcher-kva.de"` becomes `"bücher.de"`.
pub fn to_unicode(domain: &str) -> Result<IdnaResult> {
    convert(domain, sys::mjb_idna_to_unicode)
}
