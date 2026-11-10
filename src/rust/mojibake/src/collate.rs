// The Mojibake library
//
// This file is distributed under the MIT License. See LICENSE for details.

//! Locale-independent DUCET collation (UTS #10).
//!
//! ```
//! use mojibake::collate;
//!
//! let mut words = vec!["b", "Ä", "a", "C"];
//! collate::sort(&mut words);
//! assert_eq!(words, ["a", "Ä", "b", "C"]);
//! ```

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

/// A binary collation sort key, created by [`Collator::sort_key`].
///
/// Keys made by the same [`Collator`] compare (with [`Ord`]) in the same order as
/// [`Collator::compare`] compares their strings. Keys made with different options are not
/// comparable in any meaningful way.
#[derive(Clone, Debug, PartialEq, Eq, PartialOrd, Ord, Hash, Default)]
pub struct SortKey(Vec<u8>);

impl SortKey {
    /// The key bytes.
    #[must_use]
    pub fn as_bytes(&self) -> &[u8] {
        &self.0
    }

    /// The key bytes, for storage in a database index or a file.
    #[must_use]
    pub fn into_bytes(self) -> Vec<u8> {
        self.0
    }
}

impl AsRef<[u8]> for SortKey {
    fn as_ref(&self) -> &[u8] {
        &self.0
    }
}

impl From<SortKey> for Vec<u8> {
    fn from(key: SortKey) -> Self {
        key.0
    }
}

/// Collation options. The default is tertiary strength with non-ignorable variable weighting.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash, Default)]
pub struct Collator {
    /// The maximum comparison level.
    pub strength: Strength,
    /// How spaces, punctuation, and some symbols are weighted.
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
    ///
    /// # Panics
    ///
    /// If the library cannot allocate memory. [`Collator::try_compare`] returns the error
    /// instead. A failed comparison is never reported as [`Ordering::Equal`], which would
    /// silently break a sort.
    #[must_use]
    pub fn compare(self, a: &str, b: &str) -> Ordering {
        self.try_compare(a, b)
            .unwrap_or_else(|error| panic!("mojibake: {error}"))
    }

    /// Like [`Collator::compare`], but returns an error instead of panicking.
    ///
    /// # Errors
    ///
    /// [`Error::NoMemory`](crate::Error::NoMemory) if the library cannot allocate memory.
    pub fn try_compare(self, a: &str, b: &str) -> Result<Ordering> {
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
        check(status, None)?;

        Ok(order.cmp(&0))
    }

    /// A binary sort key: comparing keys gives the same order as [`Collator::compare`].
    ///
    /// Compute keys once when the same strings are compared many times, as in a sort or an
    /// index.
    ///
    /// # Errors
    ///
    /// [`Error::NoMemory`](crate::Error::NoMemory) if the library cannot allocate memory.
    pub fn sort_key(self, s: &str) -> Result<SortKey> {
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

        Ok(SortKey(output.into_vec()))
    }

    /// Sort `items` in collation order. The sort is stable, so items that compare equal keep
    /// their order.
    ///
    /// It computes one sort key per item, rather than calling the library once per comparison.
    ///
    /// ```
    /// use mojibake::collate::{Collator, Strength};
    ///
    /// let primary = Collator { strength: Strength::Primary, ..Collator::default() };
    /// let mut words = vec!["b", "A", "a"];
    /// primary.sort(&mut words);
    /// assert_eq!(words, ["A", "a", "b"]);
    /// ```
    ///
    /// # Panics
    ///
    /// If the library cannot allocate memory.
    pub fn sort<S: AsRef<str>>(self, items: &mut [S]) {
        self.sort_by_key(items, |item| item.as_ref());
    }

    /// Sort `items` in the collation order of the string that `key` returns for each one. The
    /// sort is stable.
    ///
    /// ```
    /// use mojibake::collate::Collator;
    ///
    /// let mut people = vec![("Émile", 1), ("Zoë", 2), ("eve", 3)];
    /// Collator::default().sort_by_key(&mut people, |person| person.0);
    /// assert_eq!(people, [("Émile", 1), ("eve", 3), ("Zoë", 2)]);
    /// ```
    ///
    /// # Panics
    ///
    /// If the library cannot allocate memory.
    pub fn sort_by_key<T>(self, items: &mut [T], mut key: impl FnMut(&T) -> &str) {
        items.sort_by_cached_key(|item| {
            self.sort_key(key(item))
                .unwrap_or_else(|error| panic!("mojibake: {error}"))
        });
    }
}

/// Compare two strings with the default [`Collator`].
///
/// # Panics
///
/// If the library cannot allocate memory. See [`Collator::try_compare`].
#[must_use]
pub fn compare(a: &str, b: &str) -> Ordering {
    Collator::default().compare(a, b)
}

/// A sort key with the default [`Collator`].
///
/// # Errors
///
/// [`Error::NoMemory`](crate::Error::NoMemory) if the library cannot allocate memory.
pub fn sort_key(s: &str) -> Result<SortKey> {
    Collator::default().sort_key(s)
}

/// Sort `items` with the default [`Collator`]. See [`Collator::sort`].
///
/// # Panics
///
/// If the library cannot allocate memory.
pub fn sort<S: AsRef<str>>(items: &mut [S]) {
    Collator::default().sort(items);
}
