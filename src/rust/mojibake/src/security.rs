// The Mojibake library
//
// This file is distributed under the MIT License. See LICENSE for details.

//! UTS #39 confusable detection.

use mojibake_sys as sys;

use crate::error::{check, Result};
use crate::ffi::{self, Output};

/// The confusable skeleton of `s` (UTS #39 section 4). Two strings whose skeletons are equal are
/// visually confusable.
pub fn skeleton(s: &str) -> Result<String> {
    let text = ffi::text(s);
    let mut output = Output::new();

    // SAFETY: all pointers are valid for the duration of the call.
    check(
        unsafe { sys::mjb_confusable_skeleton(&text, sys::MJB_ENC_UTF_8, &mut output.0) },
        None,
    )?;

    output.into_string()
}

/// True if `a` and `b` are visually confusable, such as `"paypal"` and `"pаypal"` (Cyrillic а).
pub fn is_confusable(a: &str, b: &str) -> bool {
    let first = ffi::text(a);
    let second = ffi::text(b);
    let mut confusable = false;

    // SAFETY: all pointers are valid for the duration of the call.
    let status = unsafe {
        sys::mjb_confusable_match(
            &first,
            &second,
            sys::MJB_MALFORMED_REPLACE,
            &mut confusable,
            std::ptr::null_mut(),
            std::ptr::null_mut(),
        )
    };

    status == sys::MJB_STATUS_OK && confusable
}
