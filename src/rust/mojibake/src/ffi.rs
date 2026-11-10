// The Mojibake library
//
// This file is distributed under the MIT License. See LICENSE for details.

//! Shared glue between Rust strings and the C API.

use std::borrow::Cow;
use std::ffi::{c_char, CStr};

use mojibake_sys as sys;

use crate::error::{check, Error, Result};
use crate::script::Script;

/// View a UTF-8 Rust string as an `mjb_text`. The text borrows `s`.
pub(crate) fn text(s: &str) -> sys::mjb_text {
    sys::mjb_text {
        buffer: s.as_ptr().cast(),
        byte_length: s.len(),
        encoding: sys::MJB_ENC_UTF_8,
    }
}

/// Convert a static NUL-terminated string returned by the library.
///
/// # Safety
///
/// `ptr` must be NULL or point to a NUL-terminated string that lives for the whole program.
pub(crate) unsafe fn static_str(ptr: *const c_char) -> Option<&'static str> {
    if ptr.is_null() {
        return None;
    }

    CStr::from_ptr(ptr).to_str().ok()
}

/// Panic if a call that cannot fail on valid input reported an error.
///
/// The library fails on a valid Rust string only when it cannot allocate memory or a size
/// overflows. Rust treats allocation failure as fatal, so the infallible wrappers panic instead of
/// returning a wrong answer. The ones that can allocate have a `try_` variant.
#[track_caller]
pub(crate) fn expect_ok(status: sys::mjb_status) {
    if let Err(error) = check(status, None) {
        panic!("mojibake: {error}");
    }
}

/// Run a `(scripts, count)` query twice: once with NULL to size the buffer, then to fill it.
pub(crate) fn scripts(
    mut call: impl FnMut(*mut sys::mjb_script, &mut usize) -> sys::mjb_status,
) -> Result<Vec<Script>> {
    let mut count = 0;
    check(call(std::ptr::null_mut(), &mut count), None)?;

    let mut raw: Vec<sys::mjb_script> = vec![0; count];

    if count != 0 {
        check(call(raw.as_mut_ptr(), &mut count), None)?;
        raw.truncate(count);
    }

    Ok(raw.into_iter().filter_map(Script::from_raw).collect())
}

/// Read a NUL-terminated fixed-size C array, stopping at its end if no NUL is present.
pub(crate) fn array_str(array: &[c_char]) -> String {
    let bytes: Vec<u8> = array
        .iter()
        .take_while(|&&c| c != 0)
        .map(|&c| c as u8)
        .collect();

    String::from_utf8_lossy(&bytes).into_owned()
}

/// An `mjb_result` that is released with `mjb_result_free` when dropped.
pub(crate) struct Output(pub(crate) sys::mjb_result);

impl Output {
    pub(crate) fn new() -> Self {
        Self(sys::mjb_result::default())
    }

    fn bytes(&self) -> &[u8] {
        if self.0.output.is_null() || self.0.output_size == 0 {
            return &[];
        }

        // SAFETY: the library guarantees `output_size` readable bytes at `output`.
        unsafe { std::slice::from_raw_parts(self.0.output.cast(), self.0.output_size) }
    }

    /// The output as UTF-8, borrowing `input` when the library did not allocate.
    pub(crate) fn into_cow(self, input: &str) -> Result<Cow<'_, str>> {
        if !self.0.owned {
            if self.0.output_size == 0 {
                return Ok(Cow::Borrowed(""));
            }

            // A borrowed output points into the input. Locate it instead of assuming offset 0.
            let start = (self.0.output as usize).wrapping_sub(input.as_ptr() as usize);
            let end = start
                .checked_add(self.0.output_size)
                .ok_or(Error::Overflow)?;

            return input
                .get(start..end)
                .map(Cow::Borrowed)
                .ok_or(Error::InvalidArgument);
        }

        self.into_string().map(Cow::Owned)
    }

    pub(crate) fn into_string(self) -> Result<String> {
        String::from_utf8(self.bytes().to_vec()).map_err(|_| Error::InvalidEncoding)
    }

    pub(crate) fn into_vec(self) -> Vec<u8> {
        self.bytes().to_vec()
    }
}

impl Drop for Output {
    fn drop(&mut self) {
        // SAFETY: the result was initialized by Output::new and filled only by the library.
        unsafe {
            sys::mjb_result_free(&mut self.0);
        }
    }
}

/// Run an allocating transform that takes `(text, policy, diagnostic)` around its output.
pub(crate) fn transform<'a>(
    input: &'a str,
    call: impl FnOnce(&sys::mjb_text, &mut sys::mjb_result, &mut sys::mjb_diagnostic) -> sys::mjb_status,
) -> Result<Cow<'a, str>> {
    let text = text(input);
    let mut output = Output::new();
    let mut diagnostic = sys::mjb_diagnostic::default();
    let status = call(&text, &mut output.0, &mut diagnostic);

    check(status, Some(&diagnostic))?;
    output.into_cow(input)
}

/// Run a counting function that takes `(text, policy, count, diagnostic)`.
///
/// # Panics
///
/// See [`expect_ok`].
pub(crate) fn count(
    input: &str,
    call: unsafe extern "C" fn(
        *const sys::mjb_text,
        sys::mjb_malformed_policy,
        *mut usize,
        *mut sys::mjb_diagnostic,
    ) -> sys::mjb_status,
) -> usize {
    let text = text(input);
    let mut count = 0;
    let mut diagnostic = sys::mjb_diagnostic::default();

    // SAFETY: all pointers are valid for the duration of the call. A Rust &str is valid UTF-8,
    // so MJB_MALFORMED_REPLACE never changes the result; it only avoids an error path.
    let status = unsafe {
        call(
            &text,
            sys::MJB_MALFORMED_REPLACE,
            &mut count,
            &mut diagnostic,
        )
    };
    expect_ok(status);

    count
}
