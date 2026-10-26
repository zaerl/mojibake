# Changelog

All notable changes to Mojibake are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- `mjb_nul_terminated_length` returns the byte length of a string that ends at its first NUL code
  unit, in any input encoding

### Changed
- `mjb_text_make` and the `mjb_text_*` helpers resolve a `MJB_NUL_TERMINATED` length when the text
  is created, so it is scanned once. The sentinel is kept when the buffer is NULL or the encoding is
  invalid, and the function that takes the text reports the error
- `mjb_decode_next` and `mjb_decode_previous` return `MJB_STATUS_INVALID_ARGUMENT` for a text whose
  `byte_length` is `MJB_NUL_TERMINATED`, like the break iterators. They rescanned the whole string
  for its terminator on every call, so iterating took quadratic time
- All internal decoding uses the whole-scalar decoder of `mjb_decode_next`, inlined. The streaming
  DFA decoder is removed. Decoding valid text is faster: about 2.4x for UTF-8, 2x for UTF-16 and
  1.4x for UTF-32 in a 2 MB mixed-script loop
- Break iterators on malformed input: each maximal subpart of a malformed subsequence is one
  U+FFFD with a position of its own, as in `mjb_decode_next`. A run of malformed bytes used to be
  merged into one U+FFFD, and an incomplete sequence at the end of the text was dropped
- Break iterators return `MJB_BT_NOT_SET` for an invalid encoding and for generic UTF-16 or UTF-32
  text without a byte order mark, instead of iterating over replacement characters
- The `state` and `in_error` members of `mjb_grapheme_break_state`, `mjb_word_break_state`,
  `mjb_sentence_break_state`, and `mjb_line_break_state` are removed. Callers only set `index`

### Fixed
- The Windows CLI interactive mode stopped accepting input after an unpaired surrogate key event.
  It decodes key events with `mjb_decode_next` and skips malformed surrogates. The internal
  `mjb_utf16_decode_step` helper and `src/utf16.h` are removed
- `mjb_grapheme_prefix_length`, `mjb_word_prefix_length`, and their `_for_width` variants
  estimated boundary offsets from the replacement codepoint and could cut at the wrong byte in
  malformed text. They use the exact offsets of the iterator
- The line break iterator now reports the LB3 mandatory break at the end of a text that holds only
  a byte order mark
- `mjb_bidi_resolve` byte offsets of multi-byte UTF-8 characters, and of the first character after a
  byte order mark, are the offset of the first byte
- Out-of-range and surrogate UTF-32 code units were silently dropped instead of being reported as
  malformed input, so functions such as `mjb_normalize` accepted them or passed them through
- A UTF-16 code unit following an unpaired high surrogate was swallowed together with the
  surrogate instead of being decoded on its own

## [0.5.1]
Codename: [LATIN CAPITAL LETTER Q]

### Added
- Added C++ examples
- C++: `char8_t` input and output, so `u8"..."` literals and `std::u8string` work with every
  function
- C++: option structs for designated initializers: `mjb::Options`, `mjb::CaseOptions`,
  `mjb::MatchOptions`, and `mjb::CollationOptions`
- C++: `grapheme_breaks`, `word_breaks`, `sentence_breaks`, and `line_breaks` ranges of
  `mjb::BreakResult`
- C++: `mjb::encoded(bytes, encoding)` output for transforms that write bytes in any encoding, and
  `convert_encoding<CharT>(input)`
- C++: `mjb::error_category()` and `mjb::make_error_code` for `mjb_status`
- C++: `BidiParagraph::chars()` as a `std::span`

### Changed
- New README.md
- New API.md
- C++: the wrapper requires C++20. The library still builds as C11 and C++17
- C++: every transform has two forms, `mjb::nfc(input, options)` and `mjb::nfc(input, out,
  options)`, which replace the `_into` and `_result` functions and `mjb::TextResult`. Transforms
  return the input's string type and encoding, and write the encoding of the output's string type.
  Optional arguments moved from positional parameters to the option structs, and the input and
  output encoding parameters were removed in favour of `mjb::Text` and `mjb::encoded`
- C++: segment and codepoint ranges are `std::ranges` forward, borrowed views with
  `std::default_sentinel_t` ends, so they compose with standard algorithms and adaptors. The
  `Breaker` classes were replaced by the break position ranges
- C++: functions that return views, including the truncation functions, are constrained by the
  `mjb::borrowed_text` concept, and Clang warns about a `Text` or view that outlives its input
- C++: `mjb::LibraryError` derives from `std::system_error`; `mjb::LocaleError` was removed
- C++: codepoint predicates are free functions only, and `Character::to_utf8` was removed in
  favour of `mjb::encode`. The C callback overload of `for_each_codepoint` and the
  `mjb_filter_flags` overloads of `filter` were removed
- C++: the native UTF-16 and UTF-32 encodings come from `std::endian::native`

### Fixed
- Removed obsolete header file

## [0.5.0]
Codename: [LATIN CAPITAL LETTER P]

### Added
- TypeScript: `graphemeSegments`, `wordSegments`, `sentenceSegments`, and `lineSegments` split the
  input at its boundaries
- C++: `_into` transforms that write into a caller's string and reuse its capacity, for the
  `std::string` and the UTF-16 and UTF-32 overloads: `normalize_into`, `nfc_into`, `nfd_into`,
  `nfkc_into`, `nfkd_into`, `filter_into`, `nfkc_casefold_into`, `case_map_into`,
  `uppercase_into`, `lowercase_into`, `titlecase_into`, `casefold_into`, `casefold_simple_into`,
  and `confusable_skeleton_into`, plus `convert_encoding_into`, `collation_key_into`,
  `idna_to_ascii_into`, and `idna_to_unicode_into`, which return the IDNA error flags
