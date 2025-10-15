/**
 * 2025-08-13
 *
 * The Attractor Unit Test library
 */

#include "attractor.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <io.h>
    #define isatty _isatty
    #define STDOUT_FILENO _fileno(stdout)
#else
    #include <unistd.h>
#endif

#define ATT_ERROR_MESSAGE(RESULT, FORMAT, EXPECTED) \
if(att_verbose >= 1 && att_show_error) { \
    fputs(att_show_colors ? "Expected \x1B[32m" : "Expected ", stdout); \
    printf(FORMAT, EXPECTED); \
    fputs(att_show_colors ? "\x1B[0m, got \x1B[31m" : ", got ", stdout); \
    printf(FORMAT, RESULT); \
    fputs(att_show_colors ? "\x1B[0m\n\n" : "\n\n", stdout); \
}

static unsigned int att_valid_tests = 0;
static unsigned int att_total_tests = 0;
static unsigned int att_verbose = ATT_VERBOSE;
static unsigned int att_show_error = ATT_SHOW_ERROR;
static int att_show_colors = 0;
static att_generic_callback att_callback = NULL;
static att_test_callback att_t_callback = NULL;

unsigned int att_get_valid_tests(void) {
    return att_valid_tests;
}

unsigned int att_get_total_tests(void) {
    return att_total_tests;
}

void att_set_verbose(unsigned int verbose) {
    att_verbose = verbose;
}

void att_set_show_error(unsigned int show_error) {
    att_show_error = show_error;
}

void att_set_generic_callback(att_generic_callback callback) {
    att_callback = callback;
}

void att_set_test_callback(att_test_callback callback) {
    att_t_callback = callback;
}

int att_assert(const char *type, int test, const char *description);

ATT_API unsigned int att_assert_c(char result, char expected, const char *description) {
    int test = att_assert("char", result == expected, description);

    if(!test) {
        ATT_ERROR_MESSAGE(result, "%c", expected);
    }

    if(att_t_callback) {
        att_t_callback(test, description);
    }

    return test;
}

ATT_API unsigned int att_assert_u_c(unsigned char result, unsigned char expected, const char *description) {
    int test = att_assert("unsigned char", result == expected, description);

    if(!test) {
        ATT_ERROR_MESSAGE(result, "%c", expected);
    }

    if(att_t_callback) {
        att_t_callback(test, description);
    }

    return test;
}

ATT_API unsigned int att_assert_p_c(char* result, char* expected, const char *description) {
    int test = att_assert("char*", ((result == expected) || ((result && expected) ? strcmp(result, expected) == 0 : 0)), description);

    if(!test) {
        ATT_ERROR_MESSAGE(result, ATT_STRING_AS_POINTERS == 1 ? "%p" : "\"%s\"", expected);
    }

    if(att_t_callback) {
        att_t_callback(test, description);
    }

    return test;
}

ATT_API unsigned int att_assert_cp_c(const char* result, const char* expected, const char *description) {
    int test = att_assert("const char*", ((result == expected) || ((result && expected) ? strcmp(result, expected) == 0 : 0)), description);

    if(!test) {
        ATT_ERROR_MESSAGE(result, ATT_STRING_AS_POINTERS == 1 ? "%p" : "\"%s\"", expected);
    }

    if(att_t_callback) {
        att_t_callback(test, description);
    }

    return test;
}

ATT_API unsigned int att_assert_hd(short result, short expected, const char *description) {
    int test = att_assert("short", result == expected, description);

    if(!test) {
        ATT_ERROR_MESSAGE(result, "%hd", expected);
    }

    if(att_t_callback) {
        att_t_callback(test, description);
    }

    return test;
}

ATT_API unsigned int att_assert_u_hu(unsigned short result, unsigned short expected, const char *description) {
    int test = att_assert("unsigned short", result == expected, description);

    if(!test) {
        ATT_ERROR_MESSAGE(result, "%hu", expected);
    }

    if(att_t_callback) {
        att_t_callback(test, description);
    }

    return test;
}

ATT_API unsigned int att_assert_d(int result, int expected, const char *description) {
    int test = att_assert("int", result == expected, description);

    if(!test) {
        ATT_ERROR_MESSAGE(result, "%d", expected);
    }

    if(att_t_callback) {
        att_t_callback(test, description);
    }

    return test;
}

ATT_API unsigned int att_assert_u_u(unsigned int result, unsigned int expected, const char *description) {
    int test = att_assert("unsigned int", result == expected, description);

    if(!test) {
        ATT_ERROR_MESSAGE(result, "%u", expected);
    }

    if(att_t_callback) {
        att_t_callback(test, description);
    }

    return test;
}

