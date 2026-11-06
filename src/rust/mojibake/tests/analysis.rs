// The Mojibake library
//
// This file is distributed under the MIT License. See LICENSE for details.

//! Filters, identifiers, emoji sequences, scripts, and generic properties.

use std::borrow::Cow;
use std::collections::HashSet;

use mojibake::emoji::{self, Qualification, SequenceKind};
use mojibake::*;

#[test]
fn filters() {
    assert_eq!(
        filter("a \t\u{3000} b", Filter::COLLAPSE_SPACES).unwrap(),
        "a b"
    );
    assert_eq!(filter("a\tb", Filter::SPACES).unwrap(), "a b");
    assert_eq!(filter("a\u{7}b\n", Filter::CONTROLS).unwrap(), "ab\n");
    assert_eq!(filter("٤٢", Filter::NUMERIC).unwrap(), "42");
    assert_eq!(filter("e\u{301}", Filter::NORMALIZE).unwrap(), "\u{e9}");
    assert_eq!(
        filter(
            "e\u{301}\u{7} \t x",
            Filter::NORMALIZE | Filter::CONTROLS | Filter::COLLAPSE_SPACES
        )
        .unwrap(),
        "\u{e9} x"
    );

    // Zalgo text keeps at most four marks per run.
    let zalgo = format!("a{}", "\u{301}".repeat(10));
    assert_eq!(
        filter(&zalgo, Filter::LIMIT_COMBINING).unwrap(),
        format!("a{}", "\u{301}".repeat(4))
    );

    // Unchanged input is borrowed.
    assert!(matches!(
        filter("hello", Filter::COLLAPSE_SPACES).unwrap(),
        Cow::Borrowed("hello")
    ));
    assert!(matches!(
        filter("hello", Filter::NONE).unwrap(),
        Cow::Borrowed("hello")
    ));
    assert_eq!(filter("", Filter::COLLAPSE_SPACES).unwrap(), "");

    let mut flags = Filter::SPACES;
    flags |= Filter::NUMERIC;
    assert!(flags.contains(Filter::NUMERIC) && !flags.contains(Filter::CONTROLS));
    assert!(Filter::NONE.is_empty() && Filter::default().is_empty());
    assert_eq!(format!("{flags:?}"), "Filter(SPACES | NUMERIC)");
    assert_eq!(format!("{:?}", Filter::NONE), "Filter(NONE)");
}

#[test]
fn identifiers() {
    for profile in [IdentifierProfile::Default, IdentifierProfile::Nfkc] {
        assert!(is_identifier("größe_1", profile));
        assert!(is_identifier("переменная", profile));
        assert!(!is_identifier("1st", profile));
        assert!(!is_identifier("a b", profile));
        assert!(!is_identifier("", profile));
    }

    // U+309B KATAKANA-HIRAGANA VOICED SOUND MARK is ID_Continue but not XID_Continue.
    assert!(is_identifier("a\u{309b}", IdentifierProfile::Default));
    assert!(!is_identifier("a\u{309b}", IdentifierProfile::Nfkc));

    assert!('a'.is_id_start() && !'_'.is_id_start() && '_'.is_id_continue());
    assert!('\u{309b}'.is_id_continue() && !'\u{309b}'.is_xid_continue());
    assert!('+'.is_pattern_syntax() && !'a'.is_pattern_syntax());
    assert!('\n'.is_pattern_white_space() && !'\u{3000}'.is_pattern_white_space());
}

