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

/// The `Bidi_Class` of a character (UAX #9 table 4).
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub enum BidiClass {
    /// `L`: Strong left to right: most letters.
    LeftToRight,
    /// `R`: Strong right to left: Hebrew and other right-to-left letters.
    RightToLeft,
    /// `AL`: Strong right to left: Arabic, Syriac, and Thaana letters.
    ArabicLetter,
    /// `EN`: European digits.
    EuropeanNumber,
    /// `ES`: Plus and minus signs.
    EuropeanSeparator,
    /// `ET`: Currency, degree, and percent signs.
    EuropeanTerminator,
    /// `AN`: Arabic-Indic digits.
    ArabicNumber,
    /// `CS`: Commas, colons, and periods between numbers.
    CommonSeparator,
    /// `NSM`: Nonspacing marks, which take the class of the preceding character.
    NonspacingMark,
    /// `BN`: Default ignorables, removed by rule X9.
    BoundaryNeutral,
    /// `B`: Paragraph separators.
    ParagraphSeparator,
    /// `S`: Tabs.
    SegmentSeparator,
    /// `WS`: Spaces.
    WhiteSpace,
    /// `ON`: Other neutrals, such as most punctuation and symbols.
    OtherNeutral,
    /// `LRE`: U+202A LEFT-TO-RIGHT EMBEDDING.
    LeftToRightEmbedding,
    /// `LRO`: U+202D LEFT-TO-RIGHT OVERRIDE.
    LeftToRightOverride,
    /// `RLE`: U+202B RIGHT-TO-LEFT EMBEDDING.
    RightToLeftEmbedding,
    /// `RLO`: U+202E RIGHT-TO-LEFT OVERRIDE.
    RightToLeftOverride,
    /// `PDF`: U+202C POP DIRECTIONAL FORMATTING.
    PopDirectionalFormat,
    /// `LRI`: U+2066 LEFT-TO-RIGHT ISOLATE.
    LeftToRightIsolate,
    /// `RLI`: U+2067 RIGHT-TO-LEFT ISOLATE.
    RightToLeftIsolate,
    /// `FSI`: U+2068 FIRST STRONG ISOLATE.
    FirstStrongIsolate,
    /// `PDI`: U+2069 POP DIRECTIONAL ISOLATE.
    PopDirectionalIsolate,
}

impl BidiClass {
    pub(crate) fn from_raw(raw: sys::mjb_bidi_class) -> Option<Self> {
        Some(match raw {
            sys::MJB_PR_BIDI_CLASS_L => Self::LeftToRight,
            sys::MJB_PR_BIDI_CLASS_R => Self::RightToLeft,
            sys::MJB_PR_BIDI_CLASS_AL => Self::ArabicLetter,
            sys::MJB_PR_BIDI_CLASS_EN => Self::EuropeanNumber,
            sys::MJB_PR_BIDI_CLASS_ES => Self::EuropeanSeparator,
            sys::MJB_PR_BIDI_CLASS_ET => Self::EuropeanTerminator,
            sys::MJB_PR_BIDI_CLASS_AN => Self::ArabicNumber,
            sys::MJB_PR_BIDI_CLASS_CS => Self::CommonSeparator,
            sys::MJB_PR_BIDI_CLASS_NSM => Self::NonspacingMark,
            sys::MJB_PR_BIDI_CLASS_BN => Self::BoundaryNeutral,
            sys::MJB_PR_BIDI_CLASS_B => Self::ParagraphSeparator,
            sys::MJB_PR_BIDI_CLASS_S => Self::SegmentSeparator,
            sys::MJB_PR_BIDI_CLASS_WS => Self::WhiteSpace,
            sys::MJB_PR_BIDI_CLASS_ON => Self::OtherNeutral,
            sys::MJB_PR_BIDI_CLASS_LRE => Self::LeftToRightEmbedding,
            sys::MJB_PR_BIDI_CLASS_LRO => Self::LeftToRightOverride,
            sys::MJB_PR_BIDI_CLASS_RLE => Self::RightToLeftEmbedding,
            sys::MJB_PR_BIDI_CLASS_RLO => Self::RightToLeftOverride,
            sys::MJB_PR_BIDI_CLASS_PDF => Self::PopDirectionalFormat,
            sys::MJB_PR_BIDI_CLASS_LRI => Self::LeftToRightIsolate,
            sys::MJB_PR_BIDI_CLASS_RLI => Self::RightToLeftIsolate,
            sys::MJB_PR_BIDI_CLASS_FSI => Self::FirstStrongIsolate,
            sys::MJB_PR_BIDI_CLASS_PDI => Self::PopDirectionalIsolate,
            _ => return None,
        })
    }

