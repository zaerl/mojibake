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

#ifndef MJB_TEST_SOURCE_DIR
    #define MJB_TEST_SOURCE_DIR "."
#endif

#define MJB_TEST_COVERAGE_MAX_ENTRIES 256
#define MJB_TEST_COVERAGE_NAME_SIZE 128

static att_test_callback error_callback = NULL;
static bool exit_on_error = false;
static bool coverage_enabled = false;
static const char *coverage_output = NULL;

typedef struct {
    char name[MJB_TEST_COVERAGE_NAME_SIZE];
    unsigned long long count;
} mjb_test_coverage_entry;

static mjb_test_coverage_entry coverage_entries[MJB_TEST_COVERAGE_MAX_ENTRIES];
static size_t coverage_entry_count = 0;
static char coverage_current[MJB_TEST_COVERAGE_NAME_SIZE] = { 0 };

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

static bool coverage_name_char(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
        c == '_';
}

static bool coverage_name_from_expression(const char *expression, char *name, size_t size) {
    const char *found = expression;

    if(!expression || size == 0) {
        return false;
    }

    // Check if it's a mjb_*( function call.
    while((found = strstr(found, "mjb_")) != NULL) {
        size_t length = 0;
        const char *next;

        while(coverage_name_char(found[length])) {
            ++length;
        }

        next = found + length;

        while(*next == ' ' || *next == '\t' || *next == '\n') {
            ++next;
        }

        if(*next == '(' && length > 0 && length < size) {
            memcpy(name, found, length);
            name[length] = '\0';

            return true;
        }

        found += 4;
    }

    return false;
}

void mjb_test_coverage_clear(void) {
    coverage_current[0] = '\0';
}

void mjb_test_coverage_set(const char *name) {
    if(!name || !name[0]) {
        coverage_current[0] = '\0';

        return;
    }

    snprintf(coverage_current, sizeof(coverage_current), "%s", name);
}

static void coverage_record(const char *name) {
    if(!coverage_enabled || !name || !name[0]) {
        return;
    }

    for(size_t i = 0; i < coverage_entry_count; ++i) {
        if(strcmp(coverage_entries[i].name, name) == 0) {
            ++coverage_entries[i].count;

            return;
        }
    }

    if(coverage_entry_count >= MJB_TEST_COVERAGE_MAX_ENTRIES) {
        return;
    }

    snprintf(coverage_entries[coverage_entry_count].name,
        sizeof(coverage_entries[coverage_entry_count].name), "%s", name);
    coverage_entries[coverage_entry_count].count = 1;
    ++coverage_entry_count;
}

// Callback for test assertions to record coverage and handle errors
static int attractor_test_callback(int test, const char *description, const char *expression,
    const char *file, unsigned int line) {
    // Exit on error if the flag is set.
    if(exit_on_error) {
        if(!test) {
            if(error_callback) {
                error_callback(test, description, expression, file, line);
            }

            exit(1);

            return 1;
        }
    }

    // Save coverage for the current assertion if coverage is enabled.
    if(coverage_enabled) {
        char name[MJB_TEST_COVERAGE_NAME_SIZE];

        if(coverage_name_from_expression(expression, name, sizeof(name))) {
            // If it's a read mjb_* function, record it for coverage
            mjb_test_coverage_set(name);
        }

        if(coverage_current[0]) {
            coverage_record(coverage_current);
        }
    }

    return 0;
}