- `mjb_text` input structure with the `mjb_text_make`, `mjb_text_utf8`, `mjb_text_utf16le`,
  `mjb_text_utf16be`, `mjb_text_utf32le`, and `mjb_text_utf32be` helpers
- `mjb_buffer` output structure, holding caller-provided storage, its capacity or written size, and
  its encoding, with the `mjb_buffer_make` helper and `mjb_buffer_text` to pass written output to
  another call as input
- C++ support for UTF-16 and UTF-32 text in the platform's byte order (`mjb::native_utf16_encoding`
  and `mjb::native_utf32_encoding`
- `position` in `mjb_grapheme_break_state`, `mjb_word_break_state`, `mjb_sentence_break_state`,
  and `mjb_line_break_state`: the break iterators store the byte offset of each position they
  report, so a segment runs from one break position to the next

### Changed
- **Breaking**: `mjb_result.transformed` is now `mjb_result.owned`. It is true when the library
  allocated `output`; when false, `output` points into the caller's input. The C++
  `TextResult::transformed()` is now `owned()`
- **Breaking**: `mjb_string_validate(buffer, byte_length, encoding, diagnostic)` is now
  `mjb_validate_encoding(const mjb_text *text, diagnostic)`
- **Breaking**: all the functions that took a buffer, byte length, and encoding now takes a
  `const mjb_text*`
- **Breaking**: all the functions that took the output encoding, output buffer, and output size now
  takes a `mjb_buffer*`
- **Breaking**: the TypeScript `graphemeBreakNext`, `wordBreakNext`, `sentenceBreakNext`, and
  `lineBreakNext` return `BreakPosition[]`, a `{ position, type }` entry for each position, instead
  of the break types alone. `position` is the byte offset in the input's encoding, read from the
  iterator's `state.position`. The site places its break symbols by position
- **Breaking**: the TypeScript `convertEncoding(input, outputEncoding, options)` is now
  `convertEncoding(input, options)` and reads `options.outputEncoding` like the other
  transformations. The output encoding still defaults to UTF-8
- **Breaking**: `mjb_caseless_match`, `mjb_collation_compare`, and `mjb_confusable_match` take two
  `const mjb_text *` instead of two buffer, byte length, and encoding triples
- **Breaking**: `mjb_collation_compare`, `mjb_caseless_match` and `mjb_confusable_match` now takes a
  `malformed_policy` and a diagnostic
- **Breaking**: `mjb_map_case` and `mjb_map_case_into` take the casing locale as an argument after
  `type`
- **Breaking**: `MJB_LOCALE_NOT_SET` selects the default mappings, and an invalid locale returns
  `MJB_STATUS_INVALID_ARGUMENT`. Concurrent calls with different locales no longer interfere
- **Breaking**: removed `mjb_set_locale`, `mjb_get_locale`, and `mjb_reset_locale`; the library no
  longer has a process-global locale. The C++ `set_locale`, `get_locale`, and `reset_locale` and the
  TypeScript `setLocale` and `getLocale` are removed too. The C++ casing functions take the locale
  as the argument after the input (after `type` for `case_map`), the TypeScript `mapCase` reads
  `options.locale`, and the CLI `--locale` option is passed to each casing call
- **Breaking**: allocator callbacks receive block sizes, so arena, pool, and accounting allocators
  no longer need to record them: `mjb_realloc_fn` is now `(context, ptr, old_size, new_size)` and
  `mjb_free_fn` is now `(context, ptr, size)`. The size passed is the one most recently requested
  for the block; requested sizes are never 0, `realloc` receives NULL with an `old_size` of 0 for a
  new block, and `free` is never called with NULL
- **Breaking**: `mjb_result` has a new `capacity` field that records the allocation size of an owned
  `output` for `mjb_result_free`
- **Breaking**: the segmentation functions are named after their unit, so each unit's functions
  sort together: `mjb_next_grapheme_break`, `mjb_next_word_break`, `mjb_next_sentence_break`, and
  `mjb_next_line_break` are now `mjb_grapheme_break_next`, `mjb_word_break_next`,
  `mjb_sentence_break_next`, and `mjb_line_break_next`; their states `mjb_next_state`,
  `mjb_next_word_state`, `mjb_next_sentence_state`, and `mjb_next_line_state` are now
  `mjb_grapheme_break_state`, `mjb_word_break_state`, `mjb_sentence_break_state`, and
  `mjb_line_break_state`. The TypeScript API follows: `graphemeBreakNext` and so on
- **Breaking**: the truncation functions are named after what they return, the byte length of a
  prefix, since they do not modify their input: `mjb_truncate_grapheme`,
  `mjb_truncate_grapheme_width`, `mjb_truncate_word`, and `mjb_truncate_word_width` are now
  `mjb_grapheme_prefix_length`, `mjb_grapheme_prefix_length_for_width`, `mjb_word_prefix_length`,
  and `mjb_word_prefix_length_for_width`, and the TypeScript methods are now
  `graphemePrefixLength` and so on. The C++ wrappers return the truncated `std::string_view`, so
  they are named `mjb::grapheme_truncate`, `mjb::grapheme_truncate_width`, `mjb::word_truncate`,
  and `mjb::word_truncate_width`
- **Breaking**: `mjb_for_each_codepoint` takes a `void *context` after the callback and passes it
  to every call, so callbacks can keep their own state without global variables:
  `mjb_for_each_codepoint(text, malformed_policy, callback, context, diagnostic)`. The callback
  type is now `bool (*)(const mjb_character *character, mjb_character_position position,
  void *context)`; the character is `const` because callbacks only read it
