// The Mojibake library
//
// This file is distributed under the MIT License. See LICENSE for details.

use std::iter::FusedIterator;

use mojibake_sys as sys;

use crate::ffi;

type BreakNext =
    unsafe extern "C" fn(*const sys::mjb_text, *mut sys::mjb_break_state) -> sys::mjb_break_type;

/// An iterator over the segments of a string, as `&str` slices.
///
/// Created by [`graphemes`], [`words`], and [`sentences`]. Word segmentation also yields the
/// spaces and punctuation between words, as UAX #29 does.
#[derive(Clone)]
#[must_use = "iterators are lazy and do nothing unless consumed"]
pub struct Segments<'a> {
    input: &'a str,
    text: sys::mjb_text,
    state: sys::mjb_break_state,
    next_break: BreakNext,
    start: usize,
    done: bool,
}

impl<'a> Segments<'a> {
    fn new(input: &'a str, next_break: BreakNext) -> Self {
        Self {
            input,
            text: ffi::text(input),
            state: sys::mjb_break_state::default(),
            next_break,
            start: 0,
            done: input.is_empty(),
        }
    }
}

impl<'a> Iterator for Segments<'a> {
    type Item = &'a str;

    fn next(&mut self) -> Option<&'a str> {
        while !self.done {
            // SAFETY: `text` borrows `input`, which outlives the iterator, and `state` is storage
            // that fits every C break state (checked by shim/layout.c).
            let kind = unsafe { (self.next_break)(&self.text, &mut self.state) };

            if kind == sys::MJB_BT_NOT_SET {
                self.done = true;
                break;
            }

            let position = self.state.position.min(self.input.len());

            if kind != sys::MJB_BT_NO_BREAK && position > self.start {
                let segment = &self.input[self.start..position];
                self.start = position;

                return Some(segment);
            }
        }

        // Yield whatever follows the last reported boundary.
        if self.start < self.input.len() {
            let segment = &self.input[self.start..];
            self.start = self.input.len();

            return Some(segment);
        }

        None
    }
}

impl FusedIterator for Segments<'_> {}

// SAFETY: the raw pointer in `text` only borrows the `&'a str` the iterator already holds, and the
// C iterator keeps all its state in `state`.
unsafe impl Send for Segments<'_> {}
unsafe impl Sync for Segments<'_> {}

/// Iterate over the extended grapheme clusters of `s` (UAX #29).
pub fn graphemes(s: &str) -> Segments<'_> {
    Segments::new(s, sys::mjb_grapheme_break_next)
}

/// Iterate over the word-break segments of `s` (UAX #29), including spaces and punctuation.
pub fn words(s: &str) -> Segments<'_> {
    Segments::new(s, sys::mjb_word_break_next)
}

/// Iterate over the sentences of `s` (UAX #29).
pub fn sentences(s: &str) -> Segments<'_> {
    Segments::new(s, sys::mjb_sentence_break_next)
}

/// The number of extended grapheme clusters in `s`.
pub fn grapheme_count(s: &str) -> usize {
    ffi::count(s, sys::mjb_grapheme_count)
}

/// The number of word-like segments in `s` (spaces and punctuation are not counted).
pub fn word_count(s: &str) -> usize {
    ffi::count(s, sys::mjb_word_count)
}

/// The number of sentences in `s`.
pub fn sentence_count(s: &str) -> usize {
    ffi::count(s, sys::mjb_sentence_count)
}

type PrefixLength = unsafe extern "C" fn(*const sys::mjb_text, usize) -> usize;

fn prefix(s: &str, count: usize, call: PrefixLength) -> &str {
    let text = ffi::text(s);
    // SAFETY: the text is valid for the duration of the call.
    let length = unsafe { call(&text, count) };

    // The library returns a segment boundary, which is always a char boundary.
    s.get(..length).unwrap_or(s)
}

/// The first `count` extended grapheme clusters of `s`, or all of `s` if it has fewer. One pass,
/// with no allocation.
///
/// ```
/// assert_eq!(mojibake::grapheme_prefix("e\u{301}🇮🇹abc", 2), "e\u{301}🇮🇹");
/// ```
pub fn grapheme_prefix(s: &str, count: usize) -> &str {
    prefix(s, count, sys::mjb_grapheme_prefix_length)
}

/// The first `count` segments that [`words`] yields, including spaces and punctuation, or all of
/// `s` if it has fewer.
///
/// ```
/// // "Hello", ",", " ", "world"
/// assert_eq!(mojibake::word_prefix("Hello, world!", 4), "Hello, world");
/// ```
pub fn word_prefix(s: &str, count: usize) -> &str {
    prefix(s, count, sys::mjb_word_prefix_length)
}

/// A line-break opportunity (UAX #14).
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub enum LineBreak {
    /// A line must break here, for example after a newline.
    Mandatory(usize),
    /// A line may break here.
    Allowed(usize),
}

impl LineBreak {
    /// The byte offset of the opportunity.
    #[must_use]
    pub fn offset(self) -> usize {
        match self {
            Self::Mandatory(offset) | Self::Allowed(offset) => offset,
        }
    }
}

/// An iterator over the line-break opportunities of a string. Created by [`line_breaks`].
#[derive(Clone)]
#[must_use = "iterators are lazy and do nothing unless consumed"]
pub struct LineBreaks<'a> {
    input: &'a str,
    text: sys::mjb_text,
    state: sys::mjb_break_state,
    done: bool,
}

impl Iterator for LineBreaks<'_> {
    type Item = LineBreak;

    fn next(&mut self) -> Option<LineBreak> {
        while !self.done {
            // SAFETY: see Segments::next.
            let kind = unsafe { sys::mjb_line_break_next(&self.text, &mut self.state) };
            let position = self.state.position.min(self.input.len());

            match kind {
                sys::MJB_BT_NOT_SET => self.done = true,
                sys::MJB_BT_MANDATORY => return Some(LineBreak::Mandatory(position)),
                sys::MJB_BT_ALLOWED if position > 0 => return Some(LineBreak::Allowed(position)),
                _ => {}
            }
        }

        None
    }
}

impl FusedIterator for LineBreaks<'_> {}

// SAFETY: see the Send and Sync implementations of Segments.
unsafe impl Send for LineBreaks<'_> {}
unsafe impl Sync for LineBreaks<'_> {}

/// Iterate over the line-break opportunities of `s` (UAX #14), as byte offsets.
pub fn line_breaks(s: &str) -> LineBreaks<'_> {
    LineBreaks {
        input: s,
        text: ffi::text(s),
        state: sys::mjb_break_state::default(),
        done: s.is_empty(),
    }
}
