// The Mojibake library
//
// This file is distributed under the MIT License. See LICENSE for details.

use mojibake::{Error, LanguageTag, Locale};

fn parse(tag: &str) -> LanguageTag {
    LanguageTag::parse(tag).unwrap_or_else(|error| panic!("{tag}: {error}"))
}

fn all<'a>(iter: impl Iterator<Item = &'a str>) -> Vec<&'a str> {
    iter.collect()
}

#[test]
fn subtags() {
    let tag = parse("sr-Latn-RS");
    assert_eq!(tag.language(), Some("sr"));
    assert_eq!(tag.script(), Some("Latn"));
    assert_eq!(tag.region(), Some("RS"));
    assert_eq!(all(tag.variants()), Vec::<&str>::new());
    assert_eq!(tag.extensions().count(), 0);
    assert_eq!(all(tag.private_use()), Vec::<&str>::new());
    assert_eq!(tag.grandfathered(), None);

    let tag = parse("zh-yue-HK");
    assert_eq!(all(tag.extlangs()), ["yue"]);
    assert_eq!(tag.region(), Some("HK"));

    assert_eq!(parse("es-419").region(), Some("419"));
    assert_eq!(all(parse("sl-rozaj-biske").variants()), ["rozaj", "biske"]);
    assert_eq!(all(parse("de-CH-1996").variants()), ["1996"]);
    assert_eq!(all(parse("de-x-phonebk").private_use()), ["phonebk"]);
}

#[test]
fn case_is_normalized() {
    let tag = parse("SR-latn-rs");
    assert_eq!(tag.language(), Some("sr"));
    assert_eq!(tag.script(), Some("Latn"));
    assert_eq!(tag.region(), Some("RS"));
    assert_eq!(tag.to_string(), "sr-Latn-RS");
    assert_eq!(parse("EN-us-X-Foo").to_string(), "en-US-x-foo");
}

#[test]
fn extensions() {
    let tag = parse("en-u-ca-gregory-nu-latn-t-ja-x-private");
    let extensions: Vec<_> = tag.extensions().collect();
    assert_eq!(extensions, [('u', "ca-gregory-nu-latn"), ('t', "ja")]);
    assert_eq!(tag.extension('u'), Some("ca-gregory-nu-latn"));
    assert_eq!(tag.extension('U'), Some("ca-gregory-nu-latn"));
    assert_eq!(tag.extension('a'), None);
    assert_eq!(all(tag.private_use()), ["private"]);

    // Single-subtag values next to each other.
    let tag = parse("de-a-aa-b-bb-c-cc");
    let extensions: Vec<_> = tag.extensions().collect();
    assert_eq!(extensions, [('a', "aa"), ('b', "bb"), ('c', "cc")]);
}

#[test]
fn private_use_and_grandfathered() {
    let tag = parse("x-whatever-else");
    assert_eq!(tag.language(), None);
    assert_eq!(all(tag.private_use()), ["whatever", "else"]);
    assert_eq!(tag.to_string(), "x-whatever-else");

    for grandfathered in ["i-klingon", "en-GB-oed", "zh-min-nan"] {
        let tag = parse(grandfathered);
        assert_eq!(
            tag.grandfathered(),
            Some(&*grandfathered.to_ascii_lowercase())
        );
        assert_eq!(tag.language(), None);
        assert_eq!(tag.to_string(), grandfathered.to_ascii_lowercase());
    }
}

#[test]
fn round_trip() {
    for tag in [
        "en",
        "zh-Hant-TW",
        "zh-yue-HK",
        "sl-rozaj-biske",
        "es-419",
        "en-u-ca-gregory-t-ja-x-a-b",
        "x-whatever",
        "i-klingon",
        "qaa-Qaaa-QM-x-southern",
    ] {
        let parsed = parse(tag);
        assert_eq!(parsed.to_string(), tag);
        assert_eq!(parse(&parsed.to_string()), parsed);
        assert_eq!(tag.parse::<LanguageTag>().unwrap(), parsed);
    }
}

#[test]
fn locales() {
    assert_eq!(parse("tr-TR").locale(), Locale::parse("tr").unwrap());
    assert_eq!(parse("tr-TR").locale().code(), Some("tr"));
    assert_eq!(parse("lt").locale().code(), Some("lt"));
    assert_eq!(parse("en-US").locale().code(), Some("en"));
    // Only two-letter language subtags select a locale.
    assert_eq!(parse("tur").locale(), Locale::ROOT);
    assert_eq!(parse("x-tr").locale(), Locale::ROOT);
}

#[test]
fn rejected() {
    for tag in [
        "",
        "-",
        "en-",
        "en--US",
        "a",
        "123",
        "toolonglanguage",
        "en-US-",
        "de-419-DE",
        "sl-rozaj-rozaj",
        "ar-a-aaa-b-bbb-a-ccc",
        "en-a",
        "en-x",
        "x",
        "en US",
        "ünicode",
    ] {
        assert_eq!(
            LanguageTag::parse(tag),
            Err(Error::InvalidArgument),
            "{tag:?}"
        );
    }

    // Each field of the C structure has a fixed size.
    let variants = ["aaaaa", "bbbbb", "ccccc", "ddddd", "eeeee", "fffff"].join("-");
    assert!(LanguageTag::parse(&format!("en-{variants}")).is_err());
    let long = vec!["abcdefgh"; 16].join("-");
    assert!(LanguageTag::parse(&format!("en-u-{long}")).is_err());
}