ATT_API unsigned int att_assert_ld(long result, long expected, const char *description) {
    int test = att_assert("long", result == expected, description);

    if(!test) {
        ATT_ERROR_MESSAGE(result, "%ld", expected);
    }

    if(att_t_callback) {
        att_t_callback(test, description);
    }

    return test;
}

ATT_API unsigned int att_assert_u_lu(unsigned long result, unsigned long expected, const char *description) {
    int test = att_assert("unsigned long", result == expected, description);

    if(!test) {
        ATT_ERROR_MESSAGE(result, "%lu", expected);
    }

    if(att_t_callback) {
        att_t_callback(test, description);
    }

    return test;
}

ATT_API unsigned int att_assert_lld(long long result, long long expected, const char *description) {
    int test = att_assert("long long", result == expected, description);

    if(!test) {
        ATT_ERROR_MESSAGE(result, "%lld", expected);
    }

    if(att_t_callback) {
        att_t_callback(test, description);
    }

    return test;
}

ATT_API unsigned int att_assert_u_llu(unsigned long long result, unsigned long long expected, const char *description) {
    int test = att_assert("unsigned long long", result == expected, description);

    if(!test) {
        ATT_ERROR_MESSAGE(result, "%llu", expected);
    }

    if(att_t_callback) {
        att_t_callback(test, description);
    }

    return test;
}

ATT_API unsigned int att_assert_f(float result, float expected, const char *description) {
    int test = att_assert("float", result == expected, description);

    if(!test) {
        ATT_ERROR_MESSAGE(result, "%f", expected);
    }

    if(att_t_callback) {
        att_t_callback(test, description);
    }

    return test;
}

ATT_API unsigned int att_assert_lf(double result, double expected, const char *description) {
    int test = att_assert("double", result == expected, description);

    if(!test) {
        ATT_ERROR_MESSAGE(result, "%lf", expected);
    }

    if(att_t_callback) {
        att_t_callback(test, description);
    }

    return test;
}

ATT_API unsigned int att_assert_Lf(long double result, long double expected, const char *description) {
    int test = att_assert("long double", result == expected, description);

    if(!test) {
        ATT_ERROR_MESSAGE(result, "%Lf", expected);
    }

    if(att_t_callback) {
        att_t_callback(test, description);
    }

    return test;
}

ATT_API unsigned int att_assert_p_p(void* result, void* expected, const char *description) {
    int test = att_assert("void*", result == expected, description);

    if(!test) {
        ATT_ERROR_MESSAGE(result, "%p", expected);
    }

    if(att_t_callback) {
        att_t_callback(test, description);
    }

    return test;
}

ATT_API unsigned int att_assert_b(_Bool result, _Bool expected, const char *description) {
    int test = att_assert("_Bool", result == expected, description);

    if(!test) {
        ATT_ERROR_MESSAGE(result, "%d", expected);
    }

    if(att_t_callback) {
        att_t_callback(test, description);
    }

    return test;
}

ATT_API unsigned int att_assert_unknown(void* result, void* expected, const char *description) {
    int test = att_assert(att_callback ? "callback" : "default", att_callback ? att_callback(result, expected, description) : (result == expected), description);

    if(!test) {
        ATT_ERROR_MESSAGE(result, "%p", expected);
    }

    if(att_t_callback) {
        att_t_callback(test, description);
    }

    return test;
}

int att_assert(const char *format, int test, const char *description) {
    ++att_total_tests;

    // Initialize the library
    if(att_total_tests == 1) {
        if(isatty(STDOUT_FILENO)) {
            const char *no_color = getenv("NO_COLOR");

#ifdef _WIN32
            // On Windows, TERM is usually not set, so enable colors by default if NO_COLOR is not set
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
                SetConsoleMode(h_out, mode_out);
            } else {
                // If we can't enable ANSI codes, disable colors
                att_show_colors = 0;
            }

            // Enable ANSI escape codes for stderr
            if(h_err != INVALID_HANDLE_VALUE && GetConsoleMode(h_err, &mode_err)) {
                mode_err |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
                SetConsoleMode(h_err, mode_err);
            }
        }
#endif
    }

    if(test) {
        ++att_valid_tests;
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

        printf(att_show_colors ? "%s [\x1b[36m%s\x1b[0m] %s\n" : "%s [%s] %s\n",
            test ? ok : fail, format, description);
    }

    return test;
}

