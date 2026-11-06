// The Mojibake library
//
// This file is distributed under the MIT License. See LICENSE for details.

use mojibake::encoding::{self, Encoding, Malformed};
use mojibake::{Diagnostic, Error};

fn malformed_at(byte_offset: usize, byte_length: usize) -> Error {
    Error::MalformedInput(Some(Diagnostic {
        byte_offset,
        byte_length,
    }))
}

/// Check `decode`, `codepoint_count`, and `validate` of `bytes` under every policy.
fn check(bytes: &[u8], from: Encoding, stop: Error, replace: &str, skip: &str) {
    let decode = |policy| encoding::decode(bytes, from, policy);
    let count = |policy| encoding::codepoint_count(bytes, from, policy);

    assert_eq!(decode(Malformed::Stop), Err(stop), "{bytes:x?} stop");
    assert_eq!(count(Malformed::Stop), Err(stop), "{bytes:x?} count");
    assert_eq!(
        encoding::validate(bytes, from),
        Err(stop),
        "{bytes:x?} validate"
    );

    assert_eq!(
        decode(Malformed::Replace).as_deref(),
        Ok(replace),
        "{bytes:x?} replace"
    );
    assert_eq!(count(Malformed::Replace), Ok(replace.chars().count()));

    assert_eq!(
        decode(Malformed::Skip).as_deref(),
        Ok(skip),
        "{bytes:x?} skip"
    );
    assert_eq!(count(Malformed::Skip), Ok(skip.chars().count()));
}

#[test]
fn ill_formed_utf8() {
    // Invalid leading byte.
    check(
        b"a\xffb",
        Encoding::Utf8,
        malformed_at(1, 1),
        "a\u{fffd}b",
        "ab",
    );
    // Missing continuation: the truncated sequence is one maximal subpart.
    check(
        b"a\xe2\x82b",
        Encoding::Utf8,
        malformed_at(1, 2),
        "a\u{fffd}b",
        "ab",
    );
    // Truncated at the end of the input.
    check(
        b"ab\xf0\x9f",
        Encoding::Utf8,
        malformed_at(2, 2),
        "ab\u{fffd}",
        "ab",
    );
    // Overlong encoding of '/': each byte is replaced on its own.
    check(
        b"\xc0\xafx",
        Encoding::Utf8,
        malformed_at(0, 1),
        "\u{fffd}\u{fffd}x",
        "x",
    );
    // Encoded surrogate U+D800.
    check(
        b"\xed\xa0\x80z",
        Encoding::Utf8,
        malformed_at(0, 1),
        "\u{fffd}\u{fffd}\u{fffd}z",
        "z",
    );
}

#[test]
fn ill_formed_utf16() {
    // Unpaired high surrogate followed by 'b'.
    check(
        b"a\x00\x00\xd8b\x00",
        Encoding::Utf16Le,
        malformed_at(2, 2),
        "a\u{fffd}b",
        "ab",
    );
    // Unpaired low surrogate.
    check(
        b"\x00a\xdc\x00\x00b",
        Encoding::Utf16Be,
        malformed_at(2, 2),
        "a\u{fffd}b",
        "ab",
    );
    // Odd byte count: a truncated code unit.
    check(
        b"a\x00b",
        Encoding::Utf16Le,
        malformed_at(2, 1),
        "a\u{fffd}",
        "a",
    );
    // High surrogate at the end of the input.
    check(
        b"a\x00\x3d\xd8",
        Encoding::Utf16Le,
        malformed_at(2, 2),
        "a\u{fffd}",
        "a",
    );
}

#[test]
fn ill_formed_utf32() {
    // U+110000 is out of range.
    check(
        b"\x00\x00\x11\x00",
        Encoding::Utf32Le,
        malformed_at(0, 4),
        "\u{fffd}",
        "",
    );
    // A surrogate is not a scalar value.
    check(
        b"\x00\x00\xd8\x00\x00\x00\x00A",
        Encoding::Utf32Be,
        malformed_at(0, 4),
        "\u{fffd}A",
        "A",
    );
}

