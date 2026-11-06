// The Mojibake library
//
// This file is distributed under the MIT License. See LICENSE for details.

use std::fmt;

use mojibake_sys as sys;

use crate::ffi;

/// A Unicode character property, for [`CharExt::has_property`](crate::CharExt::has_property)
/// and [`CharExt::property_value`](crate::CharExt::property_value).
///
/// Look one up by its long name with [`Property::from_name`], or list them with
/// [`Property::all`]. The set of properties comes from the C library's data.
///
/// ```
/// use mojibake::{CharExt, Property};
///
/// let dash = Property::from_name("Dash").unwrap();
/// assert!(dash.is_binary());
/// assert!('—'.has_property(dash));
/// ```
#[derive(Clone, Copy, PartialEq, Eq, Hash)]
pub struct Property(pub(crate) sys::mjb_property);

/// The name `mjb_property_name` returns for an out-of-range property.
const UNKNOWN: &str = "Unknown";

impl Property {
    /// Every property the library knows, in the C enum order.
    pub fn all() -> impl Iterator<Item = Self> {
        (0..)
            .map(Self)
            .take_while(|property| property.raw_name() != Some(UNKNOWN))
    }

    /// Look up a property by its long name, such as `"White_Space"` or `"kTotalStrokes"`.
    ///
    /// Matching ignores ASCII case, spaces, hyphens, and underscores (UAX #44 LM3), so
    /// `"white space"` and `"WhiteSpace"` also find `White_Space`. Short aliases such as `"WSpace"`
    /// are not recognized.
    pub fn from_name(name: &str) -> Option<Self> {
        let wanted = loose(name);

        if wanted.is_empty() {
            return None;
        }

        Self::all().find(|property| loose(property.name()) == wanted)
    }

    /// The long name of the property, such as `"Alphabetic"`.
    pub fn name(self) -> &'static str {
        self.raw_name().unwrap_or(UNKNOWN)
    }

    /// True for a binary property, which [`CharExt::has_property`](crate::CharExt::has_property)
    /// reads. Other properties are read with
    /// [`CharExt::property_value`](crate::CharExt::property_value).
    pub fn is_binary(self) -> bool {
        let mut value = false;

        // SAFETY: the pointer is valid for the duration of the call. U+0000 is a valid
        // codepoint, so the only failure is a property that is not binary.
        let status = unsafe { sys::mjb_codepoint_property_binary(0, self.0, &mut value) };

        status == sys::MJB_STATUS_OK
    }

    fn raw_name(self) -> Option<&'static str> {
        // SAFETY: mjb_property_name returns a static string for every input.
        unsafe { ffi::static_str(sys::mjb_property_name(self.0)) }
    }
}

impl fmt::Debug for Property {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "Property({})", self.name())
    }
}

impl fmt::Display for Property {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str(self.name())
    }
}

/// UAX #44 LM3: ignore case, whitespace, hyphens, and underscores.
fn loose(name: &str) -> String {
    name.chars()
        .filter(|&c| !matches!(c, ' ' | '\t' | '-' | '_'))
        .map(|c| c.to_ascii_lowercase())
        .collect()
}
