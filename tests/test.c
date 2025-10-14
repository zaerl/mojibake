/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <time.h>

#ifdef _WIN32
    #include <io.h>
    #include <direct.h>
    #include <windows.h>
    #include "../src/shell/getopt/getopt.h"
    #define isatty _isatty
    #define STDOUT_FILENO _fileno(stdout)
    #define chdir _chdir
#else
    #include <getopt.h>
    #include <unistd.h>
#endif

#include "test.h"

static att_test_callback error_callback = NULL;
static bool exit_on_error = false;

#ifdef _WIN32
// Windows-compatible strsep implementation
char *strsep(char **stringp, const char *delim) {
    char *start = *stringp;
    char *p;

    if(start == NULL) {
        return NULL;
    }

    p = strpbrk(start, delim);
    if(p) {
        *p = '\0';
        *stringp = p + 1;
    } else {
        *stringp = NULL;
    }

    return start;
}
#endif

/**
 * Get an UTF-8 string from a string of hex-encoded codepoints
 * Example: "0061 0062 0063", gives "abc"
 */
size_t get_string_from_codepoints(char *buffer, size_t size, char *codepoints) {
    char *token, *string, *tofree;
    tofree = string = strdup(buffer != NULL ? (buffer[0] == ' ' ? buffer + 1 : buffer) : "");
    unsigned int index = 0;

    while((token = strsep(&string, " ")) != NULL) {
        if(strlen(token) == 0) {
            continue; // Skip empty tokens
        }

        mjb_codepoint codepoint = strtoul((const char*)token, NULL, 16);
        if(codepoint == 0) {
            continue; // Skip invalid codepoints
        }

        unsigned int encoded_size = mjb_codepoint_encode(codepoint, codepoints + index,
            size - index, MJB_ENCODING_UTF_8);

        if(encoded_size == 0) {
            break; // Failed to encode
        }

        index += encoded_size;
    }

    codepoints[index] = '\0';
    free(tofree);

    return index;
}

int show_version(void) {
    printf("Mojibake v%s\n", MJB_VERSION);

    return 0;
}

bool is_exit_on_error(void) {
    return exit_on_error;
}

void set_error_callback(att_test_callback callback) {
    error_callback = callback;
}

int attractor_test_callback(int test, const char *description) {
    if(!test) {
        if(error_callback) {
            error_callback(test, description);
        }

        exit(1);
        return 1;
    }

    return 0;
}

void show_help(const char *executable, struct option options[], const char *descriptions[], const char *error) {
    FILE *stream = error ? stderr : stdout;

    fprintf(stream, "%s%smojibake - Mojibake test client [v%s]\n\nUsage: %s [OPTIONS]\n",
        error ? error : "",
        error ? "\n\n" : "",
        MJB_VERSION,
        executable);
    fprintf(stream, "Options:\n");

    for(unsigned long i = 0; i < 5; ++i) {
        fprintf(stream, "  -%c%s, --%s%s\n\t%s\n",
            options[i].val,
            options[i].has_arg == no_argument ? "" : " ARG",
            options[i].name,
            options[i].has_arg == no_argument ? "" : "=ARG",
            descriptions[i]);
    }
}

int main(int argc, char * const argv[]) {
#ifdef _WIN32
    LARGE_INTEGER frequency, start, end;
    QueryPerformanceFrequency(&frequency);
#else
    struct timespec start, end;
#endif
    double elapsed = 0;
    unsigned int verbosity = 0;
    int option = 0;
    int option_index = 0;
    char *filter = NULL;
    bool is_ctest = getenv("CTEST_INTERACTIVE_DEBUG_MODE") != NULL ||
        getenv("DASHBOARD_TEST_FROM_CTEST") != NULL;
    bool show_colors = isatty(STDOUT_FILENO) && getenv("NO_COLOR") == NULL && getenv("TERM") != NULL
        && strcmp(getenv("TERM"), "dumb") != 0;

    struct option long_options[] = {
        { "filter", required_argument, NULL, 'f' },
        { "help", no_argument, NULL, 'h' },
        { "verbose", no_argument, NULL, 'v' },
        { "version", no_argument, NULL, 'V' },
        { "exit-on-error", no_argument, NULL, 'e' },
        { NULL, 0, NULL, 0 }
    };
    const char *descriptions[] = {
        "Filter tests by name in the form name1,name2,...",
        "Show this help message",
        "Verbose output. -vv for more verbosity",
        "Print version",
        "Exit immediately on first test failure"
    };

    if(!is_ctest) {
#ifdef _WIN32
        QueryPerformanceCounter(&start);
#else
        clock_gettime(CLOCK_MONOTONIC, &start);
#endif
    }

    att_set_verbose(verbosity);

    while((option = getopt_long(argc, argv, "f:ehvV", long_options, &option_index)) != -1) {
        switch(option) {
            case 'f':
                filter = strdup(optarg);
                break;
            case 'e':
                exit_on_error = true;
                break;
            case 'h':
                show_help(argv[0], long_options, descriptions, NULL);
                return 0;
            case 'v':
                ++verbosity;
                break;
            case 'V':
                return show_version();
            case '?':
                // getopt_long already printed an error message
                break;
            default:
                abort();
        }
    }

    att_set_verbose(verbosity);

    if(exit_on_error) {
        att_set_test_callback(attractor_test_callback);
    }

    if(is_ctest) {
        // ctest CWD is /build/tests, so we need to set the db path to ./build/tests/mojibake.db
        chdir("../../");
    }

    unsigned int step = 0;

    #define RUN_TEST(NAME) \
        if(!filter || strstr(#NAME, filter)) { \
            if(!is_ctest) { \
                printf("%sTest: %s%s%s\n", verbosity && step ? "\n" : "", \
                    show_colors ? "\x1b[1;32m" : "", #NAME, show_colors ? "\x1b[0m" : ""); \
            } \
            test_##NAME(NULL); \
            ++step; \
        }

    // Start tests declarations.
    // RUN_TEST(breaking)
    RUN_TEST(case)
    RUN_TEST(cjk)
    RUN_TEST(codepoint)
    RUN_TEST(emoji)
    RUN_TEST(encoding)
    RUN_TEST(east_asian_width)
    RUN_TEST(hangul_composition)
    RUN_TEST(hangul)
    RUN_TEST(mojibake)
    RUN_TEST(next)
    RUN_TEST(normalization)
    RUN_TEST(plane)
    RUN_TEST(quick_check)
    RUN_TEST(segmentation)
    RUN_TEST(special_case)
    RUN_TEST(string)
    RUN_TEST(version)

#ifdef __cplusplus
    RUN_TEST(cpp_mojibake)
    RUN_TEST(cpp_normalization)
#endif

    unsigned int tests_valid = att_get_valid_tests();
    unsigned int tests_total = att_get_total_tests();
    bool valid = tests_valid == tests_total;

    // Green if valid and red if not
    const char *color_code = show_colors ? (valid ? "\x1B[32m" : "\x1B[31m") : "";

    printf("%sTests valid/run: %s%d/%d%s\n", verbosity >= 1 ? "\n" : "", color_code,
        tests_valid, tests_total, show_colors ? "\x1B[0m" : "");

    if(!is_ctest) {
#ifdef _WIN32
        QueryPerformanceCounter(&end);
        elapsed = (double)(end.QuadPart - start.QuadPart) / frequency.QuadPart;
#else
        clock_gettime(CLOCK_MONOTONIC, &end);
        elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
#endif
        printf("Execution time: %.4f seconds\n", elapsed);
    }

    if(filter) {
        free(filter);
    }

    return valid ? 0 : -1;
}
