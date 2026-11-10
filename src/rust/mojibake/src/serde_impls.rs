// The Mojibake library
//
// This file is distributed under the MIT License. See LICENSE for details.

//! `serde` support, behind the `serde` feature. Every type is written as the string its
//! `Display` implementation produces and read back with its `FromStr` implementation.

use std::fmt;
use std::marker::PhantomData;
use std::str::FromStr;

use serde::de::{self, Deserialize, Deserializer, Visitor};
use serde::ser::{Serialize, Serializer};

use crate::{Form, GeneralCategory, LanguageTag, Locale, Script};

struct FromStrVisitor<T>(PhantomData<T>, &'static str);

impl<T> Visitor<'_> for FromStrVisitor<T>
where
    T: FromStr,
    T::Err: fmt::Display,
{
    type Value = T;

    fn expecting(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str(self.1)
    }

    fn visit_str<E: de::Error>(self, value: &str) -> Result<T, E> {
        value
            .parse()
            .map_err(|error| E::custom(format_args!("invalid {}: {value:?}: {error}", self.1)))
    }
}

macro_rules! serde_as_string {
    ($($ty:ty => $expecting:literal),* $(,)?) => {
        $(
            impl Serialize for $ty {
                fn serialize<S: Serializer>(&self, serializer: S) -> Result<S::Ok, S::Error> {
                    serializer.collect_str(self)
                }
            }

            impl<'de> Deserialize<'de> for $ty {
                fn deserialize<D: Deserializer<'de>>(deserializer: D) -> Result<Self, D::Error> {
                    deserializer.deserialize_str(FromStrVisitor(PhantomData, $expecting))
                }
            }
        )*
    };
}

serde_as_string!(
    Form => "a normalization form such as \"NFC\"",
    Locale => "a BCP 47 language tag",
    LanguageTag => "a BCP 47 language tag",
    GeneralCategory => "a General_Category value such as \"Lu\"",
    Script => "an ISO 15924 script code such as \"Latn\"",
);