- **Breaking**: C++ wrapper changes:
  - `mjb::for_each_codepoint` accepts any callable, including a lambda with captures, taking
    `(const mjb_character &, mjb_character_position)`; an exception it throws stops the iteration
    and is rethrown. The C-callback overload takes the `context` pointer after the callback
  - `normalize_result`, `filter_result`, and `convert_encoding_result` reject a temporary
    `std::string` at compile time, because their `TextResult` can borrow the input
  - `LibraryError::what()` appends the library's message for the status, and
    `mjb::status_message` exposes `mjb_status_message`
  - `mjb::BreakResult::index` is now `position`, the byte offset of the reported position. It
    held the iterator's internal `index`, which is not a boundary offset. `BreakResult::codepoint`
    is decoded at `position`, U+FFFD for a malformed subsequence and `MJB_CODEPOINT_NOT_VALID` at
    the end of the text; it was the iterator's internal current codepoint, which is the previous
    character after a rule absorbs a format or extend character, such as U+200B (SB5) or U+0301
    (WB4), and at the end of the text. The breaker classes take an `mjb::Text`
  - analysis functions take an `mjb::Text` instead of a `std::string_view` and an encoding
    parameter: `validate_encoding`, `codepoint_count`, `for_each_codepoint`, `grapheme_count`,
    `word_count`, `sentence_count`, `terminal_width`, `is_identifier`, `normalization_quick_check`,
    `resolved_script_set`, `emoji_sequence`, `is_emoji_sequence`, `is_rgi_emoji`, `locale_parse`,
    `caseless_match`, `confusable_match`, `collation_compare`, `collation_key`, and
    `collation_key_result`. UTF-8 callers drop the encoding argument; for another encoding, pass
    `mjb::Text(bytes, encoding)`
  - renamed to match the C API: `validate_string` to `validate_encoding`; `is_hangul_l`,
    `is_hangul_v`, and `is_hangul_t` to `is_hangul_leading_jamo`, `is_hangul_vowel_jamo`, and
    `is_hangul_trailing_jamo`; `is_cjk_extension` to `is_cjk_extension_ideograph` (also on
    `Character`); `codepoint_numeric_value` to `numeric_value`; `parse_locale` to
    `locale_parse`; and `compare` to `collation_compare`
- **Breaking**: `mjb_hangul_syllable_composition` is no longer public, and `mjb_buffer_character`
  moved out of `mojibake.h`
- **Breaking**: The new `mjb_hangul_syllable_compose(leading, vowel, trailing, syllable)`
  composes a syllable from its jamo, and `mjb_hangul_syllable_decomposition` is now
  `mjb_hangul_syllable_decompose`. The C++ wrapper follows with `hangul_syllable_compose` and
  `hangul_syllable_decompose`
- **Breaking**: the `_into` functions now write in a single pass instead of measuring first
- `make tidy` now runs clang-tidy with `.clang-tidy` over the library, CLI, and tests, replacing
  `cppcheck`

### Fixed
- `mojibake.h` accepts C++17 on MSVC without `/Zc:__cplusplus`. MSVC reports `__cplusplus` as
  `199711L` unless that flag is set, so projects including the header from C++ hit the "C++17 or a
  later version is required" error and lost `[[nodiscard]]`; the check now also reads
  `_MSVC_LANG`, which holds the selected standard
- `mjb_map_case`, `mjb_filter`, and `mjb_normalize` now return `MJB_STATUS_UNSUPPORTED` instead of
  `MJB_STATUS_NO_MEMORY` when the output encoding cannot represent a result codepoint, matching
  their `_into` variants
- Documented `MJB_STATUS_UNSUPPORTED` for every function whose output encoding can reject a
  codepoint, and `MJB_STATUS_INVALID_ENCODING` for `mjb_confusable_skeleton`
- CLI: the interactive screen signal handler now restores the cursor and terminal using only
  async-signal-safe calls
- CLI: `break` now prints the input bytes between the break positions the iterators report, so a
  format or extend character that a rule absorbs, such as U+200B in `break sentence "a<U+200B>b"`,
  no longer replaces itself with a copy of the previous character (`÷aab÷`)
- Tests: the CLI tests in `tests/shell/` now run under CTest; their `assert-command.cmake` helper
  was missing, so `break-json.cmake` never ran
- Fixed all clang-tidy findings in the library, CLI, and tests
- Site: the malformed policy is now passed through the API options, so the arguments that follow
  it, such as the normalization form, are no longer shifted
- Site: `mjb_convert_encoding` now uses the selected output encoding
- Tests: the `CollationTest_SHIFTED.txt` conformance check compared each string with itself since
  the `mjb_collation_key` conversion, so it could not fail; it now compares the two strings again

## [0.4.2] - 2026-10-12
Codename: [LATIN CAPITAL LETTER B]

### Added
- Added a test checking every explicitly named codepoint of `UnicodeData.txt` (with
  `NameAliases.txt` control and correction aliases) against `mjb_codepoint_info`
- New data layout for codepoint names
- New locale functions
- Added `.clang-tidy`
- CLI: added locale and direction
- CLI: new `nfkc-casefold` command
- CLI: `char` now prints the script in verbose mode

### Changed
- `mjb_locale` gained `MJB_LOCALE_NOT_SET` as value 0
- Character names as page-prefixed token streams over a word lexicon
- Sequence lookups in `src/unicode-tables.h` now decode into caller buffers of
  `MJB_UNICODE_SEQUENCE_MAX` codepoints
