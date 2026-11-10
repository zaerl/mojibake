// The Mojibake library
//
// This file is distributed under the MIT License. See LICENSE for details.

//! UTS #51 emoji sequences.
//!
//! Per-character emoji properties are in [`CharExt::emoji_properties`](crate::CharExt).

use mojibake_sys as sys;

use crate::ffi;

/// The kind of an emoji sequence listed by Unicode.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub enum SequenceKind {
    /// A single emoji character, such as `"😀"`.
    Basic,
    /// A keycap sequence, such as `"1️⃣"`.
    Keycap,
    /// A flag made of two regional indicators, such as `"🇮🇹"`.
    Flag,
    /// A tag sequence, such as the flag of Scotland.
    Tag,
    /// An emoji with a skin-tone modifier, such as `"👍🏽"`.
    Modifier,
    /// A ZWJ sequence, such as `"👨‍👩‍👧"`.
    Zwj,
    /// A standardized variation sequence that requests text presentation (U+FE0E).
    TextVariation,
    /// A standardized variation sequence that requests emoji presentation (U+FE0F).
    EmojiVariation,
}

impl SequenceKind {
    fn from_raw(raw: sys::mjb_emoji_sequence_type) -> Option<Self> {
        Some(match raw {
            sys::MJB_EMOJI_SEQUENCE_BASIC => Self::Basic,
            sys::MJB_EMOJI_SEQUENCE_KEYCAP => Self::Keycap,
            sys::MJB_EMOJI_SEQUENCE_FLAG => Self::Flag,
            sys::MJB_EMOJI_SEQUENCE_TAG => Self::Tag,
            sys::MJB_EMOJI_SEQUENCE_MODIFIER => Self::Modifier,
            sys::MJB_EMOJI_SEQUENCE_ZWJ => Self::Zwj,
            sys::MJB_EMOJI_SEQUENCE_TEXT_VARIATION => Self::TextVariation,
            sys::MJB_EMOJI_SEQUENCE_EMOJI_VARIATION => Self::EmojiVariation,
            _ => return None,
        })
    }
}

/// The `emoji-test.txt` qualification status of an emoji sequence.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub enum Qualification {
    /// Not listed in `emoji-test.txt`, such as a standardized variation sequence.
    None,
    /// A component, such as a skin-tone modifier on its own.
    Component,
    /// Fully qualified.
    FullyQualified,
    /// Minimally qualified: the first character has emoji presentation, but a later one lacks
    /// its U+FE0F.
    MinimallyQualified,
    /// Unqualified: missing the U+FE0F that would give it emoji presentation.
    Unqualified,
}

impl Qualification {
    fn from_raw(raw: sys::mjb_emoji_qualification) -> Self {
        match raw {
            sys::MJB_EMOJI_QUALIFICATION_COMPONENT => Self::Component,
            sys::MJB_EMOJI_QUALIFICATION_FULLY_QUALIFIED => Self::FullyQualified,
            sys::MJB_EMOJI_QUALIFICATION_MINIMALLY_QUALIFIED => Self::MinimallyQualified,
            sys::MJB_EMOJI_QUALIFICATION_UNQUALIFIED => Self::Unqualified,
            _ => Self::None,
        }
    }
}

/// Metadata of a complete emoji sequence.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub struct Sequence {
    /// The kind of sequence, or `None` for a sequence that only `emoji-test.txt` lists, such as
    /// the unqualified `"☺"` (U+263A without U+FE0F).
    pub kind: Option<SequenceKind>,
    /// Its `emoji-test.txt` qualification status.
    pub qualification: Qualification,
    /// The number of characters in the sequence.
    pub codepoint_count: usize,
}

/// Describe `s` if the complete string is one emoji sequence listed by Unicode in
/// `emoji-sequences.txt`, `emoji-zwj-sequences.txt`, `emoji-variation-sequences.txt`, or
/// `emoji-test.txt`.
///
/// ```
/// use mojibake::emoji::{self, Qualification, SequenceKind};
///
/// let flag = emoji::sequence("🇮🇹").unwrap();
/// assert_eq!(flag.kind, Some(SequenceKind::Flag));
/// assert_eq!(flag.qualification, Qualification::FullyQualified);
/// assert!(emoji::sequence("ab").is_none());
/// ```
#[must_use]
pub fn sequence(s: &str) -> Option<Sequence> {
    let text = ffi::text(s);
    let mut emoji = sys::mjb_emoji_sequence::default();

    // SAFETY: all pointers are valid for the duration of the call.
    let status = unsafe { sys::mjb_emoji_sequence_info(&text, &mut emoji) };

    if status != sys::MJB_STATUS_OK {
        return None;
    }

    Some(Sequence {
        kind: SequenceKind::from_raw(emoji.type_),
        qualification: Qualification::from_raw(emoji.qualification),
        codepoint_count: emoji.codepoint_count,
    })
}

/// True if the complete string is one emoji sequence listed by Unicode, including standardized
/// emoji variation sequences.
#[must_use]
pub fn is_sequence(s: &str) -> bool {
    let text = ffi::text(s);

    // SAFETY: the text is valid for the duration of the call.
    unsafe { sys::mjb_is_emoji_sequence(&text) }
}

/// True if the complete string is a recommended-for-general-interchange (RGI) emoji, such as
/// `"🇮🇹"` or `"👨‍👩‍👧"`. Plain standardized variation sequences are not RGI.
#[must_use]
pub fn is_rgi(s: &str) -> bool {
    let text = ffi::text(s);

    // SAFETY: the text is valid for the duration of the call.
    unsafe { sys::mjb_is_rgi_emoji(&text) }
}
