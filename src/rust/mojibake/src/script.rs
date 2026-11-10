// The Mojibake library
//
// This file is distributed under the MIT License. See LICENSE for details.

use std::fmt;
use std::ops::RangeInclusive;
use std::str::FromStr;
use std::sync::OnceLock;

use mojibake_sys as sys;

use crate::error::Error;
use crate::ffi;

/// A Unicode script, identified by its ISO 15924 code.
///
/// It is returned by [`CharExt::script`](crate::CharExt::script) and
/// [`CharExt::script_extensions`](crate::CharExt::script_extensions), and parsed from a code with
/// [`Script::from_code`] or [`str::parse`].
///
/// ```
/// use mojibake::{CharExt, Script};
///
/// let latin: Script = "Latn".parse()?;
/// assert_eq!('a'.script(), Some(latin));
/// assert_eq!(latin.code(), "Latn");
/// assert_eq!(latin.to_string(), "Latn");
/// # Ok::<(), mojibake::Error>(())
/// ```
#[derive(Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub struct Script(sys::mjb_script);

impl Script {
    /// `Zyyy`, the `Common` script of characters shared by several scripts, such as digits.
    pub const COMMON: Self = Self(sys::MJB_SC_ZYYY);
    /// `Zinh`, the `Inherited` script of combining marks that take the script of their base.
    pub const INHERITED: Self = Self(sys::MJB_SC_ZINH);
    /// `Zzzz`, the `Unknown` script of unassigned, private-use, and surrogate code points.
    pub const UNKNOWN: Self = Self(sys::MJB_SC_ZZZZ);

    pub(crate) fn from_raw(raw: sys::mjb_script) -> Option<Self> {
        (raw > sys::MJB_SC_NOT_SET && raw < sys::MJB_SC_COUNT).then_some(Self(raw))
    }

    /// Every script, in the order of the C `mjb_script` enum: the `Script` property values,
    /// then the UTS #39 writing-system values `Hanb`, `Jpan`, and `Kore`.
    pub fn all() -> impl Iterator<Item = Self> {
        (sys::MJB_SC_NOT_SET + 1..sys::MJB_SC_COUNT).map(Self)
    }

    /// The script whose ISO 15924 code is `code`, ignoring ASCII case: `"latn"` is
    /// `Script::from_code("Latn")`.
    #[must_use]
    pub fn from_code(code: &str) -> Option<Self> {
        Self::all().find(|script| script.code().eq_ignore_ascii_case(code))
    }

    /// The four-letter ISO 15924 code, such as `"Latn"`.
    #[must_use]
    pub fn code(self) -> &'static str {
        // SAFETY: mjb_script_code returns NULL or a static string.
        unsafe { ffi::static_str(sys::mjb_script_code(self.0)) }.unwrap_or("")
    }

    /// True for a value of the `Script` property, false for the UTS #39 writing-system values
    /// `Hanb` (Han with Bopomofo), `Jpan` (Japanese), and `Kore` (Korean).
    #[must_use]
    pub fn is_property_value(self) -> bool {
        self.0 < sys::MJB_SC_PROPERTY_COUNT
    }

    /// The raw C `mjb_script` value.
    #[must_use]
    pub fn as_raw(self) -> i32 {
        self.0
    }
}

impl fmt::Debug for Script {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "Script({})", self.code())
    }
}

impl fmt::Display for Script {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str(self.code())
    }
}

impl FromStr for Script {
    type Err = Error;

    /// Parse an ISO 15924 code. Returns [`Error::NotFound`] for an unknown code.
    fn from_str(s: &str) -> Result<Self, Error> {
        Self::from_code(s).ok_or(Error::NotFound)
    }
}

/// A Unicode block, such as `Basic Latin` (U+0000 to U+007F).
///
/// Returned by [`CharExt::block`](crate::CharExt::block) and [`Block::of`].
///
/// ```
/// use mojibake::{Block, CharExt};
///
/// let block = 'é'.block().unwrap();
/// assert_eq!(block.name(), "Latin-1 Supplement");
/// assert_eq!(block.codepoints(), 0x80..=0xFF);
/// assert!(block.contains('ÿ'));
/// assert_eq!(Block::of('a').unwrap().to_string(), "Basic Latin");
/// ```
#[derive(Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub struct Block {
    id: sys::mjb_block,
    start: u32,
    end: u32,
}

// Each block name is read from the library once and kept for the life of the program, so that
// `Block` stays `Copy` and `name` can return a `&'static str`. There are MJB_BLOCK_NUM names.
#[allow(clippy::declare_interior_mutable_const)]
const NO_NAME: OnceLock<&'static str> = OnceLock::new();
static NAMES: [OnceLock<&'static str>; sys::MJB_BLOCK_NUM as usize] =
    [NO_NAME; sys::MJB_BLOCK_NUM as usize];

impl Block {
    /// The block that contains `c`, or `None` if `c` is in no block.
    pub fn of(c: char) -> Option<Self> {
        // SAFETY: mjb_block_info is plain data; all-zero is a valid value.
        let mut info: sys::mjb_block_info = unsafe { std::mem::zeroed() };
        // SAFETY: the pointer is valid for the duration of the call.
        let status = unsafe { sys::mjb_codepoint_block(c as u32, &mut info) };

        if status != sys::MJB_STATUS_OK {
            return None;
        }

        let index = usize::try_from(info.id).ok()?;
        let name = NAMES.get(index)?;
        name.get_or_init(|| Box::leak(ffi::array_str(&info.name).into_boxed_str()));

        Some(Self {
            id: info.id,
            start: info.start,
            end: info.end,
        })
    }

    /// The block name, such as `"Basic Latin"`.
    pub fn name(self) -> &'static str {
        // `Block::of` stored the name before it returned the block.
        usize::try_from(self.id)
            .ok()
            .and_then(|index| NAMES.get(index))
            .and_then(OnceLock::get)
            .copied()
            .unwrap_or("")
    }

    /// The code points of the block, including unassigned ones.
    #[must_use]
    pub fn codepoints(self) -> RangeInclusive<u32> {
        self.start..=self.end
    }

    /// True if `c` is in the block.
    #[must_use]
    pub fn contains(self, c: char) -> bool {
        self.codepoints().contains(&u32::from(c))
    }

    /// The raw C `mjb_block` value.
    #[must_use]
    pub fn as_raw(self) -> i32 {
        self.id
    }
}

impl fmt::Debug for Block {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(
            f,
            "Block({:?}, U+{:04X}..=U+{:04X})",
            self.name(),
            self.start,
            self.end
        )
    }
}

impl fmt::Display for Block {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str(self.name())
    }
}
