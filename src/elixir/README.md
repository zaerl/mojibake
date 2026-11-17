# Mojibake for Elixir

Unicode text processing for Elixir: normalization, locale-aware casing, caseless matching,
grapheme, word, sentence, and line segmentation, and terminal width. It passes the official Unicode
conformance tests.

It wraps [Mojibake](https://github.com/zaerl/mojibake), a self-contained Unicode library written in
C11 with no dependencies, as a NIF. The package compiles the C sources when you build it, so all
you need is a C compiler and `make`: no CMake, no system library, and no data files to ship.

(Despite the name, it is not a library that repairs garbled text. *Mojibake* is the Japanese word
for the garbage you get when text is decoded with the wrong encoding, which is what correct Unicode
handling avoids.)

```elixir
def deps do
  [{:mojibake, "~> 0.7"}]
end
```

```elixir
{:ok, "é"} = Mojibake.normalize("é")
{:ok, "İSTANBUL"} = Mojibake.upcase("istanbul", locale: "tr")
true = Mojibake.caseless_equal?("Straße", "STRASSE")

["Hello", ",", " ", "world", "!"] = Mojibake.words("Hello, world!")
["Hello. ", "How are you?"] = Mojibake.sentences("Hello. How are you?")
[allowed: 6, mandatory: 11] = Mojibake.line_breaks("Hello world")

6 = Mojibake.terminal_width("日本語")
"日本" = Mojibake.truncate_to_width("日本語", 5)
```

## What it adds to `String`

`String` has UTF-8 strings, graphemes, the four normalization forms, and upper and lowercase with
`:turkic` and `:greek` modes. Its Unicode data is the one built into your Elixir release. This
package adds:

| Area | API |
| --- | --- |
| Normalization (UAX #15) | `normalize/3` with `:nfkc_casefold`, `normalized?/2`, `quick_check/2` |
| Casing with any BCP 47 locale | `upcase/2`, `downcase/2`, `titlecase/2` with `locale: "lt"` |
| Case folding | `casefold/2`, full or simple |
| Caseless matching (D144-D147) | `caseless_equal?/3` |
| Word and sentence segmentation (UAX #29) | `words/1`, `sentences/1`, `word_count/1`, `sentence_count/1`, `word_prefix/2` |
| Line breaking (UAX #14) | `line_breaks/1` |
| Graphemes (UAX #29) | `graphemes/1`, `grapheme_count/1`, `grapheme_prefix/2` |
| Terminal width (UAX #11) | `terminal_width/2`, `truncate_to_width/3` |
| Malformed UTF-8 | a `:malformed` option on every transform, and errors with the byte offset |

## Design

- Transforms return `{:ok, string}` or `{:error, %Mojibake.Error{}}`, and have a `!` variant that
  raises. When the input is already in the requested form, the input binary itself is returned,
  without a copy.
- Queries such as `graphemes/1`, `grapheme_count/1`, and `terminal_width/2` never fail: they treat
  each maximal ill-formed subsequence as U+FFFD.
- Segments are sub-binaries of the input, so splitting a string does not copy it.
- Casing takes the locale as an option, so the package never touches the C library's
  process-global casing locale.
- Inputs larger than 16 KiB run on a dirty CPU scheduler, so long strings do not block the normal
  schedulers.
- The C library allocates through `enif_alloc`, so its memory shows up in the VM's allocator
  statistics.

## Optional features

The C library has four optional features: `:names` (character names), `:collation`, `:idna`, and
`:security`. They are all on by default, and `Mojibake.features/0` lists the ones compiled in. They
are not used by the functions in this release, so you can turn them off to make the NIF smaller:

```elixir
# config/config.exs
config :mojibake, features: []
```

Run `mix deps.compile mojibake --force` after changing it. The `MOJIBAKE_FEATURES` environment
variable, a comma-separated list or `none`, wins over the configuration.

## Not wrapped yet

- character properties, names, blocks, scripts, and numeric values
- encodings other than UTF-8, emoji sequences, and Hangul
- collation, IDNA, identifiers, confusables, and BCP 47 tag parsing
- the bidirectional algorithm
- precompiled NIFs, and Windows

## Contributing

The package lives in the [Mojibake repository](https://github.com/zaerl/mojibake), in
`src/elixir/`. Inside the repository it compiles the C sources in `src/`; the Hex package carries
a copy. From the repository root:

- `make test-elixir` runs the formatter check, the tests with all features on and then off, and the
  docs build, all with warnings as errors. The conformance tests read the Unicode test files that
  `./utils/generate/scripts/download-test-data.sh` downloads; they are skipped if those are missing.
- `make package-elixir` copies the C sources into the package, builds it with `mix hex.build`, and
  checks that the unpacked package compiles and loads. Nothing is published.

## License

MIT. See [LICENSE](https://github.com/zaerl/mojibake/blob/main/LICENSE).
