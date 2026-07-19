/**
 * The Attractor Unit Test library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "attractor.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// clang-format off
#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <io.h>
    #define isatty _isatty
    #ifndef STDOUT_FILENO
        #define STDOUT_FILENO _fileno(stdout)
    #endif
#else
    #include <unistd.h>
#endif

// ATT_FETCH_INCREMENT atomically increments and returns the previous value.
#ifdef __cplusplus
    #include <atomic>
    typedef std::atomic<unsigned int> att_atomic_uint;
    #define ATT_FETCH_INCREMENT(value) ((value)++)
#elif defined(_MSC_VER)
    // MSVC ships no C11 <stdatomic.h> when compiling as C (error C1189 without the
    // /experimental:c11atomics switch), so use the Interlocked intrinsics instead.
    typedef volatile LONG att_atomic_uint;
    #define ATT_FETCH_INCREMENT(value) ((unsigned int)InterlockedIncrement(&(value)) - 1)
#else
    #include <stdatomic.h>
    typedef atomic_uint att_atomic_uint;
    #define ATT_FETCH_INCREMENT(value) ((value)++)
#endif
// clang-format on

#define ATT_ERROR_MESSAGE(RESULT, FORMAT_1, FORMAT_2, EXPECTED) \
    if(att_show_error) { \
        printf(att_show_colors ? "\x1B[90m%s:%u:\x1B[0m " : "%s:%u: ", file, line); \
        if(att_verbose < 2) { \
            printf("%s: ", description); \
        } \
        fputs(att_show_colors ? "Expected \x1B[32m" : "Expected ", stdout); \
        printf(FORMAT_1, EXPECTED); \
        fputs(att_show_colors ? "\x1B[0m, got \x1B[31m" : ", got ", stdout); \
        printf(FORMAT_2, RESULT); \
        fputs(att_show_colors ? "\x1B[0m\n\n" : "\n\n", stdout); \
    }

static att_atomic_uint att_valid_tests = 0;
static att_atomic_uint att_total_tests = 0;
static unsigned int att_verbose = ATT_VERBOSE;
static unsigned int att_show_error = ATT_SHOW_ERROR;
static int att_show_colors = 0;
static long double att_float_epsilon = ATT_FLOAT_EPSILON;

// A callback to be used when the default comparison fails.
static att_generic_callback att_generic_callback_fn = NULL;

// A callback to be used when an test occurs.
static att_test_callback att_test_callback_fn = NULL;

// Variables stored by each assertion to provide context for the next assertion.
static const char *att_assert_expression = "";
static const char *att_assert_file = "";
static unsigned int att_assert_line = 0;

unsigned int att_get_valid_tests(void) {
    return (unsigned int)att_valid_tests;
}

unsigned int att_get_total_tests(void) {
    return (unsigned int)att_total_tests;
}

void att_set_verbose(unsigned int verbose) {
    att_verbose = verbose;
}

void att_set_show_error(unsigned int show_error) {
    att_show_error = show_error;
}

long double att_get_float_epsilon(void) {
    return att_float_epsilon;
}

void att_set_float_epsilon(long double epsilon) {
    att_float_epsilon = epsilon;
}

void att_set_generic_callback(att_generic_callback callback) {
    att_generic_callback_fn = callback;
}

void att_set_test_callback(att_test_callback callback) {
    att_test_callback_fn = callback;
}

void att_set_assert_context(const char *expression, const char *file, unsigned int line) {
    att_assert_expression = expression ? expression : "";
    att_assert_file = file ? file : "";
    att_assert_line = line;
}

// These functions are automatically generated. Do not edit.
// clang-format off
int att_assert(const char *type, int test, const char *description);

ATT_API unsigned int att_assert_c(char result, char expected, const char *description, const char *file, unsigned int line) {
    int test = att_assert("char", result == expected, description);

    if(!test) {
        const char *format_1;
        const char *format_2;

        if(expected < 32 || expected == 127) {
            format_1 = "\\x%02X";
        } else {
            format_1 = "%c";
        }

        if(result < 32 || result == 127) {
            format_2 = "\\x%02X";
        } else {
            format_2 = "%c";
        }

        ATT_ERROR_MESSAGE(result, format_1, format_2, expected);
    }

    return test;
}

ATT_API unsigned int att_assert_u_c(unsigned char result, unsigned char expected, const char *description, const char *file, unsigned int line) {
    int test = att_assert("unsigned char", result == expected, description);

    if(!test) {
        const char *format_1;
        const char *format_2;

        if(expected < 32 || expected == 127) {
            format_1 = "\\x%02X";
        } else {
            format_1 = "%c";
        }

        if(result < 32 || result == 127) {
            format_2 = "\\x%02X";
        } else {
            format_2 = "%c";
        }

        ATT_ERROR_MESSAGE(result, format_1, format_2, expected);
    }

    return test;
}

ATT_API unsigned int att_assert_p_c(char *result, char *expected, const char *description, const char *file, unsigned int line) {
    int test = att_assert("char *", ((result == expected) || ((result && expected) ? strcmp(result, expected) == 0 : 0)), description);

    if(!test) {
        ATT_ERROR_MESSAGE(result, ATT_STRING_AS_POINTERS == 1 ? "%p" : "\"%s\"", ATT_STRING_AS_POINTERS == 1 ? "%p" : "\"%s\"", expected);
    }

    return test;
}

ATT_API unsigned int att_assert_cp_c(const char *result, const char *expected, const char *description, const char *file, unsigned int line) {
    int test = att_assert("const char *", ((result == expected) || ((result && expected) ? strcmp(result, expected) == 0 : 0)), description);

    if(!test) {
        ATT_ERROR_MESSAGE(result, ATT_STRING_AS_POINTERS == 1 ? "%p" : "\"%s\"", ATT_STRING_AS_POINTERS == 1 ? "%p" : "\"%s\"", expected);
    }

    return test;
}

ATT_API unsigned int att_assert_hd(short result, short expected, const char *description, const char *file, unsigned int line) {
    int test = att_assert("short", result == expected, description);

    if(!test) {
        ATT_ERROR_MESSAGE(result, "%hd", "%hd", expected);
    }

    return test;
}

ATT_API unsigned int att_assert_u_hu(unsigned short result, unsigned short expected, const char *description, const char *file, unsigned int line) {
    int test = att_assert("unsigned short", result == expected, description);

    if(!test) {
        ATT_ERROR_MESSAGE(result, "%hu", "%hu", expected);
    }

    return test;
}

ATT_API unsigned int att_assert_d(int result, int expected, const char *description, const char *file, unsigned int line) {
    int test = att_assert("int", result == expected, description);

    if(!test) {
        ATT_ERROR_MESSAGE(result, "%d", "%d", expected);
    }

    return test;
}

ATT_API unsigned int att_assert_u_u(unsigned int result, unsigned int expected, const char *description, const char *file, unsigned int line) {
    int test = att_assert("unsigned int", result == expected, description);

    if(!test) {
        ATT_ERROR_MESSAGE(result, "%u", "%u", expected);
    }

    return test;
}

ATT_API unsigned int att_assert_ld(long result, long expected, const char *description, const char *file, unsigned int line) {
    int test = att_assert("long", result == expected, description);

    if(!test) {
        ATT_ERROR_MESSAGE(result, "%ld", "%ld", expected);
    }

    return test;
}

ATT_API unsigned int att_assert_u_lu(unsigned long result, unsigned long expected, const char *description, const char *file, unsigned int line) {
    int test = att_assert("unsigned long", result == expected, description);

    if(!test) {
        ATT_ERROR_MESSAGE(result, "%lu", "%lu", expected);
    }

    return test;
}

ATT_API unsigned int att_assert_lld(long long result, long long expected, const char *description, const char *file, unsigned int line) {
    int test = att_assert("long long", result == expected, description);

    if(!test) {
        ATT_ERROR_MESSAGE(result, "%lld", "%lld", expected);
    }

    return test;
}

ATT_API unsigned int att_assert_u_llu(unsigned long long result, unsigned long long expected, const char *description, const char *file, unsigned int line) {
    int test = att_assert("unsigned long long", result == expected, description);

    if(!test) {
        ATT_ERROR_MESSAGE(result, "%llu", "%llu", expected);
    }

    return test;
}

ATT_API unsigned int att_assert_f(float result, float expected, const char *description, const char *file, unsigned int line) {
    int test = att_assert("float", (result == expected) || ((result > expected ? result - expected : expected - result) <= att_float_epsilon), description);

    if(!test) {
        ATT_ERROR_MESSAGE(result, "%f", "%f", expected);
    }

    return test;
}

ATT_API unsigned int att_assert_lf(double result, double expected, const char *description, const char *file, unsigned int line) {
    int test = att_assert("double", (result == expected) || ((result > expected ? result - expected : expected - result) <= att_float_epsilon), description);

    if(!test) {
        ATT_ERROR_MESSAGE(result, "%lf", "%lf", expected);
    }

    return test;
}

ATT_API unsigned int att_assert_Lf(long double result, long double expected, const char *description, const char *file, unsigned int line) {
    int test = att_assert("long double", (result == expected) || ((result > expected ? result - expected : expected - result) <= att_float_epsilon), description);

    if(!test) {
        ATT_ERROR_MESSAGE(result, "%Lf", "%Lf", expected);
    }

    return test;
}

ATT_API unsigned int att_assert_p_p(void *result, void *expected, const char *description, const char *file, unsigned int line) {
    int test = att_assert("void *", result == expected, description);

    if(!test) {
        ATT_ERROR_MESSAGE(result, "%p", "%p", expected);
    }

    return test;
}

ATT_API unsigned int att_assert_b(_Bool result, _Bool expected, const char *description, const char *file, unsigned int line) {
    int test = att_assert("_Bool", result == expected, description);

    if(!test) {
        ATT_ERROR_MESSAGE(result, "%d", "%d", expected);
    }

    return test;
}

ATT_API unsigned int att_assert_unknown(void * result, void * expected, const char *description, const char *file, unsigned int line) {
    int test = att_assert(
        att_generic_callback_fn ? "callback" : "default",
        att_generic_callback_fn ? att_generic_callback_fn(result, expected, description) : (result == expected),
        description
    );

    if(!test) {
        ATT_ERROR_MESSAGE(result, "%p", "%p", expected);
    }

    return test;
}

// clang-format on
int att_assert(const char *format, int test, const char *description) {
    // Initialize the library on the first assertion. The fetch-increment returns the
    // previous value atomically, so exactly one thread observes zero here.
    if(ATT_FETCH_INCREMENT(att_total_tests) == 0) {
        if(isatty(STDOUT_FILENO)) {
            const char *no_color = getenv("NO_COLOR");

#ifdef _WIN32
            // On Windows, the TERM env is usually not set, so enable colors by default if NO_COLOR
            // is not set
            att_show_colors = no_color == NULL;
#else
            // On Unix, check TERM environment variable
            const char *term = getenv("TERM");
            att_show_colors = no_color == NULL && term != NULL && strcmp(term, "dumb") != 0;
#endif
        }

#ifdef _WIN32
        if(att_show_colors) {
            // On Windows, we need to enable ANSI escape codes for stdout and stderr
            HANDLE h_out = GetStdHandle(STD_OUTPUT_HANDLE);
            HANDLE h_err = GetStdHandle(STD_ERROR_HANDLE);
            DWORD mode_out, mode_err;

            // Enable ANSI escape codes for stdout
            if(h_out != INVALID_HANDLE_VALUE && GetConsoleMode(h_out, &mode_out)) {
                mode_out |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;

                if(!SetConsoleMode(h_out, mode_out)) {
                    // If we can't enable ANSI codes, disable colors
                    att_show_colors = 0;
                }
            } else {
                // If we can't get console mode, disable colors
                att_show_colors = 0;
            }

            // Enable ANSI escape codes for stderr
            if(att_show_colors && h_err != INVALID_HANDLE_VALUE &&
                GetConsoleMode(h_err, &mode_err)) {
                mode_err |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
                SetConsoleMode(h_err, mode_err);
            }
        }
#endif
    }

    if(test) {
        ATT_FETCH_INCREMENT(att_valid_tests);
    }

    if(att_test_callback_fn) {
        att_test_callback_fn(test, description, att_assert_expression, att_assert_file,
            att_assert_line);
    }

    if(att_verbose == 0) {
        // Do nothing
    } else if(att_verbose == 1) {
        fputs(test ? "." : (att_show_colors ? "\x1B[31mF\x1B[0m" : "F"), stdout);

        if(!test) {
            fputs("\n", stdout);
        }
    } else {
        const char *ok = att_show_colors ? "\x1B[32mOK\x1B[0m" : "OK";
        const char *fail = att_show_colors ? "\x1B[31mNO\x1B[0m" : "NO";

        printf(att_show_colors ? "%s [\x1b[36m%s\x1b[0m] %s\n" : "%s [%s] %s\n", test ? ok : fail,
            format, description);
    }

    return test;
}
