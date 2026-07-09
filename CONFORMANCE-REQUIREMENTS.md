# Unicode Conformance Requirements

See https://www.unicode.org/versions/Unicode17.0.0/core-spec/chapter-3/#G29705

**C1** A process shall not interpret a high-surrogate code point or a low-surrogate code point
as an abstract character.

✅ Satisfied. The common UTF encoders/decoders reject surrogate code points,
`mjb_codepoint_character` does not return a character for `U+D800`/`U+DFFF`, and public collation
input rejects surrogate-shaped CESU-8 byte sequences.

**C2** A process shall not interpret a noncharacter code point as an abstract character.

✅ Satisfied. `mjb_codepoint_is_valid` rejects `U+FDD0..U+FDEF` and `U+nFFFE/U+nFFFF`,
and `mjb_codepoint_character` returns `MJB_STATUS_INVALID_ARGUMENT` for noncharacters. Encoding
helpers may serialize noncharacters as code points, but the character metadata APIs do not treat
them as abstract characters.

**C3** A process shall not interpret an unassigned code point as an abstract character.

✅ Satisfied. Unassigned samples such as `U+0378` return `MJB_STATUS_NOT_FOUND` from
`mjb_codepoint_character`. Algorithms use default property values where required, but Mojibake does
not report reserved code points as assigned characters.

**C4** A process shall interpret a coded character sequence according to the character semantics
established by this standard, if that process does interpret that coded character sequence.

✅ Satisfied. `README.md` and `API.md` scope Mojibake's Unicode interpretation surface to the
documented public APIs and supported UTF encodings. The conformance inventory maps each advertised
Unicode algorithm or data claim to its Unicode 17.0.0 reference and test evidence.

**C5** A process shall not assume that it is required to interpret any particular coded character
sequence.

✅ Satisfied. APIs can return `MJB_STATUS_NOT_FOUND`, `false`, or replacement markers for
unsupported or invalid input, and private-use / unassigned code points are not forced into assigned
character semantics.

**C6** A process shall not assume that the interpretations of two canonical-equivalent character
sequences are distinct.

✅ Satisfied. `mjb_normalize` implements NFC/NFD/NFKC/NFKD, and `mjb_string_compare` normalizes to
NFD before building UCA collation elements. A direct probe of `U+00E9` versus `U+0065 U+0301`
compares equal and normalizes to the expected canonical forms.

**C7** When a process purports not to modify the interpretation of a valid coded character sequence,
it shall make no change to that coded character sequence other than the possible replacement of
character sequences by their canonical-equivalent sequences.

✅ Satisfied for the APIs that purport to preserve interpretation. Encoding conversion changes only
the encoding form for valid input, normalization APIs explicitly produce Unicode normalization
forms, and filtering/case APIs are documented as transformations that can change text.

**C8** When a process interprets a code unit sequence which purports to be in a Unicode character
encoding form, it shall interpret that code unit sequence according to the corresponding code point
sequence.

✅ Satisfied. The UTF iterator resolves generic UTF-16/UTF-32 and BOM-derived combined flags to a
single explicit endian form before decoding. Generic UTF-16/UTF-32 without byte-order information is
treated as a decode error, while explicit UTF-8, UTF-16LE/BE, and UTF-32LE/BE continue to decode
according to their corresponding code point sequences.

**C9** When a process generates a code unit sequence which purports to be in a Unicode character
encoding form, it shall not emit ill-formed code unit sequences.

✅ Satisfied. `mjb_codepoint_encode` rejects surrogate code points and values above `U+10FFFF`
before emitting UTF-8, UTF-16, or UTF-32, and tests cover surrogate rejection and boundary encoding
for every supported output form.

**C10** When a process interprets a code unit sequence which purports to be in a Unicode character
encoding form, it shall treat ill-formed code unit sequences as an error condition and shall not
interpret such sequences as characters.

✅ Satisfied. `mjb_collation_key` returns `MJB_STATUS_MALFORMED_INPUT` for malformed UTF-8.
`mjb_string_compare` uses the same sort-key path and returns its existing failure signal instead of
producing collation weights for malformed input.

**C11** When a process interprets a byte sequence which purports to be in a Unicode character
encoding scheme, it shall interpret that byte sequence according to the byte order and
specifications for the use of the byte order mark established by this standard for that character
encoding scheme.