- Moved the `unicode-data` folder to project root
- Forced CTest line
- Moved `update-version` script to C++
- Minor site UI changes

### Fixed
- Fixed all obsolete URLs
- Removed obsolete files
- Fixed multiple lint errors

## [0.4.1] - 2026-09-21
Codename: [LATIN CAPITAL LETTER A]

### Added
- Added `make tidy`
- Added GitHub static analysis job

### Changed
- Removed `CLAUDE.md` (now deprecated)

### Fixed
- Fixed multiple `cppcheck` warnings and errors
- Fixed CHANGELOG typo

## [0.4.0] - 2026-09-16
Codename: [COMMERCIAL AT]

### Added
- Added "malformed policy" (stop, replace, skip)
- Added `mjb_string_validate` function
- Added `mjb_decode_next` function
- Added `mjb_decode_previous` function
- Added `mjb_word_count` function
- Added support for Haiku R1/beta6

### Changed
- Updated to the final Unicode 18.0.0 release
- Now all functions that parse text accept a malformed policy
- `mjb_count_codepoints` is now `mjb_codepoint_count`. It now returns an `mjb_status` and stores the
  count in a `size_t *count` out parameter. C++ `mjb::length` is now `mjb::codepoint_count`
- Renamed TypeScript API `countCodepoints` to `codepointCount`
- Replaced `mjb_set_memory_functions` with the context-aware `mjb_set_allocator`
- Renamed `mjb_reset` to `mjb_reset_locale`. C++ is now `mjb::reset_locale`
- Updated Attractor

### Fixed
- Multiple S&Q fixes

## [0.3.6] - 2026-08-16
Codename: [DIGIT SIX]

### Added
- Added new `mjb_grapheme_count` function
- Added new `mjb_sentence_count` function
- Added new macOS views

### Changed
- Updated to Unicode 18 2026-08 draft
- Renamed `mjb_for_each_character` to `mjb_for_each_codepoint`
- Renamed `mjb_for_each_character_fn` to `mjb_for_each_codepoint_fn`
- Renamed `_mjbForEachCharacterCallback` to `_mjbForEachCodepointCallback`
- Renamed `mjb_are_confusable` to `mjb_confusable_match`
- Renamed `mjb_classify_emoji_sequence` to `mjb_emoji_sequence_info`
- WASM: renamed `forEachCharacter` to `forEachCodepoint`
- WASM: renamed `forEachCharacter` to `forEachCodepoint`
- WASM: renamed `areConfusable` to `confusableMatch`
- Reorganized macOS sidebar
- Temporarily disabled Haiku R1/beta5 until beta 6 is released

### Fixed
- Minor regressions discovered after the new Unicode 18 draft

## [0.3.5] - 2026-07-30
Codename: [DIGIT FIVE]

### Added
- Added macOS UI initial support
- Added mjb_status_message message
- Added Zig example
- Added C++ example
- Added Python 3 example
- Added Rust example

### Changed
- Removed obsolete `mjb_error` from `mjb_locale_parse` function
- More strict compiler flags
- Renamed `mjb_display_width` to `mjb_terminal_width` and replaced
  `mjb_width_context` with explicit narrow and East Asian terminal-width profiles
- Renamed the WASM `displayWidth` method and shell `display_width` field to `terminalWidth` and
  `terminal_width`

### Fixed
- Removed hardcoded team code
- Added missing function in the buzz file
- Minor documentation fixes

## [0.3.4] - 2026-07-28
Codename: [DIGIT FOUR]

### Added
- Added `mjb_caseless_match`
- Added `mjb_collation_key`
- Added `mjb_resolved_script_set`
- Added `mjb_idna_to_ascii`
- Added `mjb_idna_to_ascii_into`
- Added `mjb_idna_to_unicode`
- Added `mjb_idna_to_unicode_into`
- Added `MJB_FEATURE_COLLATION` to compile out Unicode collation support and DUCET tables
- Added `MJB_FEATURE_IDNA` to compile out IDNA and Punycode implementations and mapping tables
- Added `MJB_FEATURE_SECURITY` to compile out UTS #39 security support and confusable mapping tables
- Added release codenames
- Shell: added maximum accepted argument count

### Changed
- A slightly smaller DUCET representation
- Added `HANB`, `JPAN`, `KORE` augmented script sets to `mjb_script`
- Changed `mjb_collation_mode` type to `mjb_collation_variable_weighting` to avoid confusion
- Added optimization flags to examples
- `mjb_collation_compare`, `mjb_collation_key`, `mjb_collation_key_into` now accept a
  `variable_weighting`, the maximum UCA comparison level
- Reorganized the HTML handling of C arguments
- Shell: renamed the Uppercase, Lowercase, Titlecase to "Simple * Mapping", to not duplicate the
  preexisting "upper", etc.

### Fixed
- The code compiled with `-O3 -DNDEBUG` was removing the codepoint declared in the IDNA loop
- Wrong index, U+0000 was appended to normalized content
- Shell: disabled colors when JSON is selected
- Shell: the `nfkc` command was producing NKFD content
- Shell: now returns 1 when a user doesn't select a command
- Shell: the `bidi` command now returns 1 for an error
- Shell: `normalize`, `break` and `case` commands now output JSON if JSON is selected

## [0.3.3] - 2026-07-22
Codename: [DIGIT THREE]

### Added
- Added `MJB_NUL_TERMINATED` (`byte_length`) for encoding-aware NUL-terminated input strings
- Added `_into(...)` function alternatives to use a user-provided buffer to store results
- Added `mjb_utf8_snprintf`, `mjb_utf8_vsnprintf`, a `snprintf` that does not break encoding
- Added `mjb_utf8_grapheme_snprintf`, `mjb_utf8_grapheme_vsnprintf` that do not break graphemes
- New API documentation

