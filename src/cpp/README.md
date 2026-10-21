# Mojibake C++20 wrapper

`mojibake.hpp` is a header-only C++20 interface over the Mojibake C library. It adds string and
view types, RAII ownership, standard ranges for segments and codepoints, and exceptions that keep
the library's `mjb_status`. Everything is in the `mjb` namespace.

## Setup

The wrapper needs a C++20 compiler and standard library with concepts and `<ranges>`, such as GCC
10 or later, a recent Clang or Apple Clang, or MSVC 19.30 (Visual Studio 2022), and the Mojibake
library. The library itself still builds as C11 or C++17; the test suite skips the wrapper tests
when the compiler lacks C++20 support. Link the `Mojibake::mojibake` CMake target (see the main
[README](../../README.md#cmake-dependency)), then include the header:

```cpp
#include <mojibake.hpp>     // installed: the header is copied next to mojibake.h
// #include <cpp/mojibake.hpp> with add_subdirectory or FetchContent
```

MSVC needs no extra flags: the headers read `_MSVC_LANG`, so `/Zc:__cplusplus` is not required.

```cpp
std::string text = mjb::nfc("Cafe\xCC\x81"); // "Caf\xC3\xA9"
std::u8string folded = mjb::casefold(u8"Straße"); // u8"strasse"
size_t clusters = mjb::grapheme_count(u"A\U0001F1EE\U0001F1F9"); // 2

for(std::string_view word : mjb::words(text)) {
    // "Caf\xC3\xA9"
}
```

## Text input

Functions that read text take any `mjb::text_like` string in the platform's byte order:

- UTF-8: `std::string`, `std::string_view`, `const char *`, and `"..."` literals
- UTF-8: `std::u8string`, `std::u8string_view`, and `u8"..."` literals
- UTF-16: `std::u16string`, `std::u16string_view`, and `u"..."`
- UTF-32: `std::u32string`, `std::u32string_view`, and `U"..."`

Those that only read it take an `mjb::Text`, a non-owning view of the bytes and their encoding
that converts implicitly from all of these. `mjb::native_utf16_encoding` and
`mjb::native_utf32_encoding` name the platform's byte order. For any other encoding or byte order,
such as bytes read from a file, construct the `Text` with an explicit encoding:

```cpp
// UTF-16BE bytes, for example read from a file: "AB"
const std::string_view file_bytes("\0A\0B", 4);
size_t count = mjb::codepoint_count(mjb::Text(file_bytes, MJB_ENC_UTF_16BE)); // 2

bool same = mjb::caseless_match(u"Straße", "STRASSE"); // any two text types
```

## Options

Optional arguments are passed as a struct with designated initializers, so a call names only what
it changes:

| Struct | Fields, in declaration order | Used by |
| --- | --- | --- |
| `Options` | `malformed`, `diagnostic` | transforms, counts, decoding |
| `CaseOptions` | `locale`, `malformed`, `diagnostic` | case mapping |
| `MatchOptions` | `malformed`, `diagnostic`, `other_diagnostic` | `caseless_match`, `confusable_match` |
| `CollationOptions` | `strength`, `variable_weighting`, `malformed`, `diagnostic`, `other_diagnostic` | `collation_compare`, `collation_key` |

```cpp
mjb_diagnostic diagnostic;
std::string replaced = mjb::nfc("a\x80", { .malformed = MJB_MALFORMED_REPLACE });
std::string upper = mjb::uppercase("istanbul", { .locale = MJB_LOCALE_TR }); // "\xC4\xB0STANBUL"
size_t width = mjb::terminal_width("a\x80", { .malformed = MJB_MALFORMED_SKIP, .diagnostic = &diagnostic });
int order = mjb::collation_compare("a", "A", { .strength = mjb::CollationStrength::Primary }); // 0
```

Malformed input throws with the default `MJB_MALFORMED_STOP` policy; `MJB_MALFORMED_REPLACE`
substitutes U+FFFD and `MJB_MALFORMED_SKIP` drops the malformed bytes. `diagnostic` reports where
the first malformed sequence is, and `other_diagnostic` does the same for the second text of a
comparison.

## Transforms

Every transform has the same two forms. Given only the input, it returns a new string of the
input's own type and encoding. Given an output, it writes there instead:

```cpp
std::u16string upper16 = mjb::uppercase(u"straße"); // u"STRASSE"
std::u32string nfd32 = mjb::nfd(U"é"); // U"é"

std::string out;
mjb::nfc(u"é", out); // UTF-16 in, UTF-8 out: the output's type selects its encoding
mjb::nfc("e\xCC\x81", mjb::encoded(out, MJB_ENC_UTF_16LE)); // "\xE9\0": bytes in any encoding
```

| Transform | Functions |
| --- | --- |
| Normalization | `nfc`, `nfd`, `nfkc`, `nfkd`, `normalize(input, form)` |
| Case mapping | `uppercase`, `lowercase`, `titlecase`, `casefold`, `casefold_simple`, `case_map(input, type)` |
| Folding and filtering | `nfkc_casefold`, `filter(input, mjb::Filter::...)` |
| Security | `confusable_skeleton` |
| Encoding | `convert_encoding(input, out)`, `convert_encoding<char16_t>(input)`, `convert_encoding(input, encoding)` |

```cpp
std::string cleaned = mjb::filter("a\x01  b", mjb::Filter::Controls | mjb::Filter::CollapseSpaces);
std::u16string utf16 = mjb::convert_encoding<char16_t>("caf\xC3\xA9"); // u"café"
std::string utf32be = mjb::convert_encoding("A", MJB_ENC_UTF_32BE); // "\0\0\0A"
```

The output can be a `std::string` (UTF-8), `std::u8string`, `std::u16string`, or `std::u32string`
lvalue, or an `mjb::encoded(bytes, encoding)` for bytes in another encoding. Its content is
replaced, and its capacity is reused, so calling a transform in a loop with the same string avoids
an allocation per call. If the transform throws, the output is left empty.

```cpp
std::string line_out;
const std::string_view lines[] = { "Cafe\xCC\x81", "na\xC3\xAFve" };

for(std::string_view line : lines) {
    mjb::nfc(line, line_out);
    // use line_out
}
```

`nfc`, `uppercase`, and the other fixed transforms are function objects, so they can be passed to
algorithms that take a callable.

## Analysis

These functions take an `mjb::Text`:

```cpp
size_t codepoints = mjb::codepoint_count("\xE6\x96\x87\xE5\xAD\x97"); // 2
size_t graphemes = mjb::grapheme_count("e\xCC\x81"); // 1
size_t words = mjb::word_count(u"Hello, world!"); // 2
size_t sentences = mjb::sentence_count(U"Hi. Bye."); // 2
size_t columns = mjb::terminal_width("A\xE7\x95\x8C"); // 3: U+754C is wide
mjb_status valid = mjb::validate_encoding("a\x80"); // MJB_STATUS_MALFORMED_INPUT

bool identifier = mjb::is_identifier("delta_2"); // true
mjb_quick_check_result quick =
    mjb::normalization_quick_check("caf\xC3\xA9", mjb::NormalizationForm::NFC); // MJB_QC_YES
bool rgi = mjb::is_rgi_emoji("\xF0\x9F\x87\xAE\xF0\x9F\x87\xB9"); // true: the Italian flag
std::optional<mjb::EmojiSequence> sequence = mjb::emoji_sequence("\xF0\x9F\x99\x82");

bool caseless = mjb::caseless_match("Stra\xC3\x9F" "e", "STRASSE"); // true
bool compatible = mjb::caseless_match("\xEF\xAC\x81", "FI", mjb::CaselessMode::Compatibility);
int order = mjb::collation_compare("apple", "banana"); // negative
std::string key = mjb::collation_key("apple"); // binary sort key, compares like the text
bool confusable = mjb::confusable_match("h\xD0\xB5llo", "hello"); // true: Cyrillic U+0435

mjb::LocaleId locale = mjb::locale_parse("sr-Latn-RS");
std::string_view region = locale.region(); // "RS"
```

The counts and comparisons throw on malformed input with the default policy. The predicates, such
as `is_identifier` and `is_rgi_emoji`, are `noexcept` and return false for input they cannot
accept. A collation key is bytes whatever the input's encoding, so `collation_key` always returns
or writes a `std::string`.

## Iteration and segmentation

All ranges are C++20 views (`std::ranges::view` and `std::ranges::forward_range`), so they work
with range-based `for`, `std::ranges` algorithms, and adaptors such as `std::views::take`. They are
also borrowed ranges: an iterator stays valid after its range is destroyed, as long as the input
lives.

### Segments

`graphemes`, `words`, `sentences`, and `line_segments` return ranges whose elements are views of
the input, in its own code units: `std::string_view` for UTF-8 and `std::u16string_view` or
`std::u32string_view` for UTF-16 and UTF-32. Joining the segments gives back the input.

```cpp
const std::string text = "Hello, world. Bye.";

for(std::string_view sentence : mjb::sentences(text)) {
    // "Hello, world. ", "Bye."
}

for(std::u16string_view word : mjb::words(u"Hello, world") | std::views::take(2)) {
    // u"Hello", u","
}

for(std::string_view line : mjb::line_segments("foo bar\nbaz")) {
    // "foo ", "bar\n", "baz": each keeps its trailing spaces and mandatory break
}
```

The ranges reject a temporary `std::string`, which a range-based `for` loop would destroy before
the loop body runs.

To cut text to a limit without splitting a segment, use `grapheme_truncate`,
`grapheme_truncate_width`, `word_truncate`, and `word_truncate_width`. They return a prefix view of
the input in its own code units, and also reject a temporary string:

```cpp
std::string_view prefix = mjb::grapheme_truncate("e\xCC\x81" "abc", 2); // "e\xCC\x81" "a"
std::string_view fits = mjb::word_truncate_width("Hello world", 6); // "Hello "
std::u16string_view flags = mjb::grapheme_truncate(u"A\U0001F1EE\U0001F1F9Z", 2); // 5 code units
```

### Codepoints

`codepoints` decodes on the fly. Malformed input becomes U+FFFD by default; pass
`MJB_MALFORMED_STOP` to end the iteration there or `MJB_MALFORMED_SKIP` to drop it.

```cpp
for(char32_t codepoint : mjb::codepoints(u"A\U0001F642")) {
    // U'A', U'\U0001F642'
}
```

`for_each_codepoint` calls a function for each codepoint with its `mjb_character` details and
returns the iteration status. The callable returns false to stop; an exception it throws is
rethrown to the caller.

```cpp
size_t combining = 0;
mjb_status status = mjb::for_each_codepoint("e\xCC\x81",
    [&combining](const mjb_character &character, mjb_character_position) {
        combining += mjb::category_is_combining(character.category);

        return true;
    });
```

`decode_next` and `decode_previous` step through a text one codepoint at a time from a byte
offset.

### Break positions

`grapheme_breaks`, `word_breaks`, `sentence_breaks`, and `line_breaks` return ranges of
`mjb::BreakResult`: every position between codepoints with its break type. `position` is a byte
offset in the input, and `codepoint` is the codepoint that starts there, `MJB_CODEPOINT_NOT_VALID`
at the end of the text. Use them when you need the no-break positions or the break types; to
iterate the segments themselves, use the segment ranges.

```cpp
for(const mjb::BreakResult &result : mjb::line_breaks("a b")) {
    if(result.is_mandatory()) {
        // a mandatory break at result.position
    }
}

auto opportunities = std::ranges::count_if(mjb::line_breaks("a b"),
    [](const mjb::BreakResult &result) { return result.is_break(); }); // 2
```

## Characters and properties

Codepoint predicates are free functions; `Character` holds the full record that
`mjb_codepoint_info` reads.

```cpp
mjb::Character character(0x00E9);
std::string_view name = character.name(); // "LATIN SMALL LETTER E WITH ACUTE"
std::optional<mjb_codepoint> upper = character.uppercase();
std::optional<mjb::Character> unassigned = mjb::Character::from(0x0378); // std::nullopt

std::optional<mjb::BlockInfo> block = mjb::block_info(0x0041); // Basic Latin
std::optional<bool> alphabetic = mjb::property_binary(0x0041, MJB_PR_ALPHABETIC);
mjb_script script = mjb::script(0x0041);
bool emoji = mjb::is_emoji(0x1F642);

std::string utf8 = mjb::encode(0x00E9); // "\xC3\xA9"
std::string utf16 = mjb::encode(0x1F642, MJB_ENC_UTF_16LE); // 4 bytes: a surrogate pair
std::array<mjb_codepoint, 3> jamo = mjb::hangul_syllable_decompose(0xD55C); // U+1112 U+1161 U+11AB
```

`Character` throws for a codepoint the library cannot describe; `Character::from` returns
`std::nullopt` instead. When the library is built with `MJB_FEATURE_CHARACTER_NAMES=OFF`, `name()`
returns a placeholder such as `Codepoint U+00E9`.

## Bidirectional text

`BidiParagraph` resolves the embedding levels of a paragraph (UAX #9) and owns the result. It
takes any `Text`, and `chars()` returns its characters as a `std::span`:

```cpp
mjb::BidiParagraph paragraph("English \xD7\xA2\xD7\x91\xD7\xA8\xD7\x99\xD7\xAA");
std::vector<size_t> visual_order = paragraph.visual_order(); // logical indices in display order
std::vector<mjb_bidi_run> runs = paragraph.line_runs();
uint8_t level = paragraph.chars()[0].level;
```

## Internationalized domain names

`idna_to_ascii` and `idna_to_unicode` return an `IdnaResult` with the domain name, in the input's
string type, and its `mjb_idna_error` flags. Given an output, they write there and return the
flags.

```cpp
mjb::IdnaResult ascii = mjb::idna_to_ascii("b\xC3\xBC" "cher.example");
if(ascii.valid()) {
    std::string domain = ascii.text; // "xn--bcher-kva.example"
}

bool empty_label = mjb::idna_to_ascii("a..b").has_error(MJB_IDNA_ERROR_EMPTY_LABEL); // true
```

## Errors

Failures throw `mjb::LibraryError`, a `std::system_error` whose `code()` is in
`mjb::error_category()`. `status()` returns the original `mjb_status`, and `what()` names the
failed operation followed by the library's message for that status. `mjb::make_error_code` turns
any status into a `std::error_code`, and `mjb::status_message` returns the library's message.

```cpp
try {
    std::string normalized = mjb::nfc("a\x80");
} catch(const mjb::LibraryError &error) {
    if(error.status() == MJB_STATUS_MALFORMED_INPUT) {
        // error.what(): "Normalization failed: ..."
    }
}
```

Functions for a feature the library was built without, such as collation with
`MJB_FEATURE_COLLATION=OFF`, throw with `MJB_STATUS_FEATURE_NOT_ENABLED`.

## Lifetimes

`mjb::Text`, the ranges, their elements, and the truncation results are views: the input must
outlive them, as with `std::string_view`. The functions that return a view reject a temporary
`std::string` at compile time, and Clang's `-Wdangling` warns about a `Text` or a view kept past
the end of its input, but a view of other temporary storage is still the caller's responsibility.
Transforms return or write owned strings, so they have no lifetime requirements.

## Allocation and versions

`mjb::set_allocator` installs custom allocation functions for the C library, which allocates the
`BidiParagraph` data and temporary buffers. The strings that the wrapper returns use the standard
allocator.

`mjb::version()`, `mjb::version_number()`, and `mjb::unicode_version()` report the library and
Unicode versions.
