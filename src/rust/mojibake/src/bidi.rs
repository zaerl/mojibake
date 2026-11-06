// The Mojibake library
//
// This file is distributed under the MIT License. See LICENSE for details.

//! The Unicode Bidirectional Algorithm (UAX #9).
//!
//! [`Paragraph::new`] resolves the embedding level of every character of one paragraph. A line of
//! it is then reordered for display with [`Paragraph::reorder_line`], split into directional runs
//! with [`Paragraph::runs`], or rendered in visual order with [`Paragraph::visual`].
//!
//! Positions are indices into [`Paragraph::chars`], not codepoint indices of the text: the
//! characters that rule X9 removes (embedding and override controls, and Boundary Neutrals) are
//! not part of the paragraph. Map a position back to the text with [`BidiChar::byte_offset`] or
//! [`Paragraph::byte_range`].
//!
//! ```
//! use mojibake::bidi::{Direction, Paragraph};
//!
//! let paragraph = Paragraph::new("abc אבג", Direction::Ltr)?;
//! assert_eq!(paragraph.levels(), [0, 0, 0, 0, 1, 1, 1]);
//! assert_eq!(paragraph.visual(0..paragraph.len())?, "abc גבא");
//! # Ok::<(), mojibake::Error>(())
//! ```

use std::fmt;
use std::ops::Range;

use mojibake_sys as sys;

use crate::error::{check, Error, Result};
use crate::ffi;

/// The base direction of a paragraph.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq, Hash)]
pub enum Direction {
    /// Left to right.
    Ltr,
    /// Right to left.
    Rtl,
    /// Taken from the first strong character (rules P2 and P3), left to right if there is none.
    /// Only valid as input to [`Paragraph::new`].
    #[default]
    Auto,
}

impl Direction {
    fn raw(self) -> sys::mjb_direction {
        match self {
            Self::Ltr => sys::MJB_DIRECTION_LTR,
            Self::Rtl => sys::MJB_DIRECTION_RTL,
            Self::Auto => sys::MJB_DIRECTION_AUTO,
        }
    }

    fn from_level(level: u8) -> Self {
        if level & 1 == 1 {
            Self::Rtl
        } else {
            Self::Ltr
        }
    }
}

/// A character of a resolved paragraph.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub struct BidiChar {
    /// The character.
    pub ch: char,
    /// Byte offset of the character in the paragraph text.
    pub byte_offset: usize,
    /// Resolved embedding level. Odd levels are right to left.
    pub level: u8,
    /// The Bidi_Mirroring_Glyph to display instead of `ch` (rule L4), when `ch` is mirrored and
    /// its level is odd.
    pub mirror: Option<char>,
}

/// A run of characters with the same embedding level on a reordered line.
#[derive(Clone, Debug, PartialEq, Eq, Hash)]
pub struct Run {
    /// Positions of the run in the visual order: indices into the vector that
    /// [`Paragraph::reorder_line`] returns for the same line.
    pub visual: Range<usize>,
    /// The paragraph positions the run covers, in logical order.
    pub logical: Range<usize>,
    /// Embedding level of the run.
    pub level: u8,
    /// Direction of the run: [`Direction::Rtl`] for odd levels, [`Direction::Ltr`] otherwise.
    pub direction: Direction,
}

/// A paragraph whose embedding levels have been resolved.
pub struct Paragraph<'a> {
    text: &'a str,
    raw: sys::mjb_bidi_paragraph,
}

impl<'a> Paragraph<'a> {
    /// Resolve `text` as one paragraph with base direction `direction`.
    ///
    /// The text should not contain paragraph separators other than an optional trailing one;
    /// split it into paragraphs first.
    pub fn new(text: &'a str, direction: Direction) -> Result<Self> {
        let input = ffi::text(text);
        let mut raw = sys::mjb_bidi_paragraph::default();
        // SAFETY: all pointers are valid for the duration of the call. On failure the library
        // leaves `raw` empty, and freeing an empty paragraph is a no-op.
        let status = unsafe { sys::mjb_bidi_resolve(&input, direction.raw(), &mut raw) };
        let paragraph = Self { text, raw };

        check(status, None)?;
        Ok(paragraph)
    }