#[test]
fn round_trips() {
    let text = "a€😀 مرحبا";
    let forms = [
        Encoding::Utf8,
        Encoding::Utf16Le,
        Encoding::Utf16Be,
        Encoding::Utf32Le,
        Encoding::Utf32Be,
    ];

    for form in forms {
        let bytes = encoding::encode(text, form).unwrap();

        assert_eq!(encoding::validate(&bytes, form), Ok(()));
        assert_eq!(
            encoding::decode(&bytes, form, Malformed::Stop).unwrap(),
            text
        );
        assert_eq!(
            encoding::codepoint_count(&bytes, form, Malformed::Stop),
            Ok(text.chars().count())
        );

        for to in forms {
            assert_eq!(
                encoding::convert(&bytes, form, to, Malformed::Stop).unwrap(),
                encoding::encode(text, to).unwrap(),
                "{form:?} -> {to:?}"
            );
        }
    }

    assert_eq!(
        encoding::encode("a€😀", Encoding::Utf16Be).unwrap(),
        b"\x00a\x20\xac\xd8\x3d\xde\x00"
    );
    assert_eq!(
        encoding::encode("a€😀", Encoding::Utf32Le).unwrap(),
        b"a\x00\x00\x00\xac\x20\x00\x00\x00\xf6\x01\x00"
    );
    assert_eq!(encoding::encode("", Encoding::Utf16Le).unwrap(), b"");
    assert_eq!(
        encoding::decode(b"", Encoding::Utf32Be, Malformed::Stop).unwrap(),
        ""
    );
}

#[test]
fn byte_order_marks() {
    // The generic forms consume the BOM and need one.
    assert_eq!(
        encoding::decode(b"\xff\xfea\x00", Encoding::Utf16, Malformed::Stop).unwrap(),
        "a"
    );
    assert_eq!(
        encoding::decode(b"\xfe\xff\x00a", Encoding::Utf16, Malformed::Stop).unwrap(),
        "a"
    );
    assert_eq!(
        encoding::decode(
            b"\x00\x00\xfe\xff\x00\x00\x00a",
            Encoding::Utf32,
            Malformed::Stop
        )
        .unwrap(),
        "a"
    );
    assert_eq!(
        encoding::decode(b"a\x00", Encoding::Utf16, Malformed::Stop),
        Err(Error::InvalidEncoding)
    );

    // An explicit byte order keeps the BOM as U+FEFF.
    assert_eq!(
        encoding::decode(b"\xff\xfea\x00", Encoding::Utf16Le, Malformed::Stop).unwrap(),
        "\u{feff}a"
    );

    // There is no output form without a byte order.
    assert_eq!(
        encoding::encode("a", Encoding::Utf16),
        Err(Error::InvalidEncoding)
    );
    assert_eq!(
        encoding::encode("a", Encoding::Utf32),
        Err(Error::InvalidEncoding)
    );
}

#[test]
fn detection() {
    assert_eq!(encoding::detect(b"hello"), Some(Encoding::Utf8));
    assert_eq!(encoding::detect("héllo".as_bytes()), Some(Encoding::Utf8));
    assert_eq!(encoding::detect(b"\xef\xbb\xbfa"), Some(Encoding::Utf8));
    assert_eq!(encoding::detect(b"\xff\xfea\x00"), Some(Encoding::Utf16));
    assert_eq!(encoding::detect(b"\xfe\xff\x00a"), Some(Encoding::Utf16));
    assert_eq!(encoding::detect(b"\xff\xfe\x00\x00"), Some(Encoding::Utf32));
    assert_eq!(encoding::detect(b"\x00\x00\xfe\xff"), Some(Encoding::Utf32));
    assert_eq!(encoding::detect(b"\xff"), None);
    assert_eq!(encoding::detect(b""), None);
}

#[test]
fn diagnostic_display() {
    let error = encoding::decode(b"ab\xff", Encoding::Utf8, Malformed::Stop).unwrap_err();

    assert!(error.to_string().ends_with(" at byte 2"), "{error}");
}
