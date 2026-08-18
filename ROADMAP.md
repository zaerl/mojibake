# Roadmap

This is the Mojibake roadmap. For great justice.

## Next steps

1. **Complete UTS #39 identifier checks**
   Generate compact tables from `IdentifierStatus.txt` and `IdentifierType.txt`, then add
   `mjb_identifier_check` and `mjb_string_restriction_level`.
2. **Detailed validation and decoder iteration**
   Add `mjb_string_validate`, `mjb_decode_next`, and `mjb_decode_previous`. Use a shared diagnostic
   result with the first failing byte (or code-unit offset?) and a precise malformed-input kind.
3. **Finish typed UCD access**
   Add typed getters for code-point, code-point-sequence, and string-valued properties. Follow with
   character age, bidi mirror, modern/alias/extended names, and reverse character-name lookup. Is
   this needed?
4. **Explicit locale operations**
   Implement the currently unsupported `mjb_locale_canonicalize` using a pinned IANA Language
   Subtag Registry snapshot.
5. **Reusable configurable collators**
   Introduce immutable DUCET collator objects that encapsulate strength and variable weighting,
   then add case ordering, numeric collation, and normalization options.
6. **Streaming processing**
   Add stateful `init`/`feed`/`finish` APIs for decoding and conversion, normalization, casing, and
   segmentation, using caller-kind of API.

## Non-goals

All the other things found on +10MB libraries.

# TODO

- [ ] Add generic binary search
- [ ] Check `mjb_confusable_skeleton_into` that alloc memory by using
  `mjb_confusable_skeleton_process`
- [ ] Check maximum size of combining characters
- [ ] Optimize memory allocation in `mjb_normalize`
- [ ] Optimize memory allocation in `mjb_recompose`
- [ ] Add more runtime options. See [HN:48945337](https://news.ycombinator.com/item?id=48945337)
- [ ] Add `vcpkg` support
- [ ] Check all the links such as https://www.unicode.org/reports/tr15/tr15-57.html if they will
  have a new Unicode 18 version, that right now they don't have
- [ ] Adopt one convention everywhere for fallible and buffer-producing APIs
- [ ] Check if there are functions that truncate a codepoint or a grapheme at the end of a buffer
- [ ] Add `mjb_utf8_snprintf` and others to the C++ API
- [ ] Show the script as a name on `CharacterDetails.swift`
- [ ] Add `Script_Extension` to `CharacterDetails.swift`
- [ ] Add Yes/No identifier rows for `ID_Start`, `ID_Continue`, `XID_Start`, `XID_Continue`,
  `Pattern_Syntax`, and `Pattern_White_Space` (maybe)