### Changed
- Explicitly sized input strings now always process embedded `U+0000` codepoints
- Removed the `MJB_ALLOW_EMBEDDED_NULLS` build option and
  `MJB_DANGEROUSLY_ALLOW_EMBEDDED_NULLS` compile-time switch

### Fixed
- Missing WASM name on site

## [0.3.2] - 2026-07-21
Codename: [DIGIT TWO]

### Added
- Added to the C++ wrapper the missing C API
- Added C++ README

### Changed
- Functions like `mjb_normalization_quick_check` that can generate an error now return a
  `mjb_status` and accept a pointer to save the result
- Renamed `mjb_filter` enum to `mjb_filter_flags` to avoid confusion
- `mjb_shutdown` is now `mjb_reset`
- New locale functions `mjb_set_locale` and `mjb_get_locale`
- `mjb_string_is_confusable` is now `mjb_are_confusable`
- `mjb_break_*` functions are now `mjb_next_*_break`
- `mjb_string_length` is now `mjb_count_codepoints` to avoid confusion, and other functions
- `mjb_string_is_normalized` is now `mjb_normalization_quick_check` to avoid confusion
- Added to the C++ wrapper the missing C API
- Removed the `string_` from `mjb_string_*` kind of functions. Such as `mjb_string_is_identifier`
  that now is `mjb_is_identifier`

### Fixed
- Windows: fix MSVC C4310 warnings
- `mjb_set_locale` now accepts an enum, and not an integer
- Fix favicons main color
- Minor doc typos

## [0.3.1] - 2026-07-20
Codename: [DIGIT ONE]

### Added
- Added a `shell.c` amalgamation to the main amalgamation. Now there are three files
- Added node smoke test to test targets
- Added HTML smoke test to test targets
- Added minimum supported compilers to test targets
- Added Linux ARM64 / GCC to test targets
- Made CMake embedding dependency-safe by using namespaced options, target-scoped build flags, and
  top-level-only defaults for tests, the CLI, and installation rules
- Monday Dependabot cleanup

### Changed
- Forced `_WIN32` in the shell `getoptc.h/c` declarations
- Removed all shell headers and kept only `shell.h`
- Replaced `BUILD_SHARED` with CMake's standard `BUILD_SHARED_LIBS` option
- Now the LICENSE file is embedded in the amalgamation files
- Minor reorganization of utils/generate folder
- Fixed C4 statement

### Fixed
- Windows: `findstr` wasn't working in the `generate.bat`. I tried multiple times to make it work
  but then I gave up and changed it to a Node script
- Windows: Makefile.nmake syntax error on `GENERATE_SOURCES` declaration
- Changed check from `WIN32` to `MSVC` to identify the Microsoft compiler
- MingW: now we check if `STDOUT_FILENO` is declared before setting it
- Set exit code on WASM test to 1 after failure
- Removed the MJB_EXPORT from `mjb_string_output` and `mjb_string_output_codepoint`
- Missing Haiku on the OSes list

## [0.3.0] - 2026-07-18
Codename: [DIGIT ZERO]

### Added
- Moved to Unicode 18 beta
- Added Haiku test action
- Added `MJB_BUILD_CLI` option
- CMake changes now added `BUILD_TESTING` option

### Changed
- Now the generation script downloads the entire security zip
- Removed `enable_testing()` and added `include(CTest)`
- Added a supplementary 64-bit table for case entries outside BMP

