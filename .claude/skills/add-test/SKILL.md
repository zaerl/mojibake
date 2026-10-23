---
name: add-test
description: Add a new test assertion to the mojibake test suite
version: 0.5.1
author: Claude
tags: [testing, mojibake, c]
---

# Add Test Skill

This skill adds test assertions, or a new test case, to the Mojibake test suite.

## Test Structure

- C tests live in `tests/` and mirror the source module: `src/<module>.c` -> `tests/<module>.c`.
  C++ wrapper tests live in `tests/ext/cpp/`.
- Tests use the vendored Attractor framework (`tests/attractor.h`, do not edit it) through
  `tests/test.h`.
- A test case is defined with `ATT_TEST(name) { ... }`. The macro registers the case before
  `main()` runs, so nothing has to be declared in `tests/test.h` or listed in `tests/test.c`.
  Most files have exactly one `ATT_TEST`, named after the file with `-` replaced by `_`
  (`tests/break-word.c` -> `ATT_TEST(break_word)`), and call `static` helper functions from it.
- `tests/example.c` and `tests/ext/cpp/example.cpp` are generated from the examples in
  `utils/generate/functions.ts`. Never edit them; change the example there and run
  `make generate`.

## Assertions

- `ATT_ASSERT(actual, expected, "description")` compares ordinary values. The comparison is
  chosen from the type of `actual` (`_Generic` in C, a template in C++), so give `expected` the
  same type: `(size_t)0`, `(unsigned int)'A'`, `(const char *)MJB_VERSION`. Enums and other
  unsupported types must be cast, for example `(unsigned int)locale.locale`. `char *` values are
  compared as strings.
- `ATT_ASSERT_STATUS(call, MJB_STATUS_..., "description")` compares `mjb_status` values.
- Use `memcmp` for byte sequences that are not NUL-terminated, such as `mjb_result.output` or
  UTF-16/UTF-32 output: `ATT_ASSERT((int)memcmp(output, "e\xCC\x81", 3), 0, "...")`.
- Coverage is attributed to the `mjb_*` function called inside the assertion. When the assertion
  checks a value computed earlier, call `MJB_TEST_COVERAGE(mjb_function_name);` before the call.
- Write non-ASCII input as UTF-8 escape sequences (`"\xC4\xB0"`), not literal characters.

## Instructions

1. **Identify the target**:
   - Ask which module or public function to test if the user has not said. Accept either a base
     name (`normalization`) or a path (`src/normalization.c`).

2. **Locate or create the test file**:
   - If `tests/<module>.c` exists, find its `ATT_TEST(...)` and the helper that groups related
     assertions; add a new `static void test_<topic>(void)` helper and call it from the
     `ATT_TEST` when no existing helper fits.
   - If the file does not exist:
     - Create it with the license header and `#include "test.h"` (see the example below).
     - Add it to `TEST_SOURCES` in `tests/CMakeLists.txt`.
     - Add a CTest entry next to the others:
       `mjb_add_test("Display Name" test_name ${CMAKE_CURRENT_LIST_LINE})`.
   - For C++ wrapper tests, use `tests/ext/cpp/<topic>.cpp` with `ATT_TEST(cpp_<topic>)`,
     `#include "../../../src/cpp/mojibake.hpp"` and `#include "../../test.h"`, and register the
     file in the `MJB_TEST_CPP_WRAPPER` block of `tests/CMakeLists.txt`.

3. **Get the test details** from the user when they are not obvious: the call under test, the
   expected value, and a short description.

4. **Write the assertions**, following the conventions for the API being tested:
   - Allocating APIs: initialize `mjb_result result = { NULL, 0, 0, false };`, check the status,
     compare `result.output` using `result.output_size`, then `mjb_result_free(&result)`.
   - `_into` APIs: cover `output == NULL` measurement, a too-small buffer
     (`MJB_STATUS_OUTPUT_TOO_SMALL` with nothing written past capacity), and success.
   - Cover invalid arguments, malformed input under each relevant `mjb_malformed_policy`, and
     explicit byte lengths that include an embedded NUL.
   - A new allocating public API also needs a `SWEEP(<name>)` entry (and its `sweep_<name>`
     function) in `tests/allocator.c`, which injects an allocation failure at every step and
     checks that outputs are cleared and nothing leaks. When a test uses
     `mjb_test_allocator_fail_after()` directly, call `mjb_test_allocator_reset()` before it
     returns.
   - Code behind `MJB_FEATURE_CHARACTER_NAMES`, `MJB_FEATURE_COLLATION`, `MJB_FEATURE_IDNA`, or
     `MJB_FEATURE_SECURITY` must be wrapped in `#if MJB_FEATURE_...`; in the `#else` branch,
     assert that the API returns `MJB_STATUS_FEATURE_NOT_ENABLED` (see `tests/idna.c`).

5. **Run the focused test** (`-f` matches a substring of the test name):

   ```bash
   make test ARGS="-f normalization"
   ```

   On failure, rerun with `ARGS="-f normalization -vv"` for verbose output or add `-e` to stop
   at the first failure. Then run `make test-cpp` (the tests also compile as C++20), plus
   `make test-features` when the test touches a feature flag and `make test-asan` when it
   exercises allocation or buffers.

6. **Report** the added assertions and the commands that were run.

## Example

A new file `tests/version.c`:

```c
/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "test.h"

ATT_TEST(version) {
    ATT_ASSERT(mjb_version(), (const char *)MJB_VERSION, "Valid version");
    ATT_ASSERT(mjb_version_number(), MJB_VERSION_NUMBER, "Valid version number");
    ATT_ASSERT(mjb_unicode_version(), (const char *)MJB_UNICODE_VERSION, "Valid Unicode version");
}
```

Registered in `tests/CMakeLists.txt` with `version.c` in `TEST_SOURCES` and:

```cmake
mjb_add_test("Version" version ${CMAKE_CURRENT_LIST_LINE})
```

Run it with:

```bash
make test ARGS="-f version"
```

## Notes

- Use 4 spaces and keep lines within 100 columns; `.clang-format` is authoritative.
- Keep descriptions short and specific; they are printed when an assertion fails.
- Mark intentionally unused callback parameters with `(void)name;`.
- Place related assertions together, in the order the API documents its behavior.
