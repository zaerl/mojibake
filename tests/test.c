/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <getopt.h>
#include <time.h>

#include "test.h"

static att_test_callback error_callback = NULL;
static bool exit_on_error = false;

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
    struct timespec start, end;
    double elapsed = 0;
    unsigned int verbosity = 0;
    int option = 0;
    int option_index = 0;
    char *filter = NULL;

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

    clock_gettime(CLOCK_MONOTONIC, &start);
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

    unsigned int step = 0;

    #define RUN_TEST(NAME) \
        if(!filter || strstr(filter, #NAME)) { \
            printf("%sTest: \x1b[1;32m%s\x1b[0m\n", verbosity && step ? "\n" : "", #NAME); \
            test_##NAME(NULL); \
            ++step; \
        }

    // Start tests declarations.
    RUN_TEST(case)
    RUN_TEST(cjk)
    RUN_TEST(codepoint)
    RUN_TEST(encoding)
    RUN_TEST(hangul_composition)
    RUN_TEST(hangul)
    RUN_TEST(mojibake)
    RUN_TEST(next)
    RUN_TEST(normalization)
    RUN_TEST(plane)
    RUN_TEST(quick_check)
    RUN_TEST(special_case)
    RUN_TEST(string)
    // RUN_TEST(utf8)
    RUN_TEST(version)

    unsigned int tests_valid = att_get_valid_tests();
    unsigned int tests_total = att_get_total_tests();
    int valid = tests_valid == tests_total;

    // Green if valid and red if not
    const char *color_code = valid ? "\x1B[32m" : "\x1B[31m";

    printf("%sTests valid/run: %s%d/%d\n\x1B[0m", verbosity >= 1 ? "\n" : "", color_code, tests_valid, tests_total);

    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("Execution time: %.4f seconds\n", elapsed);

    if(filter) {
        free(filter);
    }

    return valid ? 0 : -1;
}
