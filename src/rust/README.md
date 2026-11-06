# Mojibake for Rust

Two crates that expose Mojibake to Rust:

- `mojibake-sys` compiles the C library with the [`cc`](https://crates.io/crates/cc) crate and
  declares the raw FFI. No CMake is needed.
- `mojibake` is the safe, idiomatic API, built mostly from extension traits on `str` and `char`.

The Rust standard library has encoding, simple character predicates, and full upper/lowercase
mapping. These crates add what it leaves out:

| Area | API |
| --- | --- |
| Normalization (UAX #15) | `normalize`, `is_normalized`, `nfkc_casefold` |
| Locale casing, titlecase, folding | `to_uppercase(s, Locale::parse("tr")?)`, `to_titlecase`, `casefold` |
| Caseless matching (D144-D147) | `caseless_eq`, `StrExt::caseless_eq` |
| Segmentation (UAX #29, UAX #14) | `graphemes`, `words`, `sentences`, `line_breaks`, counts |
| Terminal width (UAX #11) | `terminal_width`, `truncate_to_width` |
| Character properties | `CharExt::{general_category, name, block, script, numeric_value, east_asian_width, emoji_properties}` |
| Collation (UTS #10) | `collate::{compare, sort_key, Collator}` |
| IDNA (UTS #46) | `idna::{to_ascii, to_unicode}` |
| Confusables (UTS #39) | `security::{skeleton, is_confusable}` |

```rust
use mojibake::{CharExt, Form, Locale, StrExt};

assert_eq!("e\u{301}".normalize(Form::Nfc)?, "\u{e9}");
assert_eq!("istanbul".to_uppercase_in(Locale::parse("tr")?)?, "İSTANBUL");
assert!("Straße".caseless_eq("STRASSE"));
assert_eq!("👨‍👩‍👧é".graphemes().count(), 2);
assert_eq!("日本語".truncate_to_width(5), "日本");
assert_eq!('½'.numeric_value(), Some(0.5));
assert_eq!(mojibake::idna::to_ascii("bücher.de")?.unwrap(), "xn--bcher-kva.de");
```

## Design

- String transforms return `Cow<str>`. When the C library reports that the input is already in
  the requested form (`mjb_result.owned == false`), the input is borrowed and nothing is
  allocated.
- Casing takes an explicit `Locale`, so the safe API never touches the C library's process-global
  casing locale.
- Segment iterators wrap the C `mjb_*_break_state` iterators and yield `&str` slices of the input.
- C enums are `c_int` constants in `mojibake-sys`, never Rust enums, so an unexpected value from C
  cannot cause undefined behavior.
- `mojibake-sys/shim/layout.c` statically asserts that the opaque Rust break-state storage fits
  every C break state, and the `mojibake-sys` tests compare each hand-written struct's size,
  alignment, and key offsets with the C compiler's.

## Cargo features

`names`, `collation`, `idna`, and `security` are on by default and map to the C library's
`MJB_FEATURE_*` options. Turn them off with `default-features = false` to drop the large tables.
The matching Rust APIs are compiled out.

## Building and testing

From the repository root:

```sh
make test-rust
```

This runs rustfmt, clippy, and the tests with all features on and with all features off. The
build script reads the C sources from the repository's `src/` directory, or from
`MOJIBAKE_SRC_DIR` when it is set. It takes the source list from the generated `SOURCES` block in
`src/CMakeLists.txt`.

## Status

This is a scaffold. The FFI declarations cover only the functions the safe API uses. Not done yet:

- generate the `mojibake-sys` declarations from `utils/generate/functions.ts`, as the WASM exports
  are, instead of writing them by hand
- bind the remaining APIs: encoding conversion between UTF-8/16/32 with malformed-input policies,
  bidi, filters, identifiers, emoji sequences, script sets, and the `_into` variants
- route `mjb_set_allocator` through `std::alloc`. `mjb_realloc` and `mjb_free` already pass the
  exact block size that `Layout` needs, so owned results could become `String`s without a copy
- package the C sources (or the amalgamation) inside `mojibake-sys` so it can be published
- have `make update-version` update the workspace version in `Cargo.toml`
