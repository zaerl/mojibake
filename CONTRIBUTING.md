# Contributing to Mojibake

This document provides guidelines and information for contributors.

Mojibake uses [CMake](https://cmake.org/) on POSIX-compliant operating system and
[NMAKE](https://learn.microsoft.com/en-us/cpp/build/reference/nmake-reference?view=msvc-170) on
Windows.

You can run tests in multiple ways. On POSIX platforms:

1. `make test` run tests compiled as C
2. `make test-cpp` run tests as C++
3. `make test-asan` run tests with [AddressSanitizer](https://github.com/google/sanitizers/wiki/AddressSanitizer)
4. `make ctest` run tests using [ctest](https://cmake.org/cmake/help/latest/manual/ctest.1.html)

Windows 11:

1. `nmake /F Makefile.nmake test` run tests compiled as C
2. `nmake /F Makefile.nmake test-cpp` run tests compiled as C

Docker:

1. `make test-docker` run tests Alpine Linux, using Docker

## Code Style

- We follow a consistent code style using clang-format
- Use 4 spaces for indentation (no tabs)
- Maximum line length is 100 characters
- See .clang-format for complete style configuration

## Pull Request Process

1. Fork the repository and create a new branch for your feature/fix
2. Write clear commit messages explaining your changes
3. Add/update tests to cover your changes
4. Ensure all tests pass before submitting
5. Submit a pull request with a description of your changes

## Testing

- Add unit tests for any new functionality
- Run the full test suite before submitting changes
- Current test coverage is tracked in the project
- Aim to maintain or improve test coverage

We use Attractor to run tests.

```c
ATT_ASSERT(what_you_are_testing, "must be equal to this value", "Description")
```

For example, imagine we have found a bug on the `mjb_strnlen` function that do not correctly count
the length of the `"Hello, test"` string and we fixed it. On `tests/string.c` function add a new
assertion:

```c
ATT_ASSERT(mjb_strnlen("Hello, test", 11, MJB_ENCODING_UTF_8), 11, "UTF-8 Hello, test")
```

Then run `make test` and `make test-cpp` to be sure all tests are working.

## Reporting Issues

- Use the GitHub issue tracker
- Provide a clear description of the issue
- Include steps to reproduce if applicable
- Note your environment/version information

## License

By contributing to Mojibake, you agree that your contributions will be licensed under the MIT
License. See [LICENSE](LICENSE) for details.

## Questions?

Feel free to open an issue for any questions about contributing.

Thank you for helping.
