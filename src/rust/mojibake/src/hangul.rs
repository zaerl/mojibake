// The Mojibake library
//
// This file is distributed under the MIT License. See LICENSE for details.

//! Hangul syllables and conjoining jamo (Unicode section 3.12).
//!
//! A precomposed syllable is a leading consonant (L), a vowel (V), and an optional trailing
//! consonant (T). The jamo predicates cover only the 19 L, 21 V, and 27 T jamo that take part in
//! this arithmetic composition: U+1100-U+1112, U+1161-U+1175, and U+11A8-U+11C2.
//!
//! ```
//! use mojibake::hangul;
//!
//! assert_eq!(hangul::decompose('한'), Some(('ᄒ', 'ᅡ', Some('ᆫ'))));
//! assert_eq!(hangul::compose('ᄒ', 'ᅡ', Some('ᆫ')), Some('한'));
//! assert_eq!(hangul::syllable_name('한').as_deref(), Some("HANGUL SYLLABLE HAN"));
//! ```

use std::ffi::c_char;

use mojibake_sys as sys;

use crate::ffi;

/// Compose a syllable from a leading consonant, a vowel, and an optional trailing consonant.
/// `None` if a jamo is outside its range.
pub fn compose(leading: char, vowel: char, trailing: Option<char>) -> Option<char> {
    let mut syllable = 0;
    let trailing = trailing.map_or(0, u32::from);

    // SAFETY: the pointer is valid for the duration of the call.
    let status = unsafe {
        sys::mjb_hangul_syllable_compose(leading as u32, vowel as u32, trailing, &mut syllable)
    };

    if status != sys::MJB_STATUS_OK {
        return None;
    }

    char::from_u32(syllable)
}

/// Split a precomposed syllable into its leading consonant, vowel, and trailing consonant.
/// `None` if `syllable` is not one of the 11,172 precomposed syllables.
#[must_use]
pub fn decompose(syllable: char) -> Option<(char, char, Option<char>)> {
    let mut jamo = [0; 3];

    // SAFETY: `jamo` has room for the three codepoints the call writes.
    let status = unsafe { sys::mjb_hangul_syllable_decompose(syllable as u32, jamo.as_mut_ptr()) };

    if status != sys::MJB_STATUS_OK {
        return None;
    }

    let trailing = if jamo[2] == 0 {
        None
    } else {
        Some(char::from_u32(jamo[2])?)
    };

    Some((char::from_u32(jamo[0])?, char::from_u32(jamo[1])?, trailing))
}

/// The name of a precomposed syllable, such as `"HANGUL SYLLABLE GA"`. It is derived
/// arithmetically, so it is available without the `names` feature.
#[must_use]
pub fn syllable_name(syllable: char) -> Option<String> {
    // "HANGUL SYLLABLE " and at most 2 + 3 + 2 letters of jamo short names.
    let mut buffer = [0 as c_char; 32];

    // SAFETY: the buffer is valid for its whole length, which is passed with it.
    let status = unsafe {
        sys::mjb_hangul_syllable_name(syllable as u32, buffer.as_mut_ptr(), buffer.len())
    };

    (status == sys::MJB_STATUS_OK).then(|| ffi::array_str(&buffer))
}

/// True for one of the 11,172 precomposed syllables, U+AC00 to U+D7A3.
#[must_use]
pub fn is_syllable(c: char) -> bool {
    // SAFETY: a pure function of its argument.
    unsafe { sys::mjb_codepoint_is_hangul_syllable(c as u32) }
}

/// True for a leading, vowel, or trailing conjoining jamo.
#[must_use]
pub fn is_jamo(c: char) -> bool {
    // SAFETY: a pure function of its argument.
    unsafe { sys::mjb_codepoint_is_hangul_jamo(c as u32) }
}

/// True for a leading consonant jamo (L), U+1100 to U+1112.
#[must_use]
pub fn is_leading_jamo(c: char) -> bool {
    // SAFETY: a pure function of its argument.
    unsafe { sys::mjb_codepoint_is_hangul_leading_jamo(c as u32) }
}

/// True for a vowel jamo (V), U+1161 to U+1175.
#[must_use]
pub fn is_vowel_jamo(c: char) -> bool {
    // SAFETY: a pure function of its argument.
    unsafe { sys::mjb_codepoint_is_hangul_vowel_jamo(c as u32) }
}

/// True for a trailing consonant jamo (T), U+11A8 to U+11C2.
#[must_use]
pub fn is_trailing_jamo(c: char) -> bool {
    // SAFETY: a pure function of its argument.
    unsafe { sys::mjb_codepoint_is_hangul_trailing_jamo(c as u32) }
}
