// The Mojibake library
//
// This file is distributed under the MIT License. See LICENSE for details.

use std::borrow::Cow;

use mojibake::*;

#[test]
fn versions() {
    assert_eq!(version(), env!("CARGO_PKG_VERSION"));
    assert!(unicode_version().starts_with("18."));
}

#[test]
fn normalization() {
    assert_eq!(normalize("e\u{301}", Form::Nfc).unwrap(), "\u{e9}");
    assert_eq!(normalize("\u{e9}", Form::Nfd).unwrap(), "e\u{301}");
    assert_eq!(normalize("ﬁ", Form::Nfkc).unwrap(), "fi");
    assert_eq!(normalize("", Form::Nfc).unwrap(), "");

    // Already-normalized input is borrowed, not copied.
    assert!(matches!(
        normalize("hello", Form::Nfc).unwrap(),
        Cow::Borrowed("hello")
    ));

    assert_eq!(is_normalized("abc", Form::Nfc).unwrap(), QuickCheck::Yes);
    assert_eq!(is_normalized("\u{e9}", Form::Nfd).unwrap(), QuickCheck::No);
    assert_eq!(nfkc_casefold("ＡＢＣ").unwrap(), "abc");
}

#[test]
fn casing() {
    assert_eq!(to_uppercase("straße", Locale::ROOT).unwrap(), "STRASSE");
    assert_eq!(to_lowercase("ΣΑΣ", Locale::ROOT).unwrap(), "σας");

    let turkish = Locale::parse("tr").unwrap();
    assert_eq!(turkish.code(), Some("tr"));
    assert_eq!("istanbul".to_uppercase_in(turkish).unwrap(), "İSTANBUL");
    assert_eq!("I".to_lowercase_in(turkish).unwrap(), "ı");
    assert_eq!(Locale::parse("en-US").unwrap().code(), Some("en"));
    assert!(Locale::parse("not a tag!").is_err());

    assert_eq!(
        to_titlecase("hello world", Locale::ROOT).unwrap(),
        "Hello World"
    );
    assert_eq!(casefold("Straße").unwrap(), "strasse");
    assert_eq!(casefold_simple("Straße").unwrap(), "straße");

    assert!("Straße".caseless_eq("STRASSE"));
    assert!(caseless_eq("ǅ", "ǆ", CaselessMode::Canonical));
    assert!(caseless_eq("ﬁ", "FI", CaselessMode::Compatibility));
    assert!(!"abc".caseless_eq("abd"));
}

#[test]
fn segmentation() {
    let family = "👨\u{200d}👩\u{200d}👧";
    let text = format!("{family}e\u{301}x");
    assert_eq!(
        text.graphemes().collect::<Vec<_>>(),
        [family, "e\u{301}", "x"]
    );
    assert_eq!(grapheme_count(&text), 3);
    assert_eq!("".graphemes().count(), 0);
    assert_eq!("a".graphemes().collect::<Vec<_>>(), ["a"]);

    assert_eq!(
        "Hello, world!".words().collect::<Vec<_>>(),
        ["Hello", ",", " ", "world", "!"]
    );
    assert_eq!(word_count("Hello, world!"), 2);

    assert_eq!(
        "One. Two? Three".sentences().collect::<Vec<_>>(),
        ["One. ", "Two? ", "Three"]
    );
    assert_eq!(sentence_count("One. Two? Three"), 3);

    // Segments always reassemble the input.
    let mixed = "Ciao 世界! 🇮🇹 ok.\nNext line";
    assert_eq!(mixed.graphemes().collect::<String>(), mixed);
    assert_eq!(mixed.words().collect::<String>(), mixed);
    assert_eq!(mixed.sentences().collect::<String>(), mixed);
}

#[test]
fn line_breaking() {
    let breaks: Vec<_> = "a b\nc".line_breaks().collect();
    assert_eq!(
        breaks,
        [
            LineBreak::Allowed(2),
            LineBreak::Mandatory(4),
            LineBreak::Mandatory(5)
        ]
    );
}

