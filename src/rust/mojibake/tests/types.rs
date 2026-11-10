// The Mojibake library
//
// This file is distributed under the MIT License. See LICENSE for details.

//! Typed property values, string conversions, fallible variants, and collation helpers.

use mojibake::*;

#[test]
fn scripts() {
    let latin = Script::from_code("Latn").unwrap();

    assert_eq!(Script::from_code("LATN"), Some(latin));
    assert_eq!("latn".parse::<Script>(), Ok(latin));
    assert_eq!("Xxxx".parse::<Script>(), Err(Error::NotFound));
    assert_eq!(Script::from_code(""), None);
    assert_eq!(latin.to_string(), "Latn");
    assert_eq!(format!("{latin:?}"), "Script(Latn)");

    assert_eq!(Script::COMMON.code(), "Zyyy");
    assert_eq!(Script::INHERITED.code(), "Zinh");
    assert_eq!(Script::UNKNOWN.code(), "Zzzz");
    assert_eq!('1'.script(), Some(Script::COMMON));
    assert_eq!('\u{301}'.script(), Some(Script::INHERITED));
    assert_eq!('\u{378}'.script(), Some(Script::UNKNOWN));

    // Every script has a distinct four-letter code; the last three are writing systems.
    let all: Vec<Script> = Script::all().collect();
    let writing_systems: Vec<&str> = all
        .iter()
        .filter(|script| !script.is_property_value())
        .map(|script| script.code())
        .collect();

    assert!(all.iter().all(|script| script.code().len() == 4));
    assert!(all
        .iter()
        .all(|&script| Script::from_code(script.code()) == Some(script)));
    assert_eq!(writing_systems, ["Hanb", "Jpan", "Kore"]);
}

#[test]
fn blocks() {
    let latin = 'A'.block().unwrap();

    assert_eq!(latin.name(), "Basic Latin");
    assert_eq!(latin.codepoints(), 0..=0x7F);
    assert!(latin.contains('\u{7f}') && !latin.contains('\u{80}'));
    assert_eq!(Block::of('z'), Some(latin));
    assert_ne!(Block::of('é'), Some(latin));
    assert_eq!(latin.to_string(), "Basic Latin");
    assert_eq!(
        format!("{latin:?}"),
        r#"Block("Basic Latin", U+0000..=U+007F)"#
    );
    assert_eq!('😀'.block().unwrap().name(), "Emoticons");

    // A name is the same on every lookup.
    assert!(std::ptr::eq(latin.name(), Block::of('B').unwrap().name()));
}

#[test]
fn numeric_values() {
    let twelve = 'Ⅻ'.numeric_value().unwrap();
    assert_eq!(twelve.numeric_type, NumericType::Numeric);
    assert_eq!(twelve.as_integer(), Some(12));
    assert_eq!(twelve.to_string(), "12");

    let digit = '²'.numeric_value().unwrap();
    assert_eq!(digit.numeric_type, NumericType::Digit);
    assert_eq!(digit.as_integer(), Some(2));

    // U+0F33 TIBETAN DIGIT HALF ZERO is -1/2.
    let negative = '\u{f33}'.numeric_value().unwrap();
    assert_eq!((negative.numerator, negative.denominator), (-1, 2));
    assert_eq!(negative.as_integer(), None);
    assert_eq!(negative.as_f64(), -0.5);
    assert_eq!(negative.to_string(), "-1/2");

    // A fraction that f64 cannot represent exactly keeps its exact value.
    let sixtieth = '\u{109f6}'.numeric_value().unwrap();
    assert_eq!(sixtieth.to_string(), "1/12");
}

