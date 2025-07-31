/**
 * The Mojibake shell
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "../src/mojibake.h"
#include "characters.h"
#include "commands/commands.h"
#include "maps.h"
#include "shell.h"

int show_version(void) {
     printf("Mojibake %sv%s%s\n", color_green_start(), MJB_VERSION, color_reset());

     return 0;
 }

void show_help(const char *executable, struct option options[], const char *descriptions[], const char *error) {
    FILE *stream = error ? stderr : stdout;

    fprintf(stream, "%s%smojibake - Mojibake client [v%s]\n\nUsage: %s [OPTIONS] <command> [<args>]\n",
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

bool get_interpret_mode(const char *input) {
    if(strcmp(input, "code") == 0) {
        cmd_interpret_mode = INTERPRET_MODE_CODEPOINT;
    } else if(strcmp(input, "dec") == 0) {
        cmd_interpret_mode = INTERPRET_MODE_DECIMAL;
    } else if(strcmp(input, "char") == 0) {
        cmd_interpret_mode = INTERPRET_MODE_CHARACTER;
    } else {
        return false;
    }

    return true;
}

int main(int argc, char * const argv[]) {
    int option = 0;
    int option_index = 0;

    // Global variables
    cmd_show_colors = 0;
    cmd_verbose = false;
    cmd_interpret_mode = INTERPRET_MODE_CODEPOINT;
    cmd_output_mode = OUTPUT_MODE_PLAIN;
    current_codepoint = MJB_CODEPOINT_NOT_VALID;

    // unsigned int columns = 80;

    struct option long_options[] = {
        { "help", no_argument, NULL, 'h' },
        { "interpret", required_argument, NULL, 'i' },
        { "output", required_argument, NULL, 'o' },
        { "verbose", no_argument, NULL, 'v' },
        { "version", no_argument, NULL, 'V' },
        { NULL, 0, NULL, 0 }
    };
    const char *descriptions[] = {
        "Print help",
        "Interpret mode: code (codepoint), dec (decimal), char (character). Default: code",
        "Output mode: plain (default), json",
        "Verbose output",
        "Print version"
    };

    if(isatty(STDOUT_FILENO)) {
        /*struct winsize w;

        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

        if(w.ws_col > 0) {
            columns = w.ws_col;
        }*/

        const char *term = getenv("TERM");
        const char *no_color = getenv("NO_COLOR");
        cmd_show_colors = no_color == NULL && term != NULL && strcmp(term, "dumb") != 0;
    }

    while((option = getopt_long(argc, argv, "hi:o:vV", long_options, &option_index)) != -1) {
        switch(option) {
            case 'h':
                show_help(argv[0], long_options, descriptions, NULL);
                return 0;
            case 'i':
                if(!get_interpret_mode(optarg)) {
                    fprintf(stderr, "Invalid interpret mode: %s\n", optarg);
                    show_help(argv[0], long_options, descriptions, NULL);

                    return 1;
                }

                break;
            case 'o':
                if(strcmp(optarg, "plain") == 0) {
                    cmd_output_mode = OUTPUT_MODE_PLAIN;
                } else if(strcmp(optarg, "json") == 0) {
                    cmd_output_mode = OUTPUT_MODE_JSON;
                } else {
                    fprintf(stderr, "Invalid output mode: %s\n", optarg);
                    show_help(argv[0], long_options, descriptions, NULL);

                    return 1;
                }
                break;
            case 'v':
                cmd_verbose = true;
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

    // After global options, the next argument is the subcommand
    if(optind >= argc) {
        fprintf(stderr, "No command specified.\n");
        show_help(argv[0], long_options, descriptions, NULL);

        return 1;
    }

    if(argc - optind == 1) {
        fprintf(stderr, "No command value specified.\n");
        show_help(argv[0], long_options, descriptions, NULL);

        return 1;
    }

    int next_argc = argc - optind - 1;
    char *const *next_argv = argv + optind + 1;

    if(strcmp(argv[optind], "character") == 0) {
        return character_command(next_argc, next_argv);
    } else if(strcmp(argv[optind], "nfd") == 0) {
        return normalize_command(next_argc, next_argv, MJB_NORMALIZATION_NFD);
    } else if(strcmp(argv[optind], "nfkd") == 0) {
        return normalize_command(next_argc, next_argv, MJB_NORMALIZATION_NFKD);
    } else {
        fprintf(stderr, "Unknown command: %s\n", argv[optind]);
        show_help(argv[0], long_options, descriptions, NULL);

        return 1;
    }

    return 0;
}
