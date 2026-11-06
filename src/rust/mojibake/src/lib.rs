// The Mojibake library
//
// This file is distributed under the MIT License. See LICENSE for details.

//! Safe Rust API for [Mojibake](https://github.com/zaerl/mojibake), a small Unicode library.
//!
//! It covers what the Rust standard library leaves out: normalization, locale-aware casing and
//! caseless matching, UAX #29 and UAX #14 segmentation, terminal width, character properties and
//! names, encoding conversion with malformed-input policies, the bidirectional algorithm, DUCET
//! collation, UTS #46 IDNA, UTS #39 confusables and script sets, text filters, UAX #31
//! identifiers, and UTS #51 emoji sequences.
//!
//! ```
//! use mojibake::{Form, StrExt};
//!
//! assert_eq!("e\u{301}".normalize(Form::Nfc).unwrap(), "\u{e9}");
//! assert_eq!("👨‍👩‍👧é".graphemes().count(), 2);
//! assert_eq!("日本".terminal_width(), 4);
//! ```
//!
//! Most string APIs return [`Cow<str>`](std::borrow::Cow): when the C library reports that the
//! input is already in the requested form, the input is borrowed and nothing is allocated.

mod case;
mod character;
mod error;
mod ffi;
mod filter;
mod identifier;
mod normalize;
mod property;
mod segment;
mod str_ext;
mod width;

pub mod bidi;
pub mod emoji;
pub mod encoding;

#[cfg(feature = "collation")]
pub mod collate;
#[cfg(feature = "idna")]
pub mod idna;
#[cfg(feature = "security")]
pub mod security;

pub use bidi::BidiClass;
pub use case::{casefold, casefold_simple, caseless_eq, to_lowercase, to_titlecase, to_uppercase};
pub use case::{CaselessMode, Locale};
pub use character::{
    CharExt, CharInfo, DecompositionType, EastAsianWidth, EmojiProperties, GeneralCategory,
};
pub use error::{Diagnostic, Error, Result};
pub use filter::{filter, Filter};
pub use identifier::{is_identifier, IdentifierProfile};
pub use normalize::{is_normalized, nfkc_casefold, normalize, Form, QuickCheck};
pub use property::Property;
pub use segment::{
    grapheme_count, graphemes, line_breaks, sentence_count, sentences, word_count, words,
    LineBreak, LineBreaks, Segments,
};
pub use str_ext::StrExt;
pub use width::{terminal_width, truncate_to_width, WidthProfile};

/// The Mojibake library version, such as `"0.5.1"`.
pub fn version() -> &'static str {
    // SAFETY: mjb_version returns a static NUL-terminated ASCII string.
    unsafe { ffi::static_str(mojibake_sys::mjb_version()) }.unwrap_or("")
}

/// The Unicode version of the library's data, such as `"18.0.0"`.
pub fn unicode_version() -> &'static str {
    // SAFETY: mjb_unicode_version returns a static NUL-terminated ASCII string.
    unsafe { ffi::static_str(mojibake_sys::mjb_unicode_version()) }.unwrap_or("")
}
