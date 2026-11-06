// The Mojibake library
//
// This file is distributed under the MIT License. See LICENSE for details.

//! Locale-independent DUCET collation (UTS #10).

use std::cmp::Ordering;

use mojibake_sys as sys;

use crate::error::{check, Result};
use crate::ffi::{self, Output};

/// The maximum comparison level (UTS #10 section 3.1).
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash, Default)]
pub enum Strength {
    /// Base characters only.
    Primary,
    /// Plus accents.
    Secondary,
    /// Plus case and other tertiary variants.
    #[default]
    Tertiary,
    /// Plus shifted variable elements such as punctuation.
    Quaternary,
}

/// How variable elements (spaces, punctuation, some symbols) are weighted (UTS #10 section 4).
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash, Default)]
pub enum VariableWeighting {
    /// Variable elements keep their weights.
    #[default]
    NonIgnorable,
    /// Variable elements are ignored up to the quaternary level.
    Shifted,
}

/// Collation options. The default is tertiary strength with non-ignorable variable weighting.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash, Default)]
pub struct Collator {
    pub strength: Strength,
    pub variable_weighting: VariableWeighting,
}

impl Collator {
    fn raw(
        self,
    ) -> (
        sys::mjb_collation_variable_weighting,
        sys::mjb_collation_strength,
    ) {
        let weighting = match self.variable_weighting {
            VariableWeighting::NonIgnorable => sys::MJB_COLLATION_NON_IGNORABLE,
            VariableWeighting::Shifted => sys::MJB_COLLATION_SHIFTED,
        };
        let strength = match self.strength {
            Strength::Primary => sys::MJB_COLLATION_PRIMARY,
            Strength::Secondary => sys::MJB_COLLATION_SECONDARY,
            Strength::Tertiary => sys::MJB_COLLATION_TERTIARY,
            Strength::Quaternary => sys::MJB_COLLATION_QUATERNARY,
        };

        (weighting, strength)
    }

    /// Compare two strings.
    pub fn compare(self, a: &str, b: &str) -> Ordering {
        let (weighting, strength) = self.raw();
        let first = ffi::text(a);
        let second = ffi::text(b);
        let mut order = 0;

        // SAFETY: all pointers are valid for the duration of the call.
        let status = unsafe {
            sys::mjb_collation_compare(
                &first,
                &second,
                sys::MJB_MALFORMED_REPLACE,
                weighting,
                strength,
                &mut order,
                std::ptr::null_mut(),
                std::ptr::null_mut(),
            )
        };
        debug_assert_eq!(status, sys::MJB_STATUS_OK);

        order.cmp(&0)
    }

    /// A binary sort key: comparing keys bytewise gives the same order as [`Collator::compare`].
    pub fn sort_key(self, s: &str) -> Result<Vec<u8>> {
        let (weighting, strength) = self.raw();
        let text = ffi::text(s);
        let mut output = Output::new();

        // SAFETY: all pointers are valid for the duration of the call.
        let status = unsafe {
            sys::mjb_collation_key(
                &text,
                sys::MJB_MALFORMED_REPLACE,
                weighting,
                strength,
                &mut output.0,
                std::ptr::null_mut(),
            )
        };
        check(status, None)?;

        Ok(output.into_vec())
    }
}

/// Compare two strings with the default [`Collator`].
pub fn compare(a: &str, b: &str) -> Ordering {
    Collator::default().compare(a, b)
}

/// A sort key with the default [`Collator`].
pub fn sort_key(s: &str) -> Result<Vec<u8>> {
    Collator::default().sort_key(s)
}