    /// The short alias of the class, such as `"AL"`.
    #[must_use]
    pub fn abbreviation(self) -> &'static str {
        match self {
            Self::LeftToRight => "L",
            Self::RightToLeft => "R",
            Self::ArabicLetter => "AL",
            Self::EuropeanNumber => "EN",
            Self::EuropeanSeparator => "ES",
            Self::EuropeanTerminator => "ET",
            Self::ArabicNumber => "AN",
            Self::CommonSeparator => "CS",
            Self::NonspacingMark => "NSM",
            Self::BoundaryNeutral => "BN",
            Self::ParagraphSeparator => "B",
            Self::SegmentSeparator => "S",
            Self::WhiteSpace => "WS",
            Self::OtherNeutral => "ON",
            Self::LeftToRightEmbedding => "LRE",
            Self::LeftToRightOverride => "LRO",
            Self::RightToLeftEmbedding => "RLE",
            Self::RightToLeftOverride => "RLO",
            Self::PopDirectionalFormat => "PDF",
            Self::LeftToRightIsolate => "LRI",
            Self::RightToLeftIsolate => "RLI",
            Self::FirstStrongIsolate => "FSI",
            Self::PopDirectionalIsolate => "PDI",
        }
    }

    /// True for the strong classes `L`, `R`, and `AL`.
    #[must_use]
    pub fn is_strong(self) -> bool {
        matches!(
            self,
            Self::LeftToRight | Self::RightToLeft | Self::ArabicLetter
        )
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
    /// The class of the character after the weak and neutral rules (W1-W7, N1-N2) resolved it.
    /// `ch.bidi_class()` gives its original `Bidi_Class`.
    pub class: BidiClass,
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
    ///
    /// # Errors
    ///
    /// [`Error::NoMemory`] if the library cannot allocate memory.
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
    #[must_use]
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
    #[must_use]
    pub fn len(&self) -> usize {
        self.raw_chars().len()
    }

    /// Whether the paragraph has no characters.
    #[must_use]
    pub fn is_empty(&self) -> bool {
        self.len() == 0
    }

    /// The paragraph embedding level: 0 for left to right, 1 for right to left.
    #[must_use]
    pub fn level(&self) -> u8 {
        self.raw.paragraph_level
    }

    /// The resolved paragraph direction, never [`Direction::Auto`].
    #[must_use]
    pub fn direction(&self) -> Direction {
        Direction::from_level(self.raw.paragraph_level)
    }

    /// The resolved embedding level of each character, in logical order.
    #[must_use]
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
            // The library always resolves a class; ON is only a defensive fallback.
            class: BidiClass::from_raw(c.resolved_class).unwrap_or(BidiClass::OtherNeutral),
            mirror: if c.mirroring_glyph == 0 {
                None
            } else {
                char::from_u32(c.mirroring_glyph)
            },
        })
    }

    /// The bytes of the text that the paragraph positions `range` cover, including any removed
    /// control characters that follow the last one.
    ///
    /// # Errors
    ///
    /// [`Error::InvalidArgument`] if `range.start > range.end` or
    /// `range.end` is past [`Paragraph::len`].
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
    ///
    /// # Errors
    ///
    /// [`Error::InvalidArgument`] if `line.start > line.end` or
    /// `line.end` is past [`Paragraph::len`].
    ///
    /// [`Error::NoMemory`] if the library cannot allocate memory.
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
    ///
    /// # Errors
    ///
    /// [`Error::InvalidArgument`] if `line.start > line.end` or
    /// `line.end` is past [`Paragraph::len`].
    ///
    /// [`Error::NoMemory`] if the library cannot allocate memory.
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
    ///
    /// # Errors
    ///
    /// [`Error::InvalidArgument`] if `line.start > line.end` or
    /// `line.end` is past [`Paragraph::len`].
    ///
    /// [`Error::NoMemory`] if the library cannot allocate memory.
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
