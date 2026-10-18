# Mojibake C++17 wrapper

`mojibake.hpp` is a header-only C++17 interface over the Mojibake C library. It adds string and
view types, RAII ownership, ranges for segments and codepoints, and exceptions that keep the
library's `mjb_status`. Everything is in the `mjb` namespace.

## Setup

The wrapper needs a C++17 compiler and the Mojibake library. Link the `Mojibake::mojibake` CMake
target (see the main [README](../../README.md#cmake-dependency)), then include the header:

```cpp
#include <mojibake.hpp>     // installed: the header is copied next to mojibake.h
// #include <cpp/mojibake.hpp> with add_subdirectory or FetchContent
```

MSVC needs no extra flags: the headers read `_MSVC_LANG`, so `/Zc:__cplusplus` is not required.

```cpp
std::string text = mjb::nfc("Cafe\xCC\x81"); // "Caf\xC3\xA9"
std::string folded = mjb::casefold("Stra\xC3\x9F" "e"); // "strasse"
size_t clusters = mjb::grapheme_count(u"A\U0001F1EE\U0001F1F9"); // 2

for(std::string_view word : mjb::words(text)) {
    // "Caf\xC3\xA9"
}
```

## Text input

Functions that read text without producing new text take an `mjb::Text`: a non-owning view of the
bytes and their encoding, the C++ counterpart of `mjb_text`. It converts implicitly from:

- UTF-8: `std::string`, `std::string_view`, `const char *`, and string literals
- UTF-16: `std::u16string`, `std::u16string_view`, and `u"..."`
- UTF-32: `std::u32string`, `std::u32string_view`, and `U"..."`

UTF-16 and UTF-32 use the platform's byte order, which `mjb::native_utf16_encoding` and
`mjb::native_utf32_encoding` name. For any other encoding or byte order, such as bytes read from
a file, construct the `Text` with an explicit encoding:

```cpp
// UTF-16BE bytes, for example read from a file: "AB"
const std::string_view file_bytes("\0A\0B", 4);
size_t count = mjb::codepoint_count(mjb::Text(file_bytes, MJB_ENC_UTF_16BE)); // 2

bool same = mjb::caseless_match(u"Stra\u00DFe", "STRASSE"); // any two text types
```

## Transforms

Transforms return new text. Each has a `std::string_view` overload with input and output
encodings, UTF-8 by default, that returns a `std::string` of output bytes, and a template that
takes a UTF-16 or UTF-32 string and returns the same string type:

| Transform | Functions |
| --- | --- |
| Normalization | `normalize`, `nfc`, `nfd`, `nfkc`, `nfkd` |
| Case mapping | `case_map`, `uppercase`, `lowercase`, `titlecase`, `casefold`, `casefold_simple` |
| Folding and filtering | `nfkc_casefold`, `filter` |
| Security | `confusable_skeleton` |
| Encoding | `convert_encoding` (`std::string_view` only) |

```cpp
std::string upper = mjb::uppercase("istanbul", MJB_LOCALE_TR); // "\xC4\xB0STANBUL"
std::u16string upper16 = mjb::uppercase(u"stra\u00DFe"); // u"STRASSE"
std::u32string nfd32 = mjb::nfd(U"\u00E9"); // U"e\u0301"

std::string utf16le = mjb::nfc("e\xCC\x81", MJB_ENC_UTF_8, MJB_ENC_UTF_16LE); // "\xE9\0"
std::string cleaned = mjb::filter("a\x01  b", mjb::Filter::Controls | mjb::Filter::CollapseSpaces);
std::string replaced = mjb::nfc("a\x80", MJB_ENC_UTF_8, MJB_ENC_UTF_8, MJB_MALFORMED_REPLACE);
```

Malformed input throws with the default `MJB_MALFORMED_STOP` policy; `MJB_MALFORMED_REPLACE`
substitutes U+FFFD and `MJB_MALFORMED_SKIP` drops the malformed bytes. Functions that take a
policy also take an optional `mjb_diagnostic *` that reports where the first malformed sequence
is.

### Reusing output with `_into`

Every transform has an `_into` variant that writes into a string you pass, after the required
arguments, instead of returning a new one: `nfc_into(input, out)`,
`normalize_into(input, form, out)`, `uppercase_into(input, out, locale)`,
`convert_encoding_into(input, from, to, out)`, and so on. `collation_key_into`,
`idna_to_ascii_into`, and `idna_to_unicode_into` exist too; the IDNA ones return the error flags.

The content of `out` is replaced, and its capacity is reused, so calling an `_into` function in a
loop with the same string avoids an allocation per call. If the function throws, `out` is left
empty.

```cpp
std::string out;
const std::string_view lines[] = { "Cafe\xCC\x81", "na\xC3\xAFve" };

for(std::string_view line : lines) {
    mjb::nfc_into(line, out);
    // use out
}

std::u16string out16;
mjb::uppercase_into(u"stra\u00DFe", out16); // UTF-16 input writes a std::u16string
```

### Zero-copy results

The `_result` functions, such as `normalize_result` and `case_map_result`, return the C library's
output as an `mjb::TextResult`, with `view()` and `str()` accessors, instead of copying it into a
`std::string`. `normalize_result`, `filter_result`, and `convert_encoding_result` go further: when
the output equals the input, they do not copy it at all, `owned()` is false, and `view()` points
into the input.

```cpp
const std::string input = "already NFC";
mjb::TextResult result = mjb::normalize_result(input, mjb::NormalizationForm::NFC);
std::string_view normalized = result.view(); // points into input: owned() is false
std::string copy = result.str();
```

Because their result can borrow the input, those three reject a temporary `std::string` at compile
time.

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

bool caseless = mjb::caseless_match("Stra\xC3\x9F" "e", "STRASSE", mjb::CaselessMode::Canonical);
int order = mjb::collation_compare("apple", "banana"); // negative
std::string key = mjb::collation_key("apple"); // binary sort key, compares like the text
bool confusable = mjb::confusable_match("h\xD0\xB5llo", "hello"); // true: Cyrillic U+0435

mjb::LocaleId locale = mjb::locale_parse("sr-Latn-RS");
std::string_view region = locale.region(); // "RS"
```

The counts and comparisons throw on malformed input with the default policy. The predicates, such
as `is_identifier` and `is_rgi_emoji`, are `noexcept` and return false for input they cannot
accept.

## Iteration and segmentation

### Segments

`graphemes`, `words`, `sentences`, and `line_segments` return ranges whose elements are views of
the input, in its own code units: `std::string_view` for UTF-8 and `std::u16string_view` or
`std::u32string_view` for UTF-16 and UTF-32. Joining the segments gives back the input.

```cpp
const std::string text = "Hello, world. Bye.";

for(std::string_view sentence : mjb::sentences(text)) {
    // "Hello, world. ", "Bye."
}

for(std::u16string_view word : mjb::words(u"Hello, world")) {
    // u"Hello", u",", u" ", u"world"
}

for(std::string_view line : mjb::line_segments("foo bar\nbaz")) {
    // "foo ", "bar\n", "baz": each keeps its trailing spaces and mandatory break
}
```

The ranges reject a temporary `std::string`, which a range-based `for` loop would destroy before
the loop body runs.

To cut text to a limit without splitting a segment, use `grapheme_truncate`,
`grapheme_truncate_width`, `word_truncate`, and `word_truncate_width`. They return a prefix view of
the input in its own code units:

```cpp
std::string_view prefix = mjb::grapheme_truncate("e\xCC\x81" "abc", 2); // "e\xCC\x81" "a"
std::string_view fits = mjb::word_truncate_width("Hello world", 6); // "Hello "
std::u16string_view flags = mjb::grapheme_truncate(u"A\U0001F1EE\U0001F1F9Z", 2); // 3 code units
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

`decode_next` and `decode_previous` step through a string one codepoint at a time from a byte
offset.

### Break positions

The breaker classes, `GraphemeBreaker`, `WordBreaker`, `SentenceBreaker`, and `LineBreaker`,
report every position between codepoints with its break type. `position` is a byte offset in the
input, and `codepoint` is the codepoint that starts there, `MJB_CODEPOINT_NOT_VALID` at the end of
the text. Use them when you need the no-break positions or the break types; to iterate the
segments themselves, use the ranges.

```cpp
mjb::LineBreaker breaker("a b");

while(std::optional<mjb::BreakResult> result = breaker.next()) {
    if(result->is_mandatory()) {
        // a mandatory break at result->position
    }
}
```

## Characters and properties

```cpp
mjb::Character character(0x00E9);
std::string_view name = character.name(); // "LATIN SMALL LETTER E WITH ACUTE"
std::string utf8 = character.to_utf8(); // "\xC3\xA9"
std::optional<mjb_codepoint> upper = character.uppercase();
std::optional<mjb::Character> unassigned = mjb::Character::from(0x0378); // std::nullopt

std::optional<mjb::BlockInfo> block = mjb::block_info(0x0041); // Basic Latin
std::optional<bool> alphabetic = mjb::property_binary(0x0041, MJB_PR_ALPHABETIC);
mjb_script script = mjb::script(0x0041);
bool emoji = mjb::is_emoji(0x1F642);

std::string encoded = mjb::encode(0x1F642, MJB_ENC_UTF_16LE); // 4 bytes: a surrogate pair
std::array<mjb_codepoint, 3> jamo = mjb::hangul_syllable_decompose(0xD55C); // U+1112 U+1161 U+11AB
```

`Character` throws for a codepoint the library cannot describe; `Character::from` returns
`std::nullopt` instead. When the library is built with `MJB_FEATURE_CHARACTER_NAMES=OFF`, `name()`
returns a placeholder such as `Codepoint U+00E9`.

## Bidirectional text

`BidiParagraph` resolves the embedding levels of a paragraph (UAX #9) and owns the result:

```cpp
mjb::BidiParagraph paragraph("English \xD7\xA2\xD7\x91\xD7\xA8\xD7\x99\xD7\xAA");
std::vector<size_t> visual_order = paragraph.visual_order(); // logical indices in display order
std::vector<mjb_bidi_run> runs = paragraph.line_runs();
uint8_t level = paragraph[0].level;
```

## Internationalized domain names

```cpp
mjb::IdnaResult ascii = mjb::idna_to_ascii("b\xC3\xBC" "cher.example");
if(ascii.valid()) {
    std::string domain = ascii.str(); // "xn--bcher-kva.example"
}

bool empty_label = mjb::idna_to_ascii("a..b").has_error(MJB_IDNA_ERROR_EMPTY_LABEL); // true
```

## Errors

Failures throw `mjb::LibraryError`, a `std::runtime_error`. `status()` returns the original
`mjb_status`, and `what()` names the failed operation followed by the library's message for that
status. `locale_parse` throws `mjb::LocaleError`, which derives from it, and
`mjb::status_message` returns the library's message for any status.

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

`mjb::Text`, the segment and codepoint ranges and their elements, the breakers, the truncation
results, and a `TextResult` whose `owned()` is false are views: the input must outlive them, as
with `std::string_view`. The functions that would always leave a dangling view reject a temporary
`std::string` at compile time, but a view of other temporary storage is still the caller's
responsibility.

## Allocation and versions

`mjb::set_allocator` installs custom allocation functions for the C library, which allocates the
`TextResult` output, `BidiParagraph` data, and temporary buffers. The strings that the wrapper
returns use the standard allocator.

`mjb::version()`, `mjb::version_number()`, and `mjb::unicode_version()` report the library and
Unicode versions.
