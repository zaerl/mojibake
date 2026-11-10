// The Mojibake library
//
// This file is distributed under the MIT License. See LICENSE for details.

use std::borrow::Cow;

use crate::{
    case, normalize, segment, width, CaselessMode, Form, LineBreaks, Locale, Result, Segments,
    WidthProfile,
};

/// Unicode string algorithms that `str` does not provide.
///
/// Every method is also available as a free function.
pub trait StrExt {
    /// See [`normalize()`](crate::normalize()).
    fn normalize(&self, form: Form) -> Result<Cow<'_, str>>;
    /// See [`is_normalized`](crate::is_normalized). [`Form::quick_check`] is the faster,
    /// three-valued check.
    fn is_normalized(&self, form: Form) -> Result<bool>;
    /// See [`to_uppercase`](crate::to_uppercase).
    fn to_uppercase_in(&self, locale: Locale) -> Result<Cow<'_, str>>;
    /// See [`to_lowercase`](crate::to_lowercase).
    fn to_lowercase_in(&self, locale: Locale) -> Result<Cow<'_, str>>;
    /// See [`to_titlecase`](crate::to_titlecase).
    fn to_titlecase_in(&self, locale: Locale) -> Result<Cow<'_, str>>;
    /// See [`casefold`](crate::casefold).
    fn casefold(&self) -> Result<Cow<'_, str>>;
    /// Canonical caseless match (D145). See [`caseless_eq`](crate::caseless_eq).
    fn caseless_eq(&self, other: &str) -> bool;
    /// See [`graphemes`](crate::graphemes).
    fn graphemes(&self) -> Segments<'_>;
    /// See [`words`](crate::words).
    fn words(&self) -> Segments<'_>;
    /// See [`sentences`](crate::sentences).
    fn sentences(&self) -> Segments<'_>;
    /// See [`line_breaks`](crate::line_breaks).
    fn line_breaks(&self) -> LineBreaks<'_>;
    /// Terminal width with the narrow profile. See [`terminal_width`](crate::terminal_width).
    fn terminal_width(&self) -> usize;
    /// Truncate with the narrow profile. See [`truncate_to_width`](crate::truncate_to_width).
    fn truncate_to_width(&self, max_columns: usize) -> &str;
}

impl StrExt for str {
    fn normalize(&self, form: Form) -> Result<Cow<'_, str>> {
        normalize::normalize(self, form)
    }

    fn is_normalized(&self, form: Form) -> Result<bool> {
        normalize::is_normalized(self, form)
    }

    fn to_uppercase_in(&self, locale: Locale) -> Result<Cow<'_, str>> {
        case::to_uppercase(self, locale)
    }

    fn to_lowercase_in(&self, locale: Locale) -> Result<Cow<'_, str>> {
        case::to_lowercase(self, locale)
    }

    fn to_titlecase_in(&self, locale: Locale) -> Result<Cow<'_, str>> {
        case::to_titlecase(self, locale)
    }

    fn casefold(&self) -> Result<Cow<'_, str>> {
        case::casefold(self)
    }

    fn caseless_eq(&self, other: &str) -> bool {
        case::caseless_eq(self, other, CaselessMode::Canonical)
    }

    fn graphemes(&self) -> Segments<'_> {
        segment::graphemes(self)
    }

    fn words(&self) -> Segments<'_> {
        segment::words(self)
    }

    fn sentences(&self) -> Segments<'_> {
        segment::sentences(self)
    }

    fn line_breaks(&self) -> LineBreaks<'_> {
        segment::line_breaks(self)
    }

    fn terminal_width(&self) -> usize {
        width::terminal_width(self, WidthProfile::Narrow)
    }

    fn truncate_to_width(&self, max_columns: usize) -> &str {
        width::truncate_to_width(self, WidthProfile::Narrow, max_columns)
    }
}
