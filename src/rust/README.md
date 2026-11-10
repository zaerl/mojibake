# Mojibake for Rust

[![crates.io](https://img.shields.io/crates/v/mojibake-rs.svg)](https://crates.io/crates/mojibake-rs)
[![docs.rs](https://img.shields.io/docsrs/mojibake-rs)](https://docs.rs/mojibake-rs)
[![Rust](https://img.shields.io/badge/rust-1.77.0%2B-blue.svg?maxAge=3600)](https://github.com/zaerl/mojibake)

Unicode text processing for Rust: normalization, locale-aware casing, grapheme, word, sentence,
and line segmentation, bidirectional text, collation, IDNA, confusable detection, and the full
Unicode Character Database. It passes the official Unicode conformance tests.

It wraps [Mojibake](https://github.com/zaerl/mojibake), a self-contained Unicode library written in
C11 with no dependencies. The build script compiles the C sources with the
[`cc`](https://crates.io/crates/cc) crate, so all you need is a C compiler: no CMake, no system
library, and no data files to ship.

(Despite the name, it isn't a simple library that repair garbled text. *Mojibake* is the Japanese
word for the garbage you get when text is decoded with the wrong encoding, which is what correct
Unicode handling avoids.)

```toml
[dependencies]
mojibake-rs = "0.6"
```

The package is named `mojibake-rs` because `mojibake` is taken on crates.io. The library itself is
still named `mojibake`, so you write `use mojibake::...`.

```rust
use mojibake::{CharExt, Form, Locale, StrExt};

assert_eq!("e\u{301}".normalize(Form::Nfc)?, "\u{e9}");
assert_eq!("istanbul".to_uppercase_in(Locale::parse("tr")?)?, "İSTANBUL");
assert!("Straße".caseless_eq("STRASSE"));
assert_eq!("👨‍👩‍👧é".graphemes().count(), 2);
assert_eq!("日本語".truncate_to_width(5), "日本");
assert_eq!('½'.numeric_value(), Some(0.5));
assert_eq!(mojibake::idna::to_ascii("bücher.de")?.unwrap(), "xn--bcher-kva.de");

use mojibake::bidi::{Direction, Paragraph};
use mojibake::encoding::{decode, Encoding, Malformed};

assert_eq!(decode(b"a\x00\x00\xd8", Encoding::Utf16Le, Malformed::Replace)?, "a\u{fffd}");
let paragraph = Paragraph::new("abc אבג", Direction::Auto)?;
assert_eq!(paragraph.visual(0..paragraph.len())?, "abc גבא");
```

## What it adds to `std`

The Rust standard library has UTF-8 strings, simple character predicates, and full
upper/lowercase mapping. This crate adds what it leaves out:

| Area | API |
| --- | --- |
| Encodings and malformed input | `encoding::{decode, encode, convert, validate, codepoint_count, detect}` |
| Bidirectional text (UAX #9) | `bidi::Paragraph::{levels, reorder_line, runs, visual}`, resolved `BidiClass` per character |
| Normalization (UAX #15) | `normalize`, `is_normalized`, `nfkc_casefold` |
| Locale casing, titlecase, folding | `to_uppercase(s, Locale::parse("tr")?)`, `to_titlecase`, `casefold` |
| BCP 47 language tags (RFC 5646) | `LanguageTag::{parse, language, script, region, variants, extensions, private_use, locale}` |
| Caseless matching (D144-D147) | `caseless_eq`, `StrExt::caseless_eq` |
| Segmentation (UAX #29, UAX #14) | `graphemes`, `words`, `sentences`, `line_breaks`, counts, `grapheme_prefix`, `word_prefix` |
| Terminal width (UAX #11) | `terminal_width`, `truncate_to_width` |
| Character properties | `CharExt::{general_category, name, block, plane, script, script_extensions, numeric_value, east_asian_width, emoji_properties}` |
| Unicode planes | `Plane::{new, of, number, name, abbreviation, codepoints}` |
| `UnicodeData.txt` record | `CharExt::info` → `CharInfo`, `CharExt::{canonical_combining_class, bidi_class, decomposition_type, simple_uppercase, ...}` |
| Any binary or enumerated property | `Property::{from_name, all}`, `CharExt::{has_property, property_value}` |
| Text filters | `filter(s, Filter::NORMALIZE \| Filter::COLLAPSE_SPACES)` |
| Identifiers (UAX #31) | `is_identifier`, `CharExt::{is_id_start, is_xid_start, is_pattern_syntax, ...}` |
| Emoji sequences (UTS #51) | `emoji::{sequence, is_sequence, is_rgi}` |
| Collation (UTS #10) | `collate::{compare, sort_key, Collator}` |
| IDNA (UTS #46) | `idna::{to_ascii, to_unicode}`, typed `idna::IdnaErrors` flags |
| Hangul (Unicode 3.12) | `hangul::{compose, decompose, syllable_name, is_syllable, is_jamo, ...}` |
| Confusables and script sets (UTS #39) | `security::{skeleton, is_confusable, resolved_scripts}` |

## Crates

- `mojibake-rs` is the safe, idiomatic API, built mostly from extension traits on `str` and `char`.
  This is the one to depend on.
- `mojibake-sys` compiles the C library and declares the raw FFI. Use it directly only to write
  your own bindings.

## Design

- String transforms return `Cow<str>`. When the C library reports that the input is already in
  the requested form (`mjb_result.owned == false`), the input is borrowed and nothing is
  allocated.
- Casing takes an explicit `Locale`, so the safe API never touches the C library's process-global
  casing locale.
- Segment iterators wrap the C `mjb_*_break_state` iterators and yield `&str` slices of the input.
- `encoding` works on byte slices. `Malformed::Stop` returns `Error::MalformedInput` with the byte
  offset and length of the first ill-formed subsequence; `Replace` and `Skip` always succeed.
  `Encoding::Utf16` and `Utf32` read the byte order from a BOM and consume it; the explicit
  byte orders keep a BOM as U+FEFF.
- `bidi::Paragraph` owns the C `mjb_bidi_paragraph` and frees it on drop. Its positions index the
  characters left after rule X9 removes embedding controls; `BidiChar::byte_offset` and
  `Paragraph::byte_range` map them back to the text.
- C enums are `c_int` constants in `mojibake-sys`, never Rust enums, so an unexpected value from C
  cannot cause undefined behavior.
- `mojibake-sys/shim/layout.c` statically asserts that the opaque Rust break-state storage fits
  every C break state, and the `mojibake-sys` tests compare each hand-written struct's size,
  alignment, and key offsets with the C compiler's.

## Cargo features

`names`, `collation`, `idna`, and `security` are on by default and map to the C library's
`MJB_FEATURE_*` options. Turn them off with `default-features = false` to drop the large tables.
The matching Rust APIs are compiled out.

## Not wrapped yet

- the `_into` variants and the streaming decoder over `mjb_decode_next`. `String`, `Cow`, and
  `str` iterators cover the same ground today
- a custom allocator: the C library uses its default allocator rather than `std::alloc`

`mojibake-sys` declares every public C function except the variadic and `va_list` formatting ones.

## Contributing

The crates live in the [Mojibake repository](https://github.com/zaerl/mojibake). See the
[Rust section of CONTRIBUTING.md](https://github.com/zaerl/mojibake/blob/main/CONTRIBUTING.md#rust-crates)
for building, testing, packaging, and releasing them.