#[test]
fn string_conversions() {
    assert_eq!(GeneralCategory::UppercaseLetter.to_string(), "Lu");
    assert_eq!(GeneralCategory::UppercaseLetter.name(), "Uppercase_Letter");
    assert_eq!("Lu".parse(), Ok(GeneralCategory::UppercaseLetter));
    assert_eq!(
        "uppercase letter".parse(),
        Ok(GeneralCategory::UppercaseLetter)
    );
    assert_eq!("Private-Use".parse(), Ok(GeneralCategory::PrivateUse));
    assert_eq!("lu".parse::<GeneralCategory>(), Err(Error::NotFound));

    assert_eq!(Form::Nfkc.to_string(), "NFKC");
    assert_eq!("nfd".parse(), Ok(Form::Nfd));
    assert_eq!("NFX".parse::<Form>(), Err(Error::InvalidForm));

    let turkish: Locale = "tr-TR".parse().unwrap();
    assert_eq!(turkish, Locale::parse("tr").unwrap());
    assert_eq!(turkish.to_string(), "tr");
    assert_eq!(Locale::ROOT.to_string(), "und");
    assert_eq!("und".parse(), Ok(Locale::ROOT));
    assert!("not a tag!".parse::<Locale>().is_err());
}

#[test]
fn fallible_variants() {
    assert_eq!(
        try_caseless_eq("Straße", "STRASSE", CaselessMode::Canonical),
        Ok(true)
    );
    assert_eq!(
        try_caseless_eq("a", "b", CaselessMode::Canonical),
        Ok(false)
    );

    #[cfg(feature = "security")]
    {
        use mojibake::security::{is_confusable, try_is_confusable};

        assert_eq!(try_is_confusable("paypal", "p\u{430}ypal"), Ok(true));
        assert!(!is_confusable("paypal", "google"));
    }
}

#[cfg(feature = "collation")]
#[test]
fn collation() {
    use std::cmp::Ordering;

    use mojibake::collate::{self, Collator, Strength};

    assert_eq!(collate::compare("a", "b"), Ordering::Less);
    assert_eq!(
        Collator::default().try_compare("b", "a"),
        Ok(Ordering::Greater)
    );

    // Keys order like compare, and are plain bytes underneath.
    let words = [
        "peach",
        "péché",
        "pêche",
        "Péché",
        "pecher",
        "",
        "zebra",
        "Ångström",
    ];

    for a in words {
        for b in words {
            let (ka, kb) = (collate::sort_key(a).unwrap(), collate::sort_key(b).unwrap());
            assert_eq!(ka.cmp(&kb), collate::compare(a, b), "{a:?} vs {b:?}");
        }
    }

    let key = collate::sort_key("abc").unwrap();
    assert_eq!(key.as_bytes(), key.clone().into_bytes());

    let mut sorted = words.to_vec();
    collate::sort(&mut sorted);
    assert!(sorted
        .windows(2)
        .all(|pair| collate::compare(pair[0], pair[1]) != Ordering::Greater));

    // Owned strings sort too, and a primary-strength sort is stable on equal items.
    let mut owned = vec![String::from("b"), String::from("A"), String::from("a")];
    Collator {
        strength: Strength::Primary,
        ..Collator::default()
    }
    .sort(&mut owned);
    assert_eq!(owned, ["A", "a", "b"]);

    let mut pairs = vec![(2, "beta"), (1, "Alpha"), (3, "alpha")];
    Collator::default().sort_by_key(&mut pairs, |pair| pair.1);
    assert_eq!(pairs, [(3, "alpha"), (1, "Alpha"), (2, "beta")]);
}

#[cfg(feature = "serde")]
#[test]
fn serde_as_strings() {
    use serde::de::value::{Error as DeError, StrDeserializer};
    use serde::de::{Deserialize, IntoDeserializer};

    fn parse<'de, T: Deserialize<'de>>(s: &'de str) -> std::result::Result<T, DeError> {
        let deserializer: StrDeserializer<'de, DeError> = s.into_deserializer();
        T::deserialize(deserializer)
    }

    assert_eq!(parse::<Form>("NFC"), Ok(Form::Nfc));
    assert_eq!(parse::<Locale>("tr"), Ok(Locale::parse("tr").unwrap()));
    assert_eq!(
        parse::<LanguageTag>("sr-Latn-RS").map(|tag| tag.to_string()),
        Ok(String::from("sr-Latn-RS"))
    );
    assert_eq!(
        parse::<GeneralCategory>("Lu"),
        Ok(GeneralCategory::UppercaseLetter)
    );
    assert_eq!(parse::<Script>("Latn").map(Script::code), Ok("Latn"));

    let error = parse::<Script>("Xxxx").unwrap_err().to_string();
    assert!(error.contains("ISO 15924"), "{error}");
}