#[test]
fn emoji_sequences() {
    let flag = emoji::sequence("🇮🇹").unwrap();
    assert_eq!(flag.kind, Some(SequenceKind::Flag));
    assert_eq!(flag.qualification, Qualification::FullyQualified);
    assert_eq!(flag.codepoint_count, 2);

    assert_eq!(
        emoji::sequence("1\u{fe0f}\u{20e3}").unwrap().kind,
        Some(SequenceKind::Keycap)
    );
    assert_eq!(
        emoji::sequence("👍🏽").unwrap().kind,
        Some(SequenceKind::Modifier)
    );
    assert_eq!(emoji::sequence("👨‍👩‍👧").unwrap().kind, Some(SequenceKind::Zwj));
    assert_eq!(
        emoji::sequence("😀").unwrap().kind,
        Some(SequenceKind::Basic)
    );
    assert_eq!(
        emoji::sequence("\u{263a}\u{fe0e}").unwrap().kind,
        Some(SequenceKind::TextVariation)
    );
    // Listed in emoji-test.txt only: no kind, but a qualification.
    let unqualified = emoji::sequence("\u{263a}").unwrap();
    assert_eq!(unqualified.kind, None);
    assert_eq!(unqualified.qualification, Qualification::Unqualified);
    assert!(!emoji::is_rgi("\u{263a}"));
    assert_eq!(
        emoji::sequence("\u{263a}\u{fe0f}").unwrap().kind,
        Some(SequenceKind::Basic)
    );
    assert!(emoji::sequence("ab").is_none());
    assert!(emoji::sequence("😀😀").is_none());
    assert!(emoji::sequence("").is_none());

    assert!(emoji::is_sequence("🇮🇹") && emoji::is_rgi("🇮🇹"));
    assert!(emoji::is_sequence("\u{263a}\u{fe0e}") && !emoji::is_rgi("\u{263a}\u{fe0e}"));
    assert!(!emoji::is_sequence("a") && !emoji::is_rgi("a"));
}

#[test]
fn script_extensions() {
    assert_eq!('a'.script_extensions(), ["Latn"]);
    assert_eq!('ー'.script_extensions(), ["Hira", "Kana"]);
    assert_eq!('1'.script_extensions(), ["Zyyy"]);
}

#[test]
fn properties() {
    let all: Vec<Property> = Property::all().collect();
    assert!(all.len() > 100);

    // Every name is distinct under loose matching, so from_name finds each property.
    let names: HashSet<String> = all
        .iter()
        .map(|p| p.name().to_ascii_lowercase().replace('_', ""))
        .collect();
    assert_eq!(names.len(), all.len());

    for property in &all {
        assert_eq!(Property::from_name(property.name()), Some(*property));
    }

    let alphabetic = Property::from_name("Alphabetic").unwrap();
    assert_eq!(alphabetic.name(), "Alphabetic");
    assert_eq!(alphabetic.to_string(), "Alphabetic");
    assert_eq!(format!("{alphabetic:?}"), "Property(Alphabetic)");
    assert!(alphabetic.is_binary());
    assert!('a'.has_property(alphabetic) && !'1'.has_property(alphabetic));
    assert_eq!('a'.property_value(alphabetic), None);

    let white_space = Property::from_name("White_Space").unwrap();
    assert_eq!(Property::from_name("white space"), Some(white_space));
    assert_eq!(Property::from_name("WHITE-SPACE"), Some(white_space));
    assert!('\u{3000}'.has_property(white_space));
    assert!(Property::from_name("WSpace").is_none());
    assert!(Property::from_name("").is_none());
    assert!(Property::from_name("Unknown").is_none());

    let script = Property::from_name("Script").unwrap();
    assert!(!script.is_binary());
    assert!(!'a'.has_property(script));
    let latin = 'a'.property_value(script).unwrap();
    assert_eq!('b'.property_value(script), Some(latin));
    assert_ne!('Ж'.property_value(script), Some(latin));

    // Not in the property table: read with CharExt::is_combining and friends instead.
    let ccc = Property::from_name("Canonical_Combining_Class").unwrap();
    assert!(!ccc.is_binary());
    assert_eq!('\u{301}'.property_value(ccc), None);
}

#[cfg(feature = "security")]
#[test]
fn resolved_scripts() {
    use mojibake::security::{resolved_scripts, ScriptSet};

    assert_eq!(
        resolved_scripts("Circle").unwrap(),
        ScriptSet::Resolved(vec!["Latn"])
    );
    assert!(resolved_scripts("C\u{0456}rcle").unwrap().is_mixed_script());
    assert_eq!(resolved_scripts("").unwrap(), ScriptSet::All);
    assert_eq!(resolved_scripts("123 !").unwrap(), ScriptSet::All);

    // Han and Hiragana resolve to the Japanese writing system.
    assert_eq!(
        resolved_scripts("漢字かな").unwrap(),
        ScriptSet::Resolved(vec!["Jpan"])
    );
}
