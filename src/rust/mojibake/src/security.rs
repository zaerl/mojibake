// The Mojibake library
//
// This file is distributed under the MIT License. See LICENSE for details.

//! UTS #39 confusable detection.

use mojibake_sys as sys;

use crate::error::{check, Result};
use crate::ffi::{self, Output};
use crate::script::Script;

/// The confusable skeleton of `s` (UTS #39 section 4). Two strings whose skeletons are equal are
/// visually confusable.
///
/// # Errors
///
/// [`Error::NoMemory`](crate::Error::NoMemory) if the library cannot allocate memory.
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

/// The UTS #39 resolved script set of a string. See [`resolved_scripts`].
#[derive(Clone, Debug, PartialEq, Eq, Hash)]
pub enum ScriptSet {
    /// Every script resolves the string: it is empty or holds only `Common` and `Inherited`
    /// characters.
    All,
    /// The scripts that resolve the string. It may include the writing-system values `Hanb`,
    /// `Jpan`, and `Kore`.
    Resolved(Vec<Script>),
    /// No script resolves the string: it is mixed-script.
    Empty,
}

impl ScriptSet {
    /// True if no script resolves the string (UTS #39 section 5.1).
    #[must_use]
    pub fn is_mixed_script(&self) -> bool {
        matches!(self, Self::Empty)
    }
}

/// Intersect the augmented `Script_Extensions` of every character of `s` (UTS #39 section 5.1).
///
/// ```
/// use mojibake::security::{resolved_scripts, ScriptSet};
/// use mojibake::Script;
///
/// let latin = Script::from_code("Latn").unwrap();
/// assert_eq!(resolved_scripts("Circle").unwrap(), ScriptSet::Resolved(vec![latin]));
/// assert!(resolved_scripts("Сirclе").unwrap().is_mixed_script()); // Cyrillic С and е
/// assert_eq!(resolved_scripts("123").unwrap(), ScriptSet::All);
/// ```
///
/// # Errors
///
/// [`Error::NoMemory`](crate::Error::NoMemory) if the library cannot allocate memory.
pub fn resolved_scripts(s: &str) -> Result<ScriptSet> {
    let text = ffi::text(s);
    let mut kind = sys::MJB_SCRIPT_SET_EMPTY;

    let scripts = ffi::scripts(|scripts, count| {
        // SAFETY: all pointers are valid for the duration of the call, and `scripts` is NULL or
        // has room for `count` values.
        unsafe { sys::mjb_resolved_script_set(&text, scripts, count, &mut kind) }
    })?;

    Ok(match kind {
        sys::MJB_SCRIPT_SET_ALL => ScriptSet::All,
        sys::MJB_SCRIPT_SET_RESOLVED => ScriptSet::Resolved(scripts),
        _ => ScriptSet::Empty,
    })
}

/// True if `a` and `b` are visually confusable, such as `"paypal"` and `"pаypal"` (Cyrillic а).
///
/// # Panics
///
/// If the library cannot allocate memory. [`try_is_confusable`] returns the error instead.
#[must_use]
pub fn is_confusable(a: &str, b: &str) -> bool {
    try_is_confusable(a, b).unwrap_or_else(|error| panic!("mojibake: {error}"))
}

/// Like [`is_confusable`], but returns an error instead of panicking.
///
/// # Errors
///
/// [`Error::NoMemory`](crate::Error::NoMemory) if the library cannot allocate memory.
pub fn try_is_confusable(a: &str, b: &str) -> Result<bool> {
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
    check(status, None)?;

    Ok(confusable)
}
