// The Mojibake library
//
// This file is distributed under the MIT License. See LICENSE for details.

use std::fmt;
use std::str::FromStr;

use crate::case::{parse_tag, Locale};
use crate::error::{Error, Result};
use crate::ffi;

/// A BCP 47 language tag (RFC 5646), parsed strictly into its subtags.
///
/// The subtags are case-normalized: the language and the other subtags are lowercase, the script
/// is titlecase, and the region is uppercase. [`Display`](fmt::Display) writes the tag back in
/// that canonical case.
///
/// ```
/// use mojibake::LanguageTag;
///
/// let tag = LanguageTag::parse("SR-latn-rs-u-nu-latn")?;
/// assert_eq!(tag.language(), Some("sr"));
/// assert_eq!(tag.script(), Some("Latn"));
/// assert_eq!(tag.region(), Some("RS"));
/// assert_eq!(tag.extension('u'), Some("nu-latn"));
/// assert_eq!(tag.to_string(), "sr-Latn-RS-u-nu-latn");
/// # Ok::<(), mojibake::Error>(())
/// ```
#[derive(Clone, Debug, PartialEq, Eq, Hash)]
pub struct LanguageTag {
    language: String,
    extlang: String,
    script: String,
    region: String,
    variant: String,
    extensions: String,
    private_use: String,
    grandfathered: String,
    locale: Locale,
}

/// Split a `-`-joined field into its subtags; an empty field has none.
fn subtags(field: &str) -> impl Iterator<Item = &str> {
    field.split('-').filter(|subtag| !subtag.is_empty())
}

fn non_empty(field: &str) -> Option<&str> {
    (!field.is_empty()).then_some(field)
}

impl LanguageTag {
    /// Parse a well-formed BCP 47 language tag such as `"en-US"`, `"zh-Hant-TW"`,
    /// `"de-CH-1996"`, `"x-private"`, or the grandfathered `"i-klingon"`.
    ///
    /// Returns [`Error::InvalidArgument`] for a malformed tag, including a repeated variant or
    /// extension singleton, and for a tag too long for the C library's fixed-size fields (over 31
    /// bytes of variants or 127 bytes of extensions or private use).
    pub fn parse(tag: &str) -> Result<Self> {
        let id = parse_tag(tag)?;

        Ok(Self {
            language: ffi::array_str(&id.language),
            extlang: ffi::array_str(&id.extlang),
            script: ffi::array_str(&id.script),
            region: ffi::array_str(&id.region),
            variant: ffi::array_str(&id.variant),
            extensions: ffi::array_str(&id.extensions),
            private_use: ffi::array_str(&id.private_use),
            grandfathered: ffi::array_str(&id.grandfathered),
            locale: Locale(id.locale),
        })
    }

    /// The primary language subtag, such as `"en"`. `None` for a private-use-only or
    /// grandfathered tag.
    pub fn language(&self) -> Option<&str> {
        non_empty(&self.language)
    }

    /// The extended language subtags, such as `["yue"]` in `"zh-yue-HK"`.
    pub fn extlangs(&self) -> impl Iterator<Item = &str> {
        subtags(&self.extlang)
    }

    /// The ISO 15924 script subtag, such as `"Latn"`.
    pub fn script(&self) -> Option<&str> {
        non_empty(&self.script)
    }

    /// The region subtag: an ISO 3166-1 code such as `"US"`, or a UN M.49 code such as `"419"`.
    pub fn region(&self) -> Option<&str> {
        non_empty(&self.region)
    }

    /// The variant subtags, such as `["rozaj", "biske"]` in `"sl-rozaj-biske"`.
    pub fn variants(&self) -> impl Iterator<Item = &str> {
        subtags(&self.variant)
    }

    /// The extensions as `(singleton, value)` pairs, in tag order: `"en-u-ca-gregory-t-ja"` has
    /// `('u', "ca-gregory")` and `('t', "ja")`.
    pub fn extensions(&self) -> Extensions<'_> {
        Extensions {
            rest: &self.extensions,
        }
    }

    /// The value of the extension introduced by `singleton`, such as `"ca-gregory"` for `'u'`.
    /// The singleton is matched without regard to ASCII case.
    pub fn extension(&self, singleton: char) -> Option<&str> {
        let singleton = singleton.to_ascii_lowercase();

        self.extensions()
            .find(|&(s, _)| s == singleton)
            .map(|(_, value)| value)
    }

    /// The private-use subtags after `x`, such as `["phonebk"]` in `"de-x-phonebk"`.
    pub fn private_use(&self) -> impl Iterator<Item = &str> {
        subtags(&self.private_use).skip(1)
    }

    /// The whole tag if it is one of the grandfathered tags of RFC 5646, such as `"i-klingon"`,
    /// in lowercase.
    pub fn grandfathered(&self) -> Option<&str> {
        non_empty(&self.grandfathered)
    }

    /// The locale that a two-letter language subtag selects, such as `tr` for `"tr-TR"`, or
    /// [`Locale::ROOT`] for a language the library does not list. Pass it to the casing functions;
    /// a listed locale without casing rules, such as `en`, cases like [`Locale::ROOT`].
    pub fn locale(&self) -> Locale {
        self.locale
    }
}

impl FromStr for LanguageTag {
    type Err = Error;

    fn from_str(s: &str) -> Result<Self> {
        Self::parse(s)
    }
}

impl fmt::Display for LanguageTag {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        if let Some(grandfathered) = self.grandfathered() {
            return f.write_str(grandfathered);
        }

        let parts = [
            &self.language,
            &self.extlang,
            &self.script,
            &self.region,
            &self.variant,
            &self.extensions,
            &self.private_use,
        ];
        let mut first = true;

        for part in parts.into_iter().filter(|part| !part.is_empty()) {
            if !first {
                f.write_str("-")?;
            }

            f.write_str(part)?;
            first = false;
        }

        Ok(())
    }
}

/// The extensions of a [`LanguageTag`], from [`LanguageTag::extensions`].
#[derive(Clone, Debug)]
pub struct Extensions<'a> {
    rest: &'a str,
}

impl<'a> Iterator for Extensions<'a> {
    type Item = (char, &'a str);

    fn next(&mut self) -> Option<Self::Item> {
        // The field is "s-value-s-value". A singleton is the only one-character subtag, because
        // extension values are 2 to 8 characters long.
        let mut chars = self.rest.chars();
        let singleton = chars.next()?;
        let body = chars.as_str().strip_prefix('-').unwrap_or("");
        let end = body
            .match_indices('-')
            .map(|(i, _)| i)
            .find(|&i| body[i + 1..].split('-').next().map(str::len) == Some(1))
            .unwrap_or(body.len());

        self.rest = body[end..].strip_prefix('-').unwrap_or("");

        Some((singleton, &body[..end]))
    }
}

impl std::iter::FusedIterator for Extensions<'_> {}
