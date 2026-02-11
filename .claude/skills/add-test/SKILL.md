---
name: add-test
description: Add a new test assertion to the mojibake test suite
version: 0.1.0
author: Claude
tags: [testing, mojibake, c]
---

# Add Test Skill

This skill helps you add a new test assertion to the mojibake library test suite.

## Test Structure

- Tests are located in the `tests/` directory
- Each test file mirrors a source file: `src/file.c` -> `tests/file.c`
- Tests use the ATT_ASSERT macro: `ATT_ASSERT(value_to_test, expected_value, comment_string)`
- Each test file has a function `void *test_<name>(void *arg)` that contains the assertions

## Instructions

When this skill is invoked, follow these steps:

1. **Identify the source file**:
   - Ask the user which source file they want to add a test for (e.g., "version", "codepoint", "breaking")
   - If the user provides a full path like `src/version.c`, extract just the base name

2. **Locate or create the test file**:
   - Check if `tests/<basename>.c` exists
   - If it doesn't exist, you'll need to:
     - Create the new test file with the standard header
     - Add a test function `void *test_<basename>(void *arg)`
     - Add the function declaration to `tests/test.h`
     - Register the test in the main test runner

3. **Get test details**:
   - Ask the user for:
     - The function or value to test (e.g., `mjb_version()`)
     - The expected value (e.g., `MJB_VERSION` or a literal value)
     - A descriptive comment (e.g., "Valid version")

4. **Add the assertion**:
   - Open the test file
   - Find the test function `void *test_<basename>(void *arg)`
   - Add the new ATT_ASSERT line before the `return NULL;` statement
   - Format: `ATT_ASSERT(value_to_test, expected_value, "comment");`

5. **Verify**:
   - Show the user the added assertion
   - Suggest running only the specific test file: `ARGS="--filter=<basename>" make test`
   - Example: `ARGS="--filter=version" make test`
   - If the test fails, run it again with `-vv` flag for verbose output to see what went wrong:
     `ARGS="--filter=<basename> -vv" make test`

## Example

For adding a test to `src/version.c`:

```c
void *test_version(void *arg) {
    ATT_ASSERT(mjb_version(), (const char*)MJB_VERSION, "Valid version");
    ATT_ASSERT(mjb_version_number(), MJB_VERSION_NUMBER, "Valid version number");
    // New assertion would be added here

    return NULL;
}
```

Run only this test with:
```bash
ARGS="--filter=version" make test
```

## Notes

- Maintain consistent indentation (4 spaces)
- Keep comments descriptive but concise
- Cast values appropriately (e.g., `(const char*)` for string comparisons)
- Place assertions in logical order (related tests together)
