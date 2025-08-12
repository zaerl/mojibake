# TODO

## Main

- [ ] UTF-16 strings
- [ ] Add `mjb_casefold`
- [ ] Check maximum size of combining characters
- [ ] Optimize memory allocation in `mjb_normalize`
- [ ] Optimize memory allocation in `mjb_recompose`
- [ ] Add Github actions for testing
- [ ] Add support for Windows
- [ ] Add UTF16 support for `mjb_next_character`
- [ ] Add UTF16 support for `mjb_normalize`
- [ ] Add UTF16 support for `mjb_string_is_normalized`
- [ ] Add support for conditional mappings
- [ ] `CaseFolding.txt` table
- [ ] `WordBreakProperty.txt` table
- [ ] `SpecialCasing.txt` table
- [ ] `auxiliary/GraphemeBreakProperty.txt` table
- [ ] `auxiliary/SentenceBreakProperty.txt` table
- [ ] `auxiliary/WordBreakProperty.txt` table
- [x] `UnicodeData.txt` table
- [x] `Blocks.txt` table
- [x] Composition table
- [x] Decomposition table
- [x] Compatibility decomposition table
- [x] Codepoint to character transformation
- [x] UTF-8 strings
- [x] Remove `mjb_string_next_codepoint`
- [x] Remove `sqlite3.h` import on `mojibake.h`

## Unicode Conformance Requirements

See https://www.unicode.org/versions/Unicode16.0.0/core-spec/chapter-3/#G29705

**C1** A process shall not interpret a high-surrogate code point or a low-surrogate code point
as an abstract character.

**C2** A process shall not interpret a noncharacter code point as an abstract character.

**C3** A process shall not interpret an unassigned code point as an abstract character.

**C4** A process shall interpret a coded character sequence according to the character semantics
established by this standard, if that process does interpret that coded character sequence.

**C5** A process shall not assume that it is required to interpret any particular coded character
sequence.

**C6** A process shall not assume that the interpretations of two canonical-equivalent character
sequences are distinct.

**C7** When a process purports not to modify the interpretation of a valid coded character sequence,
it shall make no change to that coded character sequence other than the possible replacement of
character sequences by their canonical-equivalent sequences.

**C8** When a process interprets a code unit sequence which purports to be in a Unicode character
encoding form, it shall interpret that code unit sequence according to the corresponding code point
sequence.

**C9** When a process generates a code unit sequence which purports to be in a Unicode character
encoding form, it shall not emit ill-formed code unit sequences.

**C10** When a process interprets a code unit sequence which purports to be in a Unicode character
encoding form, it shall treat ill-formed code unit sequences as an error condition and shall not
interpret such sequences as characters.

**C11** When a process interprets a byte sequence which purports to be in a Unicode character
encoding scheme, it shall interpret that byte sequence according to the byte order and
specifications for the use of the byte order mark established by this standard for that character
encoding scheme.

**C12** A process that displays text containing supported right-to-left characters or embedding
codes shall display all visible representations of characters (excluding format characters) in the
same order as if the Bidirectional Algorithm had been applied to the text, unless tailored by a
higher-level protocol as permitted by the specification.

**C13** A process that produces Unicode text that purports to be in a Normalization Form shall do so
in accordance with the specifications in Section 3.11, Normalization Forms.

**C14** A process that tests Unicode text to determine whether it is in a Normalization Form shall
do so in accordance with the specifications in Section 3.11, Normalization Forms.

**C15** A process that purports to transform text into a Normalization Form must be able to produce
the results of the conformance test specified in Unicode Standard Annex #15, “Unicode Normalization
Forms.”

**C16** Normative references to the Unicode Standard itself, to property aliases, to property value
aliases, or to Unicode algorithms shall follow the formats specified in Section 3.1, Versions of the
Unicode Standard.

**C17** Higher-level protocols shall not make normative references to provisional properties.

**C18** If a process purports to implement a Unicode algorithm, it shall conform to the
specification of that algorithm in the standard, including any tailoring by a higher-level protocol
as permitted by the specification.

**C19** The specification of an algorithm may prohibit or limit tailoring by a higher-level
protocol. If a process that purports to implement a Unicode algorithm applies a tailoring, that fact
must be disclosed.

**C20** An implementation that purports to support Default Case Conversion, Default Case Detection,
or Default Caseless Matching shall do so in accordance with the definitions and specifications in
Section 3.13, Default Case Algorithms.