✅ Satisfied. BOM-derived `mjb_string_encoding` values decode through the resolved byte order.
Generic UTF-16/UTF-32 input consumes a leading BOM as the encoding scheme signature, while
explicit-endian input preserves an initial `U+FEFF` as text. Generic UTF-16/UTF-32 conversion
without a BOM, and generic UTF-16/UTF-32 output, are rejected because the byte order is not
specified.

**C12** A process that displays text containing supported right-to-left characters or embedding
codes shall display all visible representations of characters (excluding format characters) in the
same order as if the Bidirectional Algorithm had been applied to the text, unless tailored by a
higher-level protocol as permitted by the specification.

✅ Satisfied for the library's scope. Mojibake is not a renderer, but the provided bidi APIs
implement UAX #9 paragraph resolution, line reordering, and visual runs, and the tests consume both
`BidiCharacterTest.txt` and `BidiTest.txt`.

**C13** A process that produces Unicode text that purports to be in a Normalization Form shall do so
in accordance with the specifications in Section 3.11, Normalization Forms.

✅ Satisfied. `mjb_normalize` implements canonical/compatibility decomposition, canonical ordering,
canonical composition, and Hangul normalization behavior for NFC/NFD/NFKC/NFKD.

**C14** A process that tests Unicode text to determine whether it is in a Normalization Form shall
do so in accordance with the specifications in Section 3.11, Normalization Forms.

✅ Satisfied. `mjb_string_is_normalized` implements the Unicode quick-check approach using
normalization quick-check data and canonical combining class ordering, with `MJB_QC_MAYBE` exposed
when a full normalization pass is required.

**C15** A process that purports to transform text into a Normalization Form must be able to produce
the results of the conformance test specified in Unicode Standard Annex #15, “Unicode Normalization
Forms.”

✅ Satisfied. `tests/normalization.c` reads `utils/generate/unicode-data/UCD/NormalizationTest.txt`
and checks the full NFC/NFD/NFKC/NFKD closure rules against `mjb_normalize`.

**C16** Normative references to the Unicode Standard itself, to property aliases, to property value
aliases, or to Unicode algorithms shall follow the formats specified in Section 3.1, Versions of the
Unicode Standard.

✅ Satisfied. `README.md` and `API.md` state that normative Unicode references are scoped to the
Unicode Standard, Version 17.0.0 and UCD 17.0.0. Generated API metadata emits archived Unicode
17.0.0 UAX/UTS links for algorithm and property references, and public header comments name
Unicode 17.0.0 where they reference UAX/UTS material.

**C17** Higher-level protocols shall not make normative references to provisional properties.

✅ Satisfied. The repository does not define a higher-level protocol, and the public API does not
normatively require provisional Unicode properties. It exposes UCD-derived data and named
properties, but no protocol-level dependency on provisional properties was found.

**C18** If a process purports to implement a Unicode algorithm, it shall conform to the
specification of that algorithm in the standard, including any tailoring by a higher-level protocol
as permitted by the specification.

✅ Satisfied. `README.md` and `API.md` include a Unicode conformance inventory that maps every
advertised Unicode algorithm or data claim to its versioned Unicode 17.0.0 reference and test
evidence. The inventory distinguishes official Unicode conformance suites from normative data file
checks and local regression coverage, including the documented collation filtering for
surrogate-code-point rows.

**C19** The specification of an algorithm may prohibit or limit tailoring by a higher-level
protocol. If a process that purports to implement a Unicode algorithm applies a tailoring, that fact
must be disclosed.

✅ Satisfied. `README.md` and `API.md` include a Unicode tailoring section. It discloses that
`mjb_case` is locale-sensitive through process-global `mjb_locale_set`, that `MJB_LOCALE_EN` is the
default, that Turkish/Azerbaijani tailor dotted-I casing and Turkic case folding, that Lithuanian
tailors dot-above casing only, and that the other advertised Unicode algorithms are not
locale-tailored by Mojibake. Generated function metadata now carries the same disclosure on
`mjb_case` and `mjb_locale_set`.

**C20** An implementation that purports to support Default Case Conversion, Default Case Detection,
or Default Caseless Matching shall do so in accordance with the definitions and specifications in
Section 3.13, Default Case Algorithms.

✅ Satisfied. Uppercase/lowercase/case folding are backed by UnicodeData,
`SpecialCasing.txt`, `CaseFolding.txt`, and locale-sensitive casing is disclosed under C19. Default
titlecase uses the UAX #29 word-break iterator: the first cased character after each word boundary
is titlecased, and subsequent characters before the next word boundary are lowercased. Focused tests
cover internal apostrophes and case-ignorable characters. Mojibake does not expose a separate
Default Case Detection API.
