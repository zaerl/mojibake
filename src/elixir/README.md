# Mojibake for Elixir

Unicode text processing for Elixir: normalization, locale-aware casing, caseless matching,
grapheme, word, sentence, and line segmentation, terminal width, character data from the Unicode
Character Database, collation, internationalized domain names, confusable detection, identifiers,
encoding conversion, BCP 47 language tags, and bidirectional text. It passes the official Unicode
conformance tests.

It wraps [Mojibake](https://github.com/zaerl/mojibake), a self-contained Unicode library written in
C11 with no dependencies, as a NIF. The package compiles the C sources when you build it, so all
you need is a C compiler and `make`: no CMake, no system library, and no data files to ship.

(Despite the name, it is not a library that repairs garbled text. *Mojibake* is the Japanese word
for the garbage you get when text is decoded with the wrong encoding, which is what correct Unicode
handling avoids.)

```elixir
def deps do
  [{:mojibake, "~> 0.8"}]
end
```

On Linux (x86_64 and aarch64, glibc 2.35 or newer), macOS (Apple silicon and Intel), and
Windows (x86_64), the package downloads a precompiled NIF and needs no compiler. Elsewhere, such as
on Alpine Linux or FreeBSD, it compiles the NIF, which needs a C compiler and `make`. See
[Building from source](#building-from-source).

```elixir
{:ok, "é"} = Mojibake.normalize("é")
{:ok, "İSTANBUL"} = Mojibake.upcase("istanbul", locale: "tr")
true = Mojibake.caseless_equal?("Straße", "STRASSE")

["Hello", ",", " ", "world", "!"] = Mojibake.words("Hello, world!")
["Hello. ", "How are you?"] = Mojibake.sentences("Hello. How are you?")
[allowed: 6, mandatory: 11] = Mojibake.line_breaks("Hello world")

6 = Mojibake.terminal_width("日本語")
"日本" = Mojibake.truncate_to_width("日本語", 5)

"GRINNING FACE" = Mojibake.Char.name(0x1F600)
:greek = Mojibake.Char.script(?Ω)
{1, 2} = Mojibake.Char.numeric_value(?½)
true = Mojibake.Char.property?(?\s, :white_space)
%{type: :flag} = Mojibake.Emoji.sequence("🇮🇹")

["apple", "Apple", "éclair", "zebra"] = Mojibake.Collation.sort(["zebra", "éclair", "apple", "Apple"])
{:ok, "xn--bcher-kva.example"} = Mojibake.IDNA.to_ascii("Bücher.example")
true = Mojibake.Security.confusable?("paypal", "pаypal")
"sr-Latn-RS" = to_string(Mojibake.LanguageTag.parse!("SR-latn-rs"))
"abc גבא" = Mojibake.Bidi.visual("abc אבג")
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
| Character data | `Mojibake.Char`: names, categories, combining classes, bidi classes, decomposition types, numeric values, simple case mappings, blocks, scripts and Script_Extensions, East Asian width, emoji properties, and every stored binary and enumerated property |
| Scripts, blocks, planes, properties | `Mojibake.Script`, `Mojibake.Block`, `Mojibake.Plane`, `Mojibake.Property` |
| Emoji sequences (UTS #51) | `Mojibake.Emoji.sequence/1`, `rgi?/1` |
| Hangul | `Mojibake.Hangul`: composition, decomposition, syllable names |
| Collation (UTS #10) | `Mojibake.Collation`: `compare/3`, `sort/2`, `sort_key/2`, and an `Enum.sort/2` sorter |
| Internationalized domain names (UTS #46) | `Mojibake.IDNA`: `to_ascii/1`, `to_unicode/1` |
| Confusables and mixed scripts (UTS #39) | `Mojibake.Security`: `confusable?/2`, `skeleton/1`, `resolved_scripts/1` |
| Identifiers (UAX #31) | `Mojibake.Identifier`: `valid?/2`, `start?/2`, `continue?/2` |
| Encodings | `Mojibake.Encoding`: UTF-8, UTF-16, and UTF-32 detection, validation, and conversion with byte offsets in errors |
| BCP 47 language tags | `Mojibake.LanguageTag`: strict parsing and formatting |
| Bidirectional text (UAX #9) | `Mojibake.Bidi`: embedding levels, visual reordering, level runs, and mirroring |

## Design

- Transforms return `{:ok, string}` or `{:error, %Mojibake.Error{}}`, and have a `!` variant that
  raises. When the input is already in the requested form, the input binary itself is returned,
  without a copy.
- Queries such as `graphemes/1`, `grapheme_count/1`, and `terminal_width/2` never fail: they treat
  each maximal ill-formed subsequence as U+FFFD.
- Segments are sub-binaries of the input, so splitting a string does not copy it.
- A resolved bidi paragraph is a NIF resource: it is resolved once, can be shared between
  processes, and is freed when it is garbage collected.
- Casing takes the locale as an option, so the package never touches the C library's
  process-global casing locale.
- Inputs larger than 16 KiB run on a dirty CPU scheduler, so long strings do not block the normal
  schedulers.
- Character data uses atoms made from the long UCD names, such as `:uppercase_letter`, `:latin`,
  `:basic_latin`, and `:white_space`. The atom tables are generated from the same Unicode data as
  the C library, so they always agree with it.
- Codepoint functions take integers (`?A`, `0x1F600`) and raise `FunctionClauseError` for anything
  outside 0 to 0x10FFFF.
- The C library allocates through `enif_alloc`, so its memory shows up in the VM's allocator
  statistics.

## Optional features

The C library has four optional features: `:names` (character names), `:collation`, `:idna`, and
`:security`. They are all on by default, and `Mojibake.features/0` lists the ones compiled in.
`Mojibake.Char.name/1` needs `:names`, `Mojibake.Collation` needs `:collation`, `Mojibake.IDNA`
needs `:idna`, and `Mojibake.Security` needs `:security`; without them they return or raise
`Mojibake.Error` with reason `:feature_not_enabled`. Turn off the ones you do not need to make the
NIF smaller:

```elixir
# config/config.exs
config :mojibake, features: []
```

Run `mix deps.compile mojibake --force` after changing it. The `MOJIBAKE_FEATURES` environment
variable, a comma-separated list or `none`, wins over the configuration. The precompiled NIFs have
every feature, so a package with fewer features compiles from source.

## Building from source

The package compiles the NIF instead of downloading it when:

- there is no precompiled NIF for the platform, or downloading it fails
- `config :mojibake, features: [...]` or `MOJIBAKE_FEATURES` leaves out a feature
- the `MOJIBAKE_BUILD` environment variable is `1` or `true`, or the configuration has
  `config :elixir_make, :force_build, mojibake: true`

Compiling needs a C11 compiler and `make`: on Debian and Ubuntu the `build-essential` package, on
Alpine `build-base`, and on macOS the Xcode command-line tools. On Windows it needs Visual Studio
2019 16.8 or newer with the C++ tools: run `mix` from an x64 Native Tools Command Prompt, so that
`cl` and `nmake` are on the `PATH`. The C sources ship with the package; no CMake or system library
is needed.

The precompiled NIFs are assets of the [GitHub releases](https://github.com/zaerl/mojibake/releases)
of Mojibake, checked against the checksums in the package. To download them from a mirror, set
`MOJIBAKE_PRECOMPILED_URL` to a URL template with `@{artefact_filename}` for the file name, such as
`https://mirror.example/mojibake/@{artefact_filename}`.

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
