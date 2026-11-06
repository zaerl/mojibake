// The Mojibake library
//
// This file is distributed under the MIT License. See LICENSE for details.

use std::fmt;
use std::ops::RangeInclusive;

use mojibake_sys as sys;

use crate::ffi;

/// One of the 17 planes of 65,536 codepoints in the Unicode codespace.
///
/// ```
/// use mojibake::{CharExt, Plane};
///
/// assert_eq!('A'.plane(), Plane::BMP);
/// assert_eq!('😀'.plane().name(), Some("Supplementary Multilingual Plane"));
/// assert_eq!('\u{e0001}'.plane().abbreviation(), Some("SSP"));
/// assert_eq!(Plane::new(4).unwrap().name(), None); // unassigned
/// ```
#[derive(Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub struct Plane(u8);

impl Plane {
    /// Plane 0, the Basic Multilingual Plane.
    pub const BMP: Self = Self(sys::MJB_PLANE_BMP as u8);
    /// Plane 1, the Supplementary Multilingual Plane.
    pub const SMP: Self = Self(sys::MJB_PLANE_SMP as u8);
    /// Plane 2, the Supplementary Ideographic Plane.
    pub const SIP: Self = Self(sys::MJB_PLANE_SIP as u8);
    /// Plane 3, the Tertiary Ideographic Plane.
    pub const TIP: Self = Self(sys::MJB_PLANE_TIP as u8);
    /// Plane 14, the Supplementary Special-purpose Plane.
    pub const SSP: Self = Self(sys::MJB_PLANE_SSP as u8);
    /// Plane 15, Supplementary Private Use Area-A.
    pub const PUA_A: Self = Self(sys::MJB_PLANE_PUA_A as u8);
    /// Plane 16, Supplementary Private Use Area-B.
    pub const PUA_B: Self = Self(sys::MJB_PLANE_PUA_B as u8);

    /// Plane `number`, or `None` if it is not 0 to 16.
    pub const fn new(number: u8) -> Option<Self> {
        if (number as i32) < sys::MJB_PLANE_NUM {
            Some(Self(number))
        } else {
            None
        }
    }

    /// The plane that contains `c`.
    pub const fn of(c: char) -> Self {
        // A char is at most U+10FFFF, so the plane is at most 16.
        Self((c as u32 >> 16) as u8)
    }

    /// The plane number, 0 to 16.
    pub const fn number(self) -> u8 {
        self.0
    }

    /// The codepoints of the plane, such as `0x10000..=0x1FFFF` for plane 1.
    pub const fn codepoints(self) -> RangeInclusive<u32> {
        let start = self.0 as u32 * sys::MJB_PLANE_SIZE as u32;

        start..=start + (sys::MJB_PLANE_SIZE as u32 - 1)
    }

    /// The name of the plane, such as `"Basic Multilingual Plane"`, or `None` for the unassigned
    /// planes 4 to 13.
    pub fn name(self) -> Option<&'static str> {
        self.raw_name(false)
    }

    /// The abbreviation of the plane, such as `"BMP"` or `"PUA-A"`, or `None` for the unassigned
    /// planes 4 to 13.
    pub fn abbreviation(self) -> Option<&'static str> {
        self.raw_name(true)
    }

    /// True for the seven planes with a name. Planes 4 to 13 have none yet.
    pub fn is_assigned(self) -> bool {
        self.name().is_some()
    }

    fn raw_name(self, abbreviation: bool) -> Option<&'static str> {
        // SAFETY: mjb_plane_name returns NULL or a static string.
        let name = unsafe {
            ffi::static_str(sys::mjb_plane_name(
                sys::mjb_plane::from(self.0),
                abbreviation,
            ))
        };

        name.filter(|&name| name != "Unassigned")
    }
}

impl fmt::Debug for Plane {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self.abbreviation() {
            Some(abbreviation) => write!(f, "Plane({}, {abbreviation})", self.0),
            None => write!(f, "Plane({})", self.0),
        }
    }
}
