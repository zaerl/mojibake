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

#[test]
fn character_records() {
    use mojibake::bidi::BidiClass;

    assert_eq!('\u{301}'.canonical_combining_class(), 230);
    assert_eq!('\u{327}'.canonical_combining_class(), 202);
    assert_eq!('a'.canonical_combining_class(), 0);
    assert_eq!('\u{378}'.canonical_combining_class(), 0);

    for (c, class) in [
        ('a', BidiClass::LeftToRight),
        ('\u{5d0}', BidiClass::RightToLeft),
        ('\u{628}', BidiClass::ArabicLetter),
        ('1', BidiClass::EuropeanNumber),
        ('\u{664}', BidiClass::ArabicNumber),
        (' ', BidiClass::WhiteSpace),
        ('\t', BidiClass::SegmentSeparator),
        ('\u{2029}', BidiClass::ParagraphSeparator),
        ('\u{301}', BidiClass::NonspacingMark),
        ('!', BidiClass::OtherNeutral),
        ('\u{2067}', BidiClass::RightToLeftIsolate),
    ] {
        assert_eq!(c.bidi_class(), Some(class), "{c:?}");
    }
    assert_eq!('\u{378}'.bidi_class(), None);
    assert_eq!(BidiClass::ArabicLetter.abbreviation(), "AL");
    assert!(BidiClass::ArabicLetter.is_strong() && !BidiClass::EuropeanNumber.is_strong());

    assert!('('.is_bidi_mirrored() && '<'.is_bidi_mirrored() && !'a'.is_bidi_mirrored());

    assert_eq!('é'.decomposition_type(), Some(DecompositionType::Canonical));
    assert_eq!('ﬁ'.decomposition_type(), Some(DecompositionType::Compat));
    assert_eq!('²'.decomposition_type(), Some(DecompositionType::Super));
    assert_eq!('①'.decomposition_type(), Some(DecompositionType::Circle));
    assert_eq!('Ａ'.decomposition_type(), Some(DecompositionType::Wide));
    assert_eq!('a'.decomposition_type(), None);
    assert!(DecompositionType::Canonical.is_canonical() && !DecompositionType::Wide.is_canonical());

    assert_eq!('a'.simple_uppercase(), 'A');
    assert_eq!('A'.simple_lowercase(), 'a');
    assert_eq!('a'.simple_titlecase(), 'A');
    assert_eq!('\u{1c6}'.simple_titlecase(), '\u{1c5}');
    assert_eq!('\u{1c6}'.simple_uppercase(), '\u{1c4}');
    // ß has no simple uppercase mapping; its full mapping is "SS".
    assert_eq!('ß'.simple_uppercase(), 'ß');
    assert_eq!('1'.simple_uppercase(), '1');
    assert_eq!('\u{378}'.simple_lowercase(), '\u{378}');

    let four = '\u{664}'.info().unwrap();
    assert_eq!(four.general_category, GeneralCategory::DecimalNumber);
    assert_eq!(four.decimal_value, Some(4));
    assert_eq!(four.digit_value, Some(4));
    assert_eq!(four.numeric_value, Some(4.0));

    let two = '²'.info().unwrap();
    assert_eq!(two.decimal_value, None);
    assert_eq!(two.digit_value, Some(2));

    let half = '½'.info().unwrap();
    assert_eq!((half.decimal_value, half.digit_value), (None, None));
    assert_eq!(half.numeric_value, Some(0.5));

    assert!('\u{378}'.info().is_none());
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

    use idna::IdnaErrors;

    let error = idna::to_ascii("a..b").unwrap().unwrap_err();
    assert!(error.errors.contains(IdnaErrors::EMPTY_LABEL));
    assert!(!error.errors.contains(IdnaErrors::BIDI));

    let error = idna::to_ascii("-a.com").unwrap().unwrap_err();
    assert_eq!(error.errors, IdnaErrors::HYPHEN);
    assert_eq!(format!("{:?}", error.errors), "IdnaErrors(HYPHEN)");
    assert_eq!(
        error.to_string(),
        "IDNA processing failed: IdnaErrors(HYPHEN)"
    );

    let long = "a".repeat(64);
    let error = idna::to_ascii(&format!("{long}.com")).unwrap().unwrap_err();
    assert!(error.errors.contains(IdnaErrors::LABEL_LENGTH));
    // Lengths are only checked when converting to ASCII.
    assert!(idna::to_unicode(&format!("{long}.com")).unwrap().is_ok());

    let both = IdnaErrors::HYPHEN | IdnaErrors::STD3;
    assert_eq!(
        both.iter().collect::<Vec<_>>(),
        [IdnaErrors::HYPHEN, IdnaErrors::STD3]
    );
    assert_eq!(format!("{both:?}"), "IdnaErrors(HYPHEN | STD3)");
    assert_eq!(both.bits(), 0x44);
    assert!(IdnaErrors::NONE.is_empty() && IdnaErrors::default().is_empty());
    assert_eq!(format!("{:?}", IdnaErrors::NONE), "IdnaErrors(NONE)");
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
