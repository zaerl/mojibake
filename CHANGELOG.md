# Changelog

All notable changes to Mojibake are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.1.4] - 2026-03-04

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

### Added
- OpenBSD and NetBSD support in the SQLite build script
- Initial casefold support

### Changed
- Upgraded SQLite to 3.51.2

### Fixed
- Windows build warnings and compilation fixes

## [0.1.2] - 2026-02-24

### Added
- Word Boundary (WB) algorithm. Unicode TR29 word cluster breaking
- Sentence Boundary (SB) algorithm. Unicode TR29 sentence breaking
- `mjb_display_width` for computing the display width of a string
- Sentence breaking exposed in the CLI shell
- Word Break Properties (WBP) in the properties table

### Changed
- Completed Line Breaking (LB) algorithm — Unicode TR14
- Properties available in the CLI shell

### Fixed
- Line break test missing test case for Unicode 17
- Always show `!` (no-break) markers in break output
- Null-terminated string handling in break output

## [0.1.1] - 2026-02-11

### Added
- New emoji data
- New segmentation test cases

### Changed
- Refactored segmentation algorithm
- Moved East Asian Width (EAW) and Line Break (LB) to the properties table
- Segmentation state now returns `mjb_break_type`

### Fixed
- Segmentation algorithm bugs
- Embedded NULL support (`MJB_DANGEROUSLY_ALLOW_EMBEDDED_NULLS`)
- Minor UTF decoding fixes

## [0.1.0] - 2026-02-05

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

### Added
- Database prefix compression for smaller file size
- Support for Unicode name aliases
- Support for CJK Supplement characters
- Support for Khitan Small Script characters

### Fixed
- WASM missing output files
- Wrong test assertion

## [0.0.8] - 2025-12-23

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

### Fixed
- Minor documentation fixes and typos
- Wrong Makefile targets

## [0.0.6] - 2025-12-08

### Added
- FreeBSD documentation

### Fixed
- FreeBSD build: switched back to Node.js (Bun not supported on FreeBSD)
- FreeBSD build: removed bash-specific syntax

## [0.0.5] - 2025-12-07

### Fixed
- FreeBSD: use `gmake` instead of `make`
- Makefile target fixes

## [0.0.4] - 2025-12-06

### Added
- Embedded database build (`make build-embedded`, `USE_EMBEDDED_DB` CMake option)
- Embedded DB generation script

### Changed
- Renamed `mjb_character_block` to `mjb_codepoint_block`

### Fixed
- C++ unit test fixes
- Missing declaration in amalgamation file

## [0.0.3] - 2025-12-04

### Fixed
- Unicode version number in the database

## [0.0.2] - 2025-12-04

### Fixed
- Example amalgamation paths
- Alignment fixes

## [0.0.1] - 2025-12-03

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

[Unreleased]: https://github.com/zaerl/mojibake/compare/0.1.4...HEAD
[0.1.4]: https://github.com/zaerl/mojibake/compare/0.1.3...0.1.4
[0.1.3]: https://github.com/zaerl/mojibake/compare/0.1.2...0.1.3
[0.1.2]: https://github.com/zaerl/mojibake/compare/0.1.1...0.1.2
[0.1.1]: https://github.com/zaerl/mojibake/compare/0.1.0...0.1.1
[0.1.0]: https://github.com/zaerl/mojibake/compare/0.0.9...0.1.0
[0.0.9]: https://github.com/zaerl/mojibake/compare/0.0.8...0.0.9
[0.0.8]: https://github.com/zaerl/mojibake/compare/0.0.7...0.0.8
[0.0.7]: https://github.com/zaerl/mojibake/compare/0.0.6...0.0.7
[0.0.6]: https://github.com/zaerl/mojibake/compare/0.0.5...0.0.6
[0.0.5]: https://github.com/zaerl/mojibake/compare/0.0.4...0.0.5
[0.0.4]: https://github.com/zaerl/mojibake/compare/0.0.3...0.0.4
[0.0.3]: https://github.com/zaerl/mojibake/compare/0.0.2...0.0.3
[0.0.2]: https://github.com/zaerl/mojibake/compare/0.0.1...0.0.2
[0.0.1]: https://github.com/zaerl/mojibake/releases/tag/0.0.1
