# Legalese

This will be very boring if you, or your LLM, will dare to read it.

## Unicode References

Mojibake's Unicode data and algorithm references are scoped to
[The Unicode Standard, Version 18.0.0](https://www.unicode.org/versions/Unicode18.0.0/)
and the [Unicode Character Database 18.0.0](https://www.unicode.org/Public/18.0.0/).
Normative algorithm references here and in [https://github.com/zaerl/mojibake/blob/main/API.md](API.md)
use the archived Unicode 18.0.0 versions of the applicable annexes and synchronized technical standards:

- [UAX #44: Unicode Character Database, Unicode 18.0.0](https://www.unicode.org/reports/tr44/tr44-36.html)
- [UAX #9: Unicode Bidirectional Algorithm, Unicode 18.0.0](https://www.unicode.org/reports/tr9/tr9-51.html)
- [UAX #11: East Asian Width, Unicode 18.0.0](https://www.unicode.org/reports/tr11/tr11-45.html)
- [UAX #14: Unicode Line Breaking Algorithm, Unicode 18.0.0](https://www.unicode.org/reports/tr14/tr14-56.html)
- [UAX #15: Unicode Normalization Forms, Unicode 18.0.0](https://www.unicode.org/reports/tr15/tr15-57.html)
- [UAX #29: Unicode Text Segmentation, Unicode 18.0.0](https://www.unicode.org/reports/tr29/tr29-48.html)
- [UAX #31: Unicode Identifiers and Syntax, Unicode 18.0.0](https://www.unicode.org/reports/tr31/tr31-44.html)
- [UTS #10: Unicode Collation Algorithm, Unicode 18.0.0](https://www.unicode.org/reports/tr10/tr10-54.html)
- [UTS #39: Unicode Security Mechanisms, Unicode 18.0.0](https://www.unicode.org/reports/tr39/tr39-33.html)
- [UTS #51: Unicode Emoji, Unicode 18.0.0](https://www.unicode.org/reports/tr51/tr51-30.html)

Generic Unicode links, when present, are informational or download links rather than normative
conformance references.

## Unicode Conformance Requirements

See https://www.unicode.org/versions/Unicode18.0.0/core-spec/chapter-3/#G29705

### C1

> A process shall not interpret a high-surrogate code point or a low-surrogate code point as an
abstract character.

✅ Satisfied. The common UTF encoders/decoders reject surrogate code points,
`mjb_codepoint_info` does not return a character for `U+D800`/`U+DFFF`, and public collation
input rejects surrogate-shaped CESU-8 byte sequences.

### C2

> A process shall not interpret a noncharacter code point as an abstract character.

✅ Satisfied. `mjb_codepoint_is_valid` rejects `U+FDD0..U+FDEF` and `U+nFFFE/U+nFFFF`, and
`mjb_codepoint_info` returns `MJB_STATUS_INVALID_ARGUMENT` for noncharacters. Encoding helpers
may serialize noncharacters as code points, but the character metadata APIs do not treat them as
abstract characters.

### C3

> A process shall not interpret an unassigned code point as an abstract character.

✅ Satisfied. Unassigned samples such as `U+0378` return `MJB_STATUS_NOT_FOUND` from
`mjb_codepoint_info`. Algorithms use default property values where required, but Mojibake does
not report reserved code points as assigned characters.

### C4

> A process shall interpret a coded character sequence according to the character semantics
established by this standard, if that process does interpret that coded character sequence.

✅ Satisfied. `README.md` and `API.md` scope Mojibake's Unicode interpretation surface to the
documented public APIs and supported UTF encodings. The conformance inventory maps each advertised
Unicode algorithm or data claim to its Unicode 18.0.0 reference and test evidence. Mojibake does not
render text or interpret standardized or ideographic variation sequences as glyph-selection
requests. `mjb_string_emoji_sequence` recognizes assigned emoji variation sequences from
`emoji-variation-sequences.txt`. Misplaced variation selectors and unsupported variation sequences
are not interpreted as requests to modify the glyph of a preceding character.

### C5

> A process shall not assume that it is required to interpret any particular coded character
sequence.

✅ Satisfied. APIs can return `MJB_STATUS_NOT_FOUND`, `false`, or replacement markers for
unsupported or invalid input, and private-use / unassigned code points are not forced into assigned
character semantics.

### C6

> A process shall not assume that the interpretations of two canonical-equivalent character
sequences are distinct.

✅ Satisfied. `mjb_normalize` implements NFC/NFD/NFKC/NFKD, `mjb_nfkc_casefold` applies the
mandatory final NFC step, and `mjb_collation_compare` normalizes to NFD before building UCA collation
elements. A direct probe of `U+00E9` versus `U+0065 U+0301` compares equal and normalizes to the
expected canonical forms.

### C7

> When a process purports not to modify the interpretation of a valid coded character sequence,
it shall make no change to that coded character sequence other than the possible replacement of
character sequences by their canonical-equivalent sequences.

✅ Satisfied for the APIs that purport to preserve interpretation. Encoding conversion changes only
the encoding form for valid input, normalization APIs explicitly produce Unicode normalization
forms, and filtering/case APIs are documented as transformations that can change text.

### C8

> When a process interprets a code unit sequence which purports to be in a Unicode character
encoding form, it shall interpret that code unit sequence according to the corresponding code point
sequence.

✅ Satisfied. The UTF iterator resolves generic UTF-16/UTF-32 and BOM-derived combined flags to a
single explicit endian form before decoding. Generic UTF-16/UTF-32 without byte-order information is
treated as a decode error, while explicit UTF-8, UTF-16LE/BE, and UTF-32LE/BE continue to decode
according to their corresponding code point sequences.

### C9

> When a process generates a code unit sequence which purports to be in a Unicode character
encoding form, it shall not emit ill-formed code unit sequences.

✅ Satisfied. `mjb_codepoint_encode` rejects surrogate code points and values above `U+10FFFF`
before emitting UTF-8, UTF-16, or UTF-32, and tests cover surrogate rejection and boundary encoding
for every supported output form.

### C10

> When a process interprets a code unit sequence which purports to be in a Unicode character
encoding form, it shall treat ill-formed code unit sequences as an error condition and shall not
interpret such sequences as characters.

✅ Satisfied. `mjb_collation_key` returns `MJB_STATUS_MALFORMED_INPUT` for malformed UTF-8.
`mjb_collation_compare` uses the same sort-key path and returns its existing failure signal instead of
producing collation weights for malformed input.

### C11

> When a process interprets a byte sequence which purports to be in a Unicode character
encoding scheme, it shall interpret that byte sequence according to the byte order and
specifications for the use of the byte order mark established by this standard for that character
encoding scheme.

✅ Satisfied. BOM-derived `mjb_detect_encoding` values decode through the resolved byte order.
Generic UTF-16/UTF-32 input consumes a leading BOM as the encoding scheme signature, while
explicit-endian input preserves an initial `U+FEFF` as text. Generic UTF-16/UTF-32 conversion
without a BOM, and generic UTF-16/UTF-32 output, are rejected because the byte order is not
specified.

### C12

> A process that displays text containing supported right-to-left characters or embedding
codes shall display all visible representations of characters (excluding format characters) in the
same order as if the Bidirectional Algorithm had been applied to the text, unless tailored by a
higher-level protocol as permitted by the specification.

✅ Satisfied for the library's scope. Mojibake is not a renderer, but the provided bidi APIs
implement UAX #9 paragraph resolution, line reordering, and visual runs, and the tests consume both
`BidiCharacterTest.txt` and `BidiTest.txt`.

### C13

> A process that produces Unicode text that purports to be in a Normalization Form shall do so
in accordance with the specifications in Section 3.11, Normalization Forms.

✅ Satisfied. `mjb_normalize` implements canonical/compatibility decomposition, canonical ordering,
canonical composition, and Hangul normalization behavior for NFC/NFD/NFKC/NFKD.

### C14

> A process that tests Unicode text to determine whether it is in a Normalization Form shall
do so in accordance with the specifications in Section 3.11, Normalization Forms.

✅ Satisfied. `mjb_normalization_quick_check` implements the Unicode quick-check approach using
normalization quick-check data and canonical combining class ordering, with `MJB_QC_MAYBE` exposed
when a full normalization pass is required.

### C15

> A process that purports to transform text into a Normalization Form must be able to produce
the results of the conformance test specified in Unicode Standard Annex #15, “Unicode Normalization
Forms.”

✅ Satisfied. `tests/normalization.c` reads `utils/generate/unicode-data/UCD/NormalizationTest.txt`
and checks the full NFC/NFD/NFKC/NFKD closure rules against `mjb_normalize`.

### C16

> Normative references to the Unicode Standard itself, to property aliases, to property value
aliases, or to Unicode algorithms shall follow the formats specified in Section 3.1, Versions of the
Unicode Standard.

✅ Satisfied. `README.md`, `API.md` and `CONFORMANCE_REQUIREMENTS.md` state that normative Unicode
references are scoped to the Unicode Standard, Version 18.0.0 and UCD 18.0.0. Generated API metadata
emits archived Unicode 18.0.0 UAX/UTS links for algorithm and property references, and public header
comments name Unicode 18.0.0 where they reference UAX/UTS material.

### C17

> Higher-level protocols shall not make normative references to provisional properties.

✅ Satisfied. The repository does not define a higher-level protocol, and the public API does not
normatively require provisional Unicode properties. It exposes UCD-derived data and named
properties, but no protocol-level dependency on provisional properties was found.

### C18

> If a process purports to implement a Unicode algorithm, it shall conform to the
specification of that algorithm in the standard, including any tailoring by a higher-level protocol
as permitted by the specification.

✅ Satisfied. `README.md`, `API.md` and `CONFORMANCE_REQUIREMENTS.md` include a Unicode conformance
inventory that maps every advertised Unicode algorithm or data claim to its versioned Unicode 18.0.0
reference and test evidence. The inventory distinguishes official Unicode conformance suites from
normative data file checks and local regression coverage. This includes every explicit Unicode 18
`NFKC_CF` mapping from `DerivedNormalizationProps.txt`, every UTS #39 skeleton mapping from
`confusables.txt`, and the documented collation filtering for surrogate-code-point rows.

### C19

> The specification of an algorithm may prohibit or limit tailoring by a higher-level
protocol. If a process that purports to implement a Unicode algorithm applies a tailoring, that fact
must be disclosed.

✅ Satisfied. `README.md`, `API.md` and `CONFORMANCE_REQUIREMENTS.md` include a Unicode tailoring
section. It discloses that `mjb_map_case` is locale-sensitive through process-global `mjb_locale_set`,
that `MJB_LOCALE_EN` is the default, that Turkish/Azerbaijani tailor dotted-I casing and Turkic case
folding, that Lithuanian tailors dot-above casing only, and that the other advertised Unicode
algorithms are not locale-tailored by Mojibake. Generated function metadata now carries the same
disclosure on `mjb_map_case` and `mjb_locale_set`.

### C20

> An implementation that purports to support Default Case Conversion, Default Case Detection,
or Default Caseless Matching shall do so in accordance with the definitions and specifications in
Section 3.13, Default Case Algorithms.

✅ Satisfied. Uppercase/lowercase/case folding are backed by UnicodeData, `SpecialCasing.txt`,
`CaseFolding.txt`, and locale-sensitive casing is disclosed under C19. Default titlecase uses the
UAX #29 word-break iterator: the first cased character after each word boundary is titlecased, and
subsequent characters before the next word boundary are lowercased. Focused tests cover internal
apostrophes, case-ignorable characters, and Cherokee folding stability. `mjb_nfkc_casefold`
implements Unicode R5 by repeatedly applying NFKC, full default case folding, and removal of
`Default_Ignorable_Code_Point` characters until stable, followed by NFC. `tests/normalization.c`
checks every explicit Unicode 18 `NFKC_CF` mapping from `DerivedNormalizationProps.txt`. Mojibake
does not expose a separate *Default Case Detection* API.

## Unicode Tailoring

Unless listed here, Mojibake applies the referenced Unicode 18.0.0 algorithms *without*
higher-level protocol tailoring.

- **Case conversion and case folding**: `mjb_map_case` is locale-sensitive through the process-global
  locale set by `mjb_locale_set`. The default locale is `MJB_LOCALE_EN`. `MJB_LOCALE_TR` and
  `MJB_LOCALE_AZ` apply the Turkish/Azerbaijani dotted-I rules from `SpecialCasing.txt` for
  uppercase, lowercase, and titlecase, and the Turkic `T` mappings from `CaseFolding.txt` for full
  and simple case folding. `MJB_LOCALE_LT` applies Lithuanian dot-above rules from
  `SpecialCasing.txt` for uppercase, lowercase, and titlecase; case folding remains the default
  non-Turkic mapping.
- **Collation**: `mjb_collation_compare` and `mjb_collation_key` use DUCET without locale collation
  tailoring. The `mjb_collation_mode` argument only selects the UCA variable weighting strategy.
- **Display width**: `mjb_display_width` has an explicit `mjb_width_context` policy for East Asian
  Width `Ambiguous` characters. `mjb_codepoint_east_asian_width` itself reports the Unicode 18.0.0
  property value without tailoring.
- **Other Unicode algorithms**: normalization, NFKC case folding, bidirectional processing,
  grapheme/word/sentence/line breaking, identifier validation, confusable skeletons, and emoji
  sequence checks are not locale-tailored by Mojibake.

## Unicode conformance inventory

Mojibake interprets Unicode text only through the public APIs and supported UTF encodings listed in
this documentation. It does not implement *rendering*, *font shaping*, *locale collation tailoring*,
or *higher-level protocol* behavior beyond the documented locale-sensitive casing and display-width
policy. The table below maps the advertised Unicode algorithm and data claims to their Unicode
18.0.0 reference and test evidence.

| Claim | Public surface | Unicode reference | Evidence |
| --- | --- | --- | --- |
| Unicode Character Database data and derived properties | `mjb_codepoint_info`, `mjb_codepoint_property_binary`, `mjb_codepoint_property_int`, `mjb_codepoint_script_extensions`, script/block/category/numeric helpers | [UAX #44](https://www.unicode.org/reports/tr44/tr44-36.html), [UAX #24](https://www.unicode.org/reports/tr24/tr24-40.html), UCD 18.0.0 | Generated from UCD data files including `UnicodeData.txt`, `Blocks.txt`, `Scripts.txt`, `ScriptExtensions.txt`, `PropList.txt`, `DerivedCoreProperties.txt`, `PropertyAliases.txt`, and `PropertyValueAliases.txt`; every explicit Script_Extensions range is covered by `tests/properties.c`. |
| Unicode Normalization Forms and quick check | `mjb_normalize`, `mjb_normalization_quick_check` | [UAX #15](https://www.unicode.org/reports/tr15/tr15-57.html) | `NormalizationTest.txt`, `DerivedNormalizationProps.txt`, `tests/normalization.c`, and `tests/quick-check.c`. |
| Default case conversion and caseless matching | `mjb_map_case`, `mjb_nfkc_casefold`, simple codepoint case helpers | [Unicode Core Section 3.13](https://www.unicode.org/versions/Unicode18.0.0/core-spec/chapter-3/#G33992), [UAX #29](https://www.unicode.org/reports/tr29/tr29-48.html) for titlecase word boundaries, [UAX #31](https://www.unicode.org/reports/tr31/tr31-44.html) for identifier caseless matching | `SpecialCasing.txt`, `CaseFolding.txt`, `WordBreakTest.txt`, every explicit `NFKC_CF` mapping in `DerivedNormalizationProps.txt`, `tests/special-case.c`, `tests/case.c`, `tests/normalization.c`, and `tests/break-word.c`. |
| Grapheme, word, and sentence boundaries | `mjb_next_grapheme_break`, `mjb_next_word_break`, `mjb_next_sentence_break`, related truncation helpers | [UAX #29](https://www.unicode.org/reports/tr29/tr29-48.html) | `GraphemeBreakTest.txt`, `WordBreakTest.txt`, `SentenceBreakTest.txt`, `tests/segmentation.c`, `tests/break-word.c`, and `tests/break-sentence.c`. |
| Line breaking | `mjb_next_line_break` | [UAX #14](https://www.unicode.org/reports/tr14/tr14-56.html) | `LineBreakTest.txt` and `tests/break-line.c`. |
| Bidirectional Algorithm | `mjb_bidi_resolve`, `mjb_bidi_reorder_line`, `mjb_bidi_line_runs` | [UAX #9](https://www.unicode.org/reports/tr9/tr9-51.html) | `BidiCharacterTest.txt`, `BidiTest.txt`, `tests/bidi.c`, and `tests/bidi-class.c`. |
| Unicode Collation Algorithm, DUCET | `mjb_collation_compare`, `mjb_collation_key` | [UTS #10](https://www.unicode.org/reports/tr10/tr10-54.html) | `CollationTest_NON_IGNORABLE.txt`, `CollationTest_SHIFTED.txt`, and `tests/collation.c`; surrogate-code-point rows are filtered because public string input rejects ill-formed surrogate code points. |
| Unicode identifiers and pattern syntax data | ID/XID/pattern predicates and `mjb_string_is_identifier` | [UAX #31](https://www.unicode.org/reports/tr31/tr31-44.html) | UCD ID/XID and pattern properties from `DerivedCoreProperties.txt` and `PropList.txt`; covered by `tests/identifier.c`. |
| Confusable skeleton generation and matching | `mjb_confusable_skeleton`, `mjb_string_is_confusable` | [UTS #39](https://www.unicode.org/reports/tr39/tr39-33.html) | Every mapping in `confusables.txt`, every pair in `intentional.txt`, and `tests/security.c`. |
| Emoji properties and sequence data | Emoji property predicates, `mjb_string_emoji_sequence`, RGI checks | [UTS #51](https://www.unicode.org/reports/tr51/tr51-30.html) | `emoji-data.txt`, `emoji-sequences.txt`, `emoji-zwj-sequences.txt`, `emoji-variation-sequences.txt`, `emoji-test.txt`, and `tests/emoji.c`. |
| East Asian Width property | `mjb_codepoint_east_asian_width`; consumed by `mjb_display_width` | [UAX #11](https://www.unicode.org/reports/tr11/tr11-45.html) | `EastAsianWidth.txt`, `tests/east-asian-width.c`, and property tests; display column counts are a documented local policy over that property. |