#[test]
fn width() {
    assert_eq!("abc".terminal_width(), 3);
    assert_eq!("日本".terminal_width(), 4);
    assert_eq!("e\u{301}".terminal_width(), 1);
    assert_eq!(terminal_width("±", WidthProfile::EastAsian), 2);
    assert_eq!("日本語".truncate_to_width(5), "日本");
    assert_eq!("e\u{301}e\u{301}".truncate_to_width(1), "e\u{301}");
}

#[test]
fn characters() {
    assert_eq!('A'.general_category(), GeneralCategory::UppercaseLetter);
    assert_eq!('€'.general_category().abbreviation(), "Sc");
    assert_eq!('\u{378}'.general_category(), GeneralCategory::Unassigned);
    assert_eq!('A'.block().as_deref(), Some("Basic Latin"));
    assert_eq!('A'.script(), Some("Latn"));
    assert_eq!('Ж'.script(), Some("Cyrl"));
    assert_eq!('Ⅻ'.numeric_value(), Some(12.0));
    assert_eq!('½'.numeric_value(), Some(0.5));
    assert_eq!('A'.numeric_value(), None);
    assert_eq!('日'.east_asian_width(), Some(EastAsianWidth::Wide));
    assert_eq!('a'.east_asian_width(), Some(EastAsianWidth::Narrow));
    assert!('😀'.emoji_properties().presentation);
    assert!(!'a'.emoji_properties().emoji);
    assert!('\u{301}'.is_combining());
    assert!('a'.is_xid_start() && !'1'.is_xid_start() && '1'.is_xid_continue());

    #[cfg(feature = "names")]
    {
        assert_eq!(
            'é'.name().as_deref(),
            Some("LATIN SMALL LETTER E WITH ACUTE")
        );
        assert_eq!('가'.name().as_deref(), Some("HANGUL SYLLABLE GA"));
    }
}

#[cfg(feature = "collation")]
#[test]
fn collation() {
    use mojibake::collate::{self, Collator, Strength};
    use std::cmp::Ordering;

    assert_eq!(collate::compare("a", "B"), Ordering::Less);
    assert_eq!(collate::compare("résumé", "resume"), Ordering::Greater);

    let primary = Collator {
        strength: Strength::Primary,
        ..Collator::default()
    };
    assert_eq!(primary.compare("résumé", "Resume"), Ordering::Equal);

    // Accents and case only break ties: "Äpfel" sorts as "apfel", before "apple".
    let mut words = vec!["zebra", "apple", "Zoo", "Äpfel", "Apple"];
    words.sort_by(|a, b| collate::compare(a, b));
    assert_eq!(words, ["Äpfel", "apple", "Apple", "zebra", "Zoo"]);

    let keys: Vec<_> = ["b", "a", "c"]
        .iter()
        .map(|s| collate::sort_key(s).unwrap())
        .collect();
    assert!(keys[1] < keys[0] && keys[0] < keys[2]);
}

#[cfg(feature = "idna")]
#[test]
fn idna() {
    assert_eq!(
        idna::to_ascii("bücher.de").unwrap().unwrap(),
        "xn--bcher-kva.de"
    );
    assert_eq!(
        idna::to_unicode("xn--bcher-kva.de").unwrap().unwrap(),
        "bücher.de"
    );
    assert_eq!(
        idna::to_ascii("Example.COM").unwrap().unwrap(),
        "example.com"
    );
    assert!(idna::to_ascii("xn--a.com").unwrap().is_err());
}

#[cfg(feature = "security")]
#[test]
fn security() {
    assert!(security::is_confusable("paypal", "p\u{430}ypal"));
    assert!(!security::is_confusable("paypal", "paypa1x"));
    assert_eq!(
        security::skeleton("p\u{430}ypal").unwrap(),
        security::skeleton("paypal").unwrap()
    );
}

#[test]
fn errors() {
    let error = Error::NoMemory;
    assert!(!error.to_string().is_empty());
}