static bool write_coverage_file(void) {
    FILE *file;

    if(!coverage_output) {
        return true;
    }

    file = fopen(coverage_output, "w");

    if(file == NULL) {
        perror("coverage");

        return false;
    }

    fputs("{\n  \"coverage\": [\n", file);

    for(size_t i = 0; i < coverage_entry_count; ++i) {
        fprintf(file, "    { \"name\": \"%s\", \"count\": %llu }%s\n",
            coverage_entries[i].name,
            coverage_entries[i].count,
            i + 1 == coverage_entry_count ? "" : ",");
    }

    fputs("  ]\n}\n", file);
    fclose(file);

    return true;
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

void show_help(const char *executable, struct option options[], const char *descriptions[], const char *error) {
    FILE *stream = error ? stderr : stdout;

    fprintf(stream, "%s%smojibake - Mojibake test client [v%s]\n\nUsage: %s [OPTIONS]\n",
        error ? error : "",
        error ? "\n\n" : "",
        MJB_VERSION,
        executable);
    fprintf(stream, "Options:\n");

    for(unsigned long i = 0; options[i].name != NULL; ++i) {
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
    bool show_colors = false;

    if(isatty(STDOUT_FILENO)) {
        const char *no_color = getenv("NO_COLOR");

#ifdef _WIN32
        // On Windows, TERM is usually not set, so enable colors by default if NO_COLOR is not set
        show_colors = no_color == NULL;
#else
        // On Unix, check TERM environment variable
        const char *term = getenv("TERM");
        show_colors = no_color == NULL && term != NULL && strcmp(term, "dumb") != 0;
#endif
    }

    struct option long_options[] = {
        { "coverage", required_argument, NULL, 'C' },
        { "filter", required_argument, NULL, 'f' },
        { "help", no_argument, NULL, 'h' },
        { "verbose", no_argument, NULL, 'v' },
        { "version", no_argument, NULL, 'V' },
        { "exit-on-error", no_argument, NULL, 'e' },
        { NULL, 0, NULL, 0 }
    };
    const char *descriptions[] = {
        "Write runtime API coverage counts to the given JSON file",
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

    while((option = getopt_long(argc, argv, "C:f:ehvV", long_options, &option_index)) != -1) {
        switch(option) {
            case 'C':
                coverage_enabled = true;
                coverage_output = optarg;
                break;
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

    if(exit_on_error || coverage_enabled) {
        att_set_test_callback(attractor_test_callback);
    }

    if(is_ctest) {
        if(chdir(MJB_TEST_SOURCE_DIR) != 0) {
            // If the test source directory is not found, print an error message and exit
            perror("chdir");

            return 1;
        }
    }

    unsigned int step = 0;

    #define RUN_TEST(NAME) \
        if(!filter || strstr(#NAME, filter)) { \
            mjb_test_coverage_clear(); \
            if(!is_ctest) { \
                printf("%sTest: %s%s%s\n", verbosity && step ? "\n" : "", \
                    show_colors ? "\x1b[1;32m" : "", #NAME, show_colors ? "\x1b[0m" : ""); \
            } \
            test_##NAME(NULL); \
            ++step; \
        }

    // Start tests declarations.
    RUN_TEST(bidi)
    RUN_TEST(bidi_class)
    RUN_TEST(break_line)
    RUN_TEST(break_sentence)
    RUN_TEST(break_word)
    RUN_TEST(case)
    RUN_TEST(cjk)
    RUN_TEST(codepoint)
    RUN_TEST(collation)
    RUN_TEST(display)
    RUN_TEST(east_asian_width)
    RUN_TEST(embedded_null)
    RUN_TEST(emoji)
    RUN_TEST(encoding)
    RUN_TEST(example)
    RUN_TEST(filter)
    RUN_TEST(hangul_composition)
    RUN_TEST(hangul)
    RUN_TEST(identifier)
    RUN_TEST(locales)
    RUN_TEST(mojibake)
    RUN_TEST(next)
    RUN_TEST(normalization)
    RUN_TEST(plane)
    RUN_TEST(properties)
    RUN_TEST(quick_check)
    RUN_TEST(security)
    RUN_TEST(segmentation)
    RUN_TEST(special_case)
    RUN_TEST(string)
    RUN_TEST(utf)
    RUN_TEST(version)

#ifdef __cplusplus
    RUN_TEST(cpp_break)
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

    if(coverage_enabled && !write_coverage_file()) {
        return 1;
    }

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
