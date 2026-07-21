# Roadmap

This is the Mojibake roadmap. For great justice.

## Next steps

1. **Resolved script sets**
   Add `mjb_string_script_set` by combining the `Script_Extensions` sets of every code point. See
   UTS #39 restriction levels.

2. **Complete UTS #39 identifier checks**
   Generate compact tables from `IdentifierStatus.txt` and `IdentifierType.txt`, then add
   `mjb_identifier_check` and `mjb_string_restriction_level`.

3. **Caseless comparison**
   Add `mjb_collation_compare_caseless` with default, canonical, compatibility, and identifier modes.

4. **Detailed validation and decoder iteration**
   Add `mjb_string_validate`, `mjb_decode_next`, and `mjb_decode_previous`. Use a shared diagnostic
   result with the first failing byte (or code-unit offset?) and a precise malformed-input kind.

5. **Finish typed UCD access**
   Add typed getters for code-point, code-point-sequence, and string-valued properties. Follow with
   character age, bidi mirror, modern/alias/extended names, and reverse character-name lookup. Is
   this needed?

6. **Explicit locale operations**
   Implement the currently unsupported `mjb_locale_canonicalize` using a pinned IANA Language
   Subtag Registry snapshot.

7. **UTS #46 IDNA**
   Implement nontransitional `mjb_idna_to_ascii` and `mjb_idna_to_unicode`, including Punycode,
   STD3, hyphen, bidi, and joiner checks. Validate against the complete `IdnaTestV2.txt` suite.

8. **Reusable configurable collators**
   Introduce immutable DUCET collator objects with strength, case, numeric, normalization, and
   variable-weighting options.

9. **Streaming processing**
   Add stateful `init`/`feed`/`finish` APIs for decoding and conversion, normalization, casing, and
   segmentation, using caller-kind of API.

## Non-goals

All the other things found on +10MB libraries.

# TODO

- [ ] Add generic binary search
- [ ] Check maximum size of combining characters
- [ ] Optimize memory allocation in `mjb_normalize`
- [ ] Optimize memory allocation in `mjb_recompose`
- [ ] Add more runtime options. See [HN:48945337](https://news.ycombinator.com/item?id=48945337)
- [ ] Add `vcpkg` support
- [ ] Check all the links such as https://www.unicode.org/reports/tr15/tr15-57.html if they will
have a new Unicode 18 version, that right now they don't have