    /// The paragraph text.
    pub fn text(&self) -> &'a str {
        self.text
    }

    fn raw_chars(&self) -> &[sys::mjb_bidi_char] {
        if self.raw.chars.is_null() || self.raw.count == 0 {
            return &[];
        }

        // SAFETY: mjb_bidi_resolve allocated `count` initialized characters at `chars`, and they
        // live until Drop frees them.
        unsafe { std::slice::from_raw_parts(self.raw.chars, self.raw.count) }
    }

    /// The number of characters of the paragraph, after rule X9 removed the control characters.
    pub fn len(&self) -> usize {
        self.raw_chars().len()
    }

    /// Whether the paragraph has no characters.
    pub fn is_empty(&self) -> bool {
        self.len() == 0
    }

    /// The paragraph embedding level: 0 for left to right, 1 for right to left.
    pub fn level(&self) -> u8 {
        self.raw.paragraph_level
    }

    /// The resolved paragraph direction, never [`Direction::Auto`].
    pub fn direction(&self) -> Direction {
        Direction::from_level(self.raw.paragraph_level)
    }

    /// The resolved embedding level of each character, in logical order.
    pub fn levels(&self) -> Vec<u8> {
        self.raw_chars().iter().map(|c| c.level).collect()
    }

    /// The characters of the paragraph, in logical order.
    pub fn chars(&self) -> impl ExactSizeIterator<Item = BidiChar> + '_ {
        self.raw_chars().iter().map(|c| BidiChar {
            // The library decoded these from a &str, so they are scalar values.
            ch: char::from_u32(c.codepoint).unwrap_or(char::REPLACEMENT_CHARACTER),
            byte_offset: c.byte_offset,
            level: c.level,
            mirror: if c.mirroring_glyph == 0 {
                None
            } else {
                char::from_u32(c.mirroring_glyph)
            },
        })
    }

    /// The bytes of the text that the paragraph positions `range` cover, including any removed
    /// control characters that follow the last one.
    pub fn byte_range(&self, range: Range<usize>) -> Result<Range<usize>> {
        let chars = self.raw_chars();

        if range.start > range.end || range.end > chars.len() {
            return Err(Error::InvalidArgument);
        }

        let offset = |index: usize| chars.get(index).map_or(self.text.len(), |c| c.byte_offset);

        Ok(offset(range.start)..offset(range.end))
    }

    /// Reorder the line made of the paragraph positions `line` for display (rules L1 and L2).
    ///
    /// The result lists the paragraph positions in visual order, from left to right. An empty
    /// line gives an empty vector.
    pub fn reorder_line(&self, line: Range<usize>) -> Result<Vec<usize>> {
        if line.start > line.end || line.end > self.len() {
            return Err(Error::InvalidArgument);
        }

        if line.is_empty() {
            return Ok(Vec::new());
        }

        let mut order = vec![0; line.len()];
        // SAFETY: `order` has room for `line.len()` positions, and `line` is within the paragraph.
        let status = unsafe {
            sys::mjb_bidi_reorder_line(&self.raw, line.start, line.end, order.as_mut_ptr())
        };

        check(status, None)?;
        Ok(order)
    }

    /// Split the reordered line made of the paragraph positions `line` into level runs, from left
    /// to right.
    pub fn runs(&self, line: Range<usize>) -> Result<Vec<Run>> {
        let order = self.reorder_line(line)?;
        let mut count = 0;

        // SAFETY: `order` holds `order.len()` valid paragraph positions. A NULL `runs` only counts.
        check(
            unsafe {
                sys::mjb_bidi_line_runs(
                    &self.raw,
                    order.as_ptr(),
                    order.len(),
                    std::ptr::null_mut(),
                    &mut count,
                )
            },
            None,
        )?;

        let mut raw = vec![sys::mjb_bidi_run::default(); count];

        // SAFETY: `raw` has room for the `count` runs the library just counted.
        check(
            unsafe {
                sys::mjb_bidi_line_runs(
                    &self.raw,
                    order.as_ptr(),
                    order.len(),
                    raw.as_mut_ptr(),
                    &mut count,
                )
            },
            None,
        )?;
        raw.truncate(count);

        raw.iter()
            .map(|run| {
                let positions = order
                    .get(run.start..run.end)
                    .ok_or(Error::InvalidArgument)?;
                // Rule L2 reverses contiguous logical ranges, so a level run of the visual order
                // covers a contiguous logical range.
                let first = positions.iter().copied().min().unwrap_or(0);
                let last = positions.iter().copied().max().map_or(0, |p| p + 1);

                Ok(Run {
                    visual: run.start..run.end,
                    logical: first..last,
                    level: run.level,
                    direction: Direction::from_level(run.level),
                })
            })
            .collect()
    }

    /// The line made of the paragraph positions `line` in visual order, with mirrored characters
    /// replaced by their mirroring glyphs (rule L4). Removed control characters are left out.
    pub fn visual(&self, line: Range<usize>) -> Result<String> {
        let order = self.reorder_line(line)?;
        let chars: Vec<BidiChar> = self.chars().collect();

        Ok(order
            .into_iter()
            .map(|index| {
                let c = chars[index];
                c.mirror.unwrap_or(c.ch)
            })
            .collect())
    }
}

// SAFETY: the paragraph owns its character array, the methods only read it, and the library frees
// it with the process-global allocator, which must be thread-safe.
unsafe impl Send for Paragraph<'_> {}
unsafe impl Sync for Paragraph<'_> {}

impl Drop for Paragraph<'_> {
    fn drop(&mut self) {
        // SAFETY: `raw` was filled by mjb_bidi_resolve, or is still empty.
        unsafe { sys::mjb_bidi_paragraph_free(&mut self.raw) };
    }
}

impl fmt::Debug for Paragraph<'_> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("Paragraph")
            .field("text", &self.text)
            .field("direction", &self.direction())
            .field("levels", &self.levels())
            .finish()
    }
}
