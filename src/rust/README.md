# Mojibake for Rust

Two crates that expose Mojibake to Rust:

- `mojibake-sys` compiles the C library with the [`cc`](https://crates.io/crates/cc) crate and
  declares the raw FFI. No CMake is needed.
- `mojibake` is the safe, idiomatic API, built mostly from extension traits on `str` and `char`.

The Rust standard library has UTF-8 strings, simple character predicates, and full
upper/lowercase mapping. These crates add what it leaves out:

| Area | API |
| --- | --- |
| Encodings and malformed input | `encoding::{decode, encode, convert, validate, codepoint_count, detect}` |
| Bidirectional text (UAX #9) | `bidi::Paragraph::{levels, reorder_line, runs, visual}` |
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

use mojibake::bidi::{Direction, Paragraph};
use mojibake::encoding::{decode, Encoding, Malformed};

assert_eq!(decode(b"a\x00\x00\xd8", Encoding::Utf16Le, Malformed::Replace)?, "a\u{fffd}");
let paragraph = Paragraph::new("abc אבג", Direction::Auto)?;
assert_eq!(paragraph.visual(0..paragraph.len())?, "abc גבא");
```

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

## Building and testing

From the repository root:

```sh
make test-rust
```

This runs rustfmt, clippy, and the tests with all features on and with all features off.

The `mojibake-sys` build script looks for the C sources in this order:

1. `MOJIBAKE_SRC_DIR`, when it is set
2. the repository's `src/` directory, when the crate is built inside a Mojibake checkout
3. `mojibake-sys/csrc/`, the git-ignored copy that `make vendor-rust` makes for packaging

A checkout wins over `csrc/`, so a leftover copy is never compiled in-tree. The source list comes
from the generated `SOURCES` block of `CMakeLists.txt`.

## Packaging and publishing

`make package-rust` copies `src/*.c`, `src/*.h`, `src/CMakeLists.txt`, and `LICENSE` into
`mojibake-sys/csrc/`, then runs `cargo package --workspace` and
`cargo publish --workspace --dry-run`. Both crates are packaged and each is built from its
`.crate` file, with `mojibake` resolving the unpublished `mojibake-sys` from Cargo's local staging
registry. CI runs it on every push. It needs Cargo 1.90 or newer for workspace packaging; users of
the crates still need only Rust 1.77.

Cargo treats the git-ignored `csrc/` as uncommitted, so the target always passes `--allow-dirty`
and instead refuses to run when `src/` or `LICENSE` has uncommitted or untracked changes. Set
`ALLOW_DIRTY=1` to package a work in progress. Extra Cargo flags go in `ARGS`.

To release the crates:

1. Set `VERSION` and run `make update-version`. It sets the workspace version in `Cargo.toml`,
   the `mojibake-sys` dependency of `mojibake`, and both entries in `Cargo.lock`.
2. Commit the release and check that `make test-rust` and `make package-rust` pass.
3. Publish from the same clean checkout. Cargo uploads `mojibake-sys` first, then `mojibake`:

   ```sh
   make vendor-rust
   (cd src/rust && cargo publish --workspace --locked --allow-dirty)
   ```

## Status

This is a scaffold. `mojibake-sys` declares every public function except the variadic and
`va_list` formatting ones. The declarations in `mojibake-sys/src/functions.rs` are generated from
`utils/generate/functions.ts` by `make generate`; do not edit them. The types, constants, and
layout tests in `mojibake-sys/src/lib.rs` and `mojibake-sys/shim/layout.c` are written by hand.
Not done yet:

- expose the remaining APIs in the safe crate: filters, identifiers, emoji sequences, script sets,
  the `_into` variants, and streaming decoders over `mjb_decode_next`
- route `mjb_set_allocator` through `std::alloc`. `mjb_realloc` and `mjb_free` already pass the
  exact block size that `Layout` needs, so owned results could become `String`s without a copy