### Fixed
- Critical: removed `utils/` from Git export-ignore
- Changed fuzz/corpus/** files to "binary"

## [0.2.8] - 2026-07-16
Codename: [LEFT PARENTHESIS]

### Added
- Added FreeBSD, OpenBSD, NetBSD test flows
- Created CODEOWNERS
- Updated documentation
- WASM section rework

### Changed
- Removed mjb_codepoint_to_lowercase to avoid confusion
- Removed mjb_codepoint_to_uppercase to avoid confusion
- Removed mjb_codepoint_to_titlecase to avoid confusion
- Dependabot update

### Fixed
- Added missing permissions to test flow
- Missing WASM declarations
- Minor test typos

## [0.2.7] - 2026-07-16
Codename: [APOSTROPHE]

### Added
- Added break algorithms HTML output
- Added custom Dependabot
- Added dependency review GitHub workflow
- Added "Quality" GitHub workflow that runs `clang-format`
- Added `mjb_encoding_is_valid_input`
- Added `mjb_encoding_is_valid_output`

### Changed
- UCD names data tables to a new schema
- Converted 6 sparse page indexes from 16-bit to 8-bit entries
- Packed collation elements into 4 bytes instead of 6
- Packed NUL-terminated strings on names table
- Updated version script
- Added function signatures to `API.md`
- Moved TODO.md to the more serious `ROADMAP.md`
- Applied Clang format to all source files
- Minor documentation refactoring

### Fixed
- Removed obsolete `free_result` function
- Removed obsolete `FREE_RESULT` macro
- Removed `examples/example` binary
- Run confusable tests only on static builds
- On break sentence SB2 Any ÷ eot

## [0.2.6] - 2026-07-13
Codename: [AMPERSAND]

### Added
- Forced clang-format rules from now on
- Reformatted all files
- Windows: added "Test (embedded NULLs)" flow
- Windows: added "Test (C++)" flow
- Changed site color tone map to something more modern
- Added small examples for all functions
- Added `ROADMAP.md`
- Added `mjb_codepoint_script_extensions` algorithm
- Added `mjb_confusable_skeleton` algorithm
- Added `mjb_nfkc_casefold` algorithm
- HTML: added bitfield for `mjb_case`

### Changed
- Renamed `doc/` to `example/`
- Moved legalese to conformance file
- Removed fuzz paragraph
- Fix: `Mojibake.codepointEncode` wrong signature
- Fix: `Mojibake.stringLength` wrong signature
- Inverted `mjb_normalize`, `mjb_map_case` and `mjb_string_filter` parameters
- Split `mjb_codepoint_property_value` in `mjb_codepoint_property_binary`,
  `mjb_codepoint_property_int`
- Minor functions reordering

### Fixed
- Windows C++: MSVC wrong default code page
- Windows/Attractor: missing `reinterpret_cast`
- C++/Linux: fixed `missing-field-initializer`
- Linux: fix format-truncation warning
- OpenBSD: fixed "collation entries" algorithm (+10 minutes)
- Missing include in `unicode-tables.h`

## [0.2.5] - 2026-07-11
Codename: [PERCENT SIGN]

### Added
- Limit combining characters filter
- New documentation
- Added docs to functions list
- New site from .md files
- New emojis
- HTML C code is now highlighted
- Added governance file
- Added OpenSSF Best Practices badge
- New `mjb_result_free` function
- Added Code of Conduct

### Changed
- Site is generated from README.md
- Now WASM returns `mjb_result` instead of a string
- Now WASM strings also have a `Uint8Array`
- Functions reordered
- Added Developer Certificate of Origin

### Fixed
- Minor typos
- HTML code indenting
- Missing HTML block parser
- Not-WASM functions were missing on the site

## [0.2.4] - 2026-07-09
Codename: [DOLLAR SIGN]

### Added
- Added Unicode Conformance Requirements conformance
- Added `details`, `returns`, `example`, `related`, `specs`, `ownership` to the functions generator
- New example generator
- Main documentation and examples
- `mjb_codepoint_numeric_value` documentation
- `mjb_codepoint_to_lowercase` documentation

### Changed
- `mjb_alloc` no longer `memset()` the memory to `0`
- Removed `mjb_codepoint_properties`
- Removed `mjb_codepoint_property`
- Renamed `mjb_codepoint_has_property` to `mjb_codepoint_property_value`
- Renamed `mjb_strnlen` to `mjb_string_length`
- Renamed `mjb_segmentation` to `mjb_break_grapheme_cluster`
- Renamed `mjb_next_character` to `mjb_string_each_character`
- Renamed `_mjbNextCharacterCallback` to `_mjbEachCharacterCallback`
- Added second encoding to `mjb_collation_compare`
- Added second encoding to `mjb_string_is_confusable`
- Renamed `size` to `byte_length`
- Changed test function return from `void*` to `int`
- Renamed `MJB_ENCODING_*` to `MJB_ENC_*`
- Inverted `mjb_normalize` parameters
- Added `output_encoding` to `mjb_map_case`
- Removed all company names
- Moved the Unicode Conformance Requirements points to a separate file

### Fixed
- Removed `mjb_case` redundant tests
- C++: added `noexcept`
- Added Intellisense generator
- Windows: fixed C4310 warning

## [0.2.3] - 2026-07-06
Codename: [NUMBER SIGN]

### Added
- Attractor JS library
- WASM coverage
- Support for `emoji-test.txt`
- Support for `intentional.txt`
- Emoji sequences
- Emoji variation sequences
- CLI "emoji" command
- `test-wasm` target
- UTF-16 pending surrogates
- Support for dynamic library creation
- `MJB_FEATURE_CHARACTER_NAMES` to strip away name tables

### Changed
- Updated Attractor and new coverage
- Split bidi tests into two files
- Renamed workflows to avoid confusion
- Removed coverage script
- Substituted all `bool` statuses to `mjb_status`
- Removed `mjb_initialize*`

### Fixed
- Windows: missing `build-api` target
- Windows: warning C4232 'malloc' is not static
- Docker: missing artifacts
- `mjb_codepoint_encode` must not accept surrogates
- `mjb_case` preserve uncased CJK
- `mjb_string_output()` crashing on realloc failure

## [0.2.2] - 2026-07-02
Codename: [QUOTATION MARK]

### Added
- NetBSD support
- TypeScript library
- TypeScript `unicode.ts` generator
- Support for conditional mappings
- Support for `S` and `T` case foldings
- Support for `BidiTest.txt` test file
- `libFuzzer` fuzz tests
- GitHub test workflow
- Security file

### Changed
- Removed server
- Moved `functions.js` to `functions.ts`
- Changed `onclick` events in main HTML file
- Removed `UNICODE-LICENSE` file
- Updated Attractor to the latest version

### Fixed
- Missing `ctest-cpp` target
- Moved `mjb_size` to WASM-generated
- Added missing license headers
- Added missing `mjb_string_is_confusable` to WASM
- Fixed missing file imports in Dockerfile
- Windows: fixed `C1189`, `C4267` warnings
- `UBSan`: fixed multiple security problems

## [0.2.1] - 2026-06-27
Codename: [EXCLAMATION MARK]

### Added
- Added tests for main allocation functions
- Added tests for `mjb_category_is_*` functions
- Added `UndefinedBehaviorSanitizer` (`UBSan`) build and test targets
- Added missing `mjb_break_word` tests
- Added `test-all` target
- Download of `CLDR` artifacts
- Added `BCP 47` language tag parser
- Added `mjb_locale_parse` WASM support
- Added `mjb_locale_parse` CLI support
- Added `grapheme`, `word`, `line`, `sentence` CLI subcommands
- Added missing `generate-amalgamation.bat` file
- Initial support for CLDR validity parsing

### Changed
- Moved all builds to separate folders
- Updated coverage
- Removed unused WASM exported functions
- Added `NULL` checks to all functions that had `MJB_NONNULL` before
- Added memory allocation safety checks
- Cleaned amalgamation-generated code
- Removed `width` CLI parameter
- `\r` and `\n` are now displayed as special characters on CLI

### Fixed
- Minor documentation fixes
- Fixed WASM `mjb_normalize` call
- Removed all `MJB_NONNULL` declarations
- Added missing ASCII check to `mjb_decode_step`
- Added missing ASCII check to `mjb_codepoint_encode`
- Fixed output encoding on `mjb_string_filter` normalized strings
- Added support for `MJB_DANGEROUSLY_ALLOW_EMBEDDED_NULLS` to `mjb_string_is_utf8`
- Set `width` CLI argument as required
- FreeBSD: removed `gmake` specific syntax
- Windows: fixed `C4310`, `C4456`, `C4566`, `C4701`, `C4702` warnings

## [0.2.0] - 2026-06-23
Codename: [SPACE]

### Added
- Replaced SQLite with embedded data tables
- Confusable detection
- Numeric functions
- Initial support for locales

### Changed
- Removed SQLite usage
- Removed usage of `mojibake.db` in the generator
- Removed hardcoded .c amalgamation arrays
- Updated Node
- Removed the embedded database mode
- Moved C++ API to src folder
- Moved to `tsx`, removed `ts-node`
- Moved AI main file to `AGENTS.md`

### Fixed
- Set default CN category to zero
- The online demo is temporarily offline
- Removed usage of `strncpy`

## [0.1.5] - 2026-03-06
Codename [NEGATIVE ACKNOWLEDGE]

### Added
- Unicode collation algorithm
- Collation key generation
- Truncate grapheme function
- Truncate word function
- Identifier function
- Normalization output encoding
- Support for bidi mirroring
- Scripts detection

### Changed
- Normalization function now accepts an output encoding
- CJK ideograph now intercepts all Unicode 17 extension ranges

### Fixed
- Wrong enum counts
- Missing EAW name
- Uninitialized memory on EAW function
- Missing test files on Dockerfile

## [0.1.4] - 2026-03-04
Codename [DEVICE CONTROL FOUR]

### Added
- Unicode Bidirectional Algorithm
- BidiCharacterTest.txt tests
- Support for OpenBSD
- .gitattributes
- CHANGELOG.md
- bug_report.md template issue
- Breakers in C++ API
- CLI bidi command

## Changed
- Windows placeholder functions are now added to all OS
- Update SQLite to 3.51.2
- Minor refactoring of README.md
- CMake upper bound to 3.31

### Fixes
- Missing files on amalgamation

## [0.1.3] - 2026-02-27
Codename [DEVICE CONTROL THREE]

### Added
- OpenBSD and NetBSD support in the SQLite build script
- Initial casefold support

### Changed
- Upgraded SQLite to 3.51.2

### Fixed
- Windows build warnings and compilation fixes

## [0.1.2] - 2026-02-24
Codename [DEVICE CONTROL TWO]

### Added
- Word Boundary (WB) algorithm. Unicode TR29 word cluster breaking
- Sentence Boundary (SB) algorithm. Unicode TR29 sentence breaking
- `mjb_display_width` for computing the display width of a string
- Sentence breaking exposed in the CLI shell
- Word Break Properties (WBP) in the properties table

### Changed
- Completed Line Breaking (LB) algorithm - Unicode TR14
- Properties available in the CLI shell

### Fixed
- Line break test missing test case for Unicode 17
- Always show `!` (no-break) markers in break output
- Null-terminated string handling in break output

## [0.1.1] - 2026-02-11
Codename [DEVICE CONTROL ONE]

### Added
- New emoji data
- New segmentation test cases

### Changed
- Refactored segmentation algorithm
- Moved East `Asian Width (EAW)` and `Line Break (LB)` to the properties table
- Segmentation state now returns `mjb_break_type`

### Fixed
- Segmentation algorithm bugs
- Embedded NULL support (`MJB_DANGEROUSLY_ALLOW_EMBEDDED_NULLS`)
- Minor UTF decoding fixes

## [0.1.0] - 2026-02-05
Codename [DATA LINK ESCAPE]

### Added
- Codepoint properties system (`properties.c`) with derived core properties
- Initial segmentation algorithm rewrite
- Support for multiple-byte `U+FFFD` replacement
- JavaScript API for the WASM build
- `Bidi_Class` added to excluded properties

### Changed
- Properties storage refactored to use range-based table
- UCD file parser refactored

### Fixed
- C++17 compatibility fixes
- WASM missing declarations

## [0.0.9] - 2026-01-12
Codename [HORIZONTAL TABULATION]

### Added
- Database prefix compression for smaller file size
- Support for Unicode name aliases
- Support for CJK Supplement characters
- Support for Khitan Small Script characters

### Fixed
- WASM missing output files
- Wrong test assertion

## [0.0.8] - 2025-12-23
Codename [BACKSPACE]

### Added
- Interactive break mode in the CLI
- Filter option in the CLI
- Shared library build target
- Check for maximum combining character count

### Changed
- UTF-8 decoding algorithm improvements
- Shell screen functions moved to a separate file
- Database optimizations

### Fixed
- Memory leaks in break allocation
- Wrong break check
- Linux type-limits compiler warning
- Decimal and digit column handling

## [0.0.7] - 2025-12-09
Codename [ALERT]

### Fixed
- Minor documentation fixes and typos
- Wrong Makefile targets

## [0.0.6] - 2025-12-08
Codename [ACKNOWLEDGE]

### Added
- FreeBSD documentation

### Fixed
- FreeBSD build: switched back to Node.js (Bun not supported on FreeBSD)
- FreeBSD build: removed bash-specific syntax

## [0.0.5] - 2025-12-07
Codename [ENQUIRY]

### Fixed
- FreeBSD: use `gmake` instead of `make`
- Makefile target fixes

## [0.0.4] - 2025-12-06
Codename [END OF TRANSMISSION]

### Added
- Embedded database build (`make build-embedded`, `USE_EMBEDDED_DB` CMake option)
- Embedded DB generation script

### Changed
- Renamed `mjb_character_block` to `mjb_codepoint_block`

### Fixed
- C++ unit test fixes
- Missing declaration in amalgamation file

## [0.0.3] - 2025-12-04
Codename [END OF TEXT]

### Fixed
- Unicode version number in the database

## [0.0.2] - 2025-12-04
Codename [START OF TEXT]

### Fixed
- Example amalgamation paths
- Alignment fixes

## [0.0.1] - 2025-12-03
Codename [START OF HEADING]

### Added
- Initial public release
- Unicode 17.0.0 support
- UTF-8, UTF-16, UTF-32 encoding and decoding
- Unicode normalization: NFD, NFKD
- Case conversion (uppercase, lowercase, titlecase)
- Grapheme cluster segmentation
- CJK ideograph detection
- Unicode plane and block operations
- SQLite-backed Unicode Character Database
- CLI shell (`mojibake.sh` / `mojibake.bat`)
- C++ wrapper (`ext/cpp/mojibake.cpp`)
- WASM build support
- Docker-based test environment

[Unreleased]: https://github.com/zaerl/mojibake/compare/v0.5.1...HEAD
[0.5.1]: https://github.com/zaerl/mojibake/compare/v0.5.0...v0.5.1
[0.5.0]: https://github.com/zaerl/mojibake/compare/v0.4.2...v0.5.0
[0.4.2]: https://github.com/zaerl/mojibake/compare/v0.4.1...v0.4.2
[0.4.1]: https://github.com/zaerl/mojibake/compare/v0.4.0...v0.4.1
[0.4.0]: https://github.com/zaerl/mojibake/compare/v0.3.6...v0.4.0
[0.3.6]: https://github.com/zaerl/mojibake/compare/v0.3.5...v0.3.6
[0.3.5]: https://github.com/zaerl/mojibake/compare/v0.3.4...v0.3.5
[0.3.4]: https://github.com/zaerl/mojibake/compare/v0.3.3...v0.3.4
[0.3.3]: https://github.com/zaerl/mojibake/compare/v0.3.2...v0.3.3
[0.3.2]: https://github.com/zaerl/mojibake/compare/v0.3.1...v0.3.2
[0.3.1]: https://github.com/zaerl/mojibake/compare/v0.3.0...v0.3.1
[0.3.0]: https://github.com/zaerl/mojibake/compare/v0.2.8...v0.3.0
[0.2.8]: https://github.com/zaerl/mojibake/compare/v0.2.7...v0.2.8
[0.2.7]: https://github.com/zaerl/mojibake/compare/v0.2.6...v0.2.7
[0.2.6]: https://github.com/zaerl/mojibake/compare/v0.2.5...v0.2.6
[0.2.5]: https://github.com/zaerl/mojibake/compare/v0.2.4...v0.2.5
[0.2.4]: https://github.com/zaerl/mojibake/compare/v0.2.3...v0.2.4
[0.2.3]: https://github.com/zaerl/mojibake/compare/v0.2.2...v0.2.3
[0.2.2]: https://github.com/zaerl/mojibake/compare/v0.2.1...v0.2.2
[0.2.1]: https://github.com/zaerl/mojibake/compare/v0.2.0...v0.2.1
[0.2.0]: https://github.com/zaerl/mojibake/compare/v0.1.5...v0.2.0
[0.1.5]: https://github.com/zaerl/mojibake/compare/v0.1.4...v0.1.5
[0.1.4]: https://github.com/zaerl/mojibake/compare/v0.1.3...v0.1.4
[0.1.3]: https://github.com/zaerl/mojibake/compare/v0.1.2...v0.1.3
[0.1.2]: https://github.com/zaerl/mojibake/compare/v0.1.1...v0.1.2
[0.1.1]: https://github.com/zaerl/mojibake/compare/v0.1.0...v0.1.1
[0.1.0]: https://github.com/zaerl/mojibake/compare/v0.0.9...v0.1.0
[0.0.9]: https://github.com/zaerl/mojibake/compare/v0.0.8...v0.0.9
[0.0.8]: https://github.com/zaerl/mojibake/compare/v0.0.7...v0.0.8
[0.0.7]: https://github.com/zaerl/mojibake/compare/v0.0.6...v0.0.7
[0.0.6]: https://github.com/zaerl/mojibake/compare/v0.0.5...v0.0.6
[0.0.5]: https://github.com/zaerl/mojibake/compare/v0.0.4...v0.0.5
[0.0.4]: https://github.com/zaerl/mojibake/compare/v0.0.3...v0.0.4
[0.0.3]: https://github.com/zaerl/mojibake/compare/v0.0.2...v0.0.3
[0.0.2]: https://github.com/zaerl/mojibake/compare/v0.0.1...v0.0.2
[0.0.1]: https://github.com/zaerl/mojibake/releases/tag/v0.0.1
