/**
 * The Mojibake shell
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #include <io.h>
    #include "getopt/getopt.h"
    #define isatty _isatty
    #define STDOUT_FILENO _fileno(stdout)
#else
    #include <getopt.h>
    #include <unistd.h>
#endif

#include "../mojibake.h"
#include "characters.h"
#include "commands/commands.h"
#include "maps.h"
#include "shell.h"

static int mjbsh_show_version(void) {
    mjb_character character;
    bool valid = mjb_codepoint_character(MJB_VERSION_NUMBER, &character);

    printf("Mojibake %sv%s [%s]%s\n", mjbsh_green(), mjb_version(), valid ? character.name : "UNKNOWN", mjbsh_reset());

    return 0;
 }

static void mjbsh_show_help(struct option options[], const char *descriptions[], mjbsh_command commands[], const char *error) {
    FILE *stream = error ? stderr : stdout;

    fprintf(stream, "%s%sUsage: mojibake [options...] <command> [<args>]\n\nMojibake client [v%s]\n\n",
        error ? error : "",
        error ? "\n\n" : "",
        MJB_VERSION);
    fprintf(stream, "Options:\n");

    for(unsigned long i = 0; options[i].val != 0; ++i) {
        fprintf(stream, "  -%c%s, --%s%s\n\t%s\n",
            options[i].val,
            options[i].has_arg == no_argument ? "" : " <arg>",
            options[i].name,
            options[i].has_arg == no_argument ? "" : "=<arg>",
            descriptions[i]);
    }

    fprintf(stream, "\nCommands:\n");

    for(unsigned long i = 0; commands[i].name != NULL; ++i) {
        fprintf(stream, "  %s\n\t%s\n", commands[i].name, commands[i].description);
    }
}

/**
 * The Mojibake shell
 *
 * EX SIGNIS ORDO
 */
int main(int argc, char * const argv[]) {
    int option = 0;
    int option_index = 0;

#ifdef _WIN32
    UINT original_cp = GetConsoleOutputCP();
    SetConsoleOutputCP(CP_UTF8);
#endif

    // unsigned int columns = 80;

    struct option long_options[] = {
        { "codepoint", no_argument, NULL, 'c' },
        { "help", no_argument, NULL, 'h' },
        { "json-indent", required_argument, NULL, 'j' },
        { "output", required_argument, NULL, 'o' },
        { "verbose", no_argument, NULL, 'v' },
        { "version", no_argument, NULL, 'V' },
        { "width", no_argument, NULL, 'w' },
        { NULL, 0, NULL, 0 }
    };

    const char *descriptions[] = {
        "Interpret input as a list of codepoints",
        "Print help",
        "JSON indent level (0-10). Default: 0",
        "Output mode: plain, json. Default: plain\n"
        "\t\tplain: print the result in plain text\n"
        "\t\tjson: print the result in JSON format",
        "Verbose output",
        "Print version",
        "Width of output"
    };

    mjbsh_command commands[] = {
        { "break", "Break the input into line breaks", mjbsh_break_command, 0 },
        { "segment", "Segment the input into grapheme clusters", mjbsh_segment_command, 0 },
        { "char", "Print the characters for the given string", mjbsh_character_command, 0 },
        { "codepoint", "Print the character for the given codepoint", mjbsh_codepoint_command, 0 },
        { "filter", "Filter the input", mjbsh_filter_command,
            MJB_FILTER_NORMALIZE | MJB_FILTER_SPACES | MJB_FILTER_COLLAPSE_SPACES |
            MJB_FILTER_CONTROLS | MJB_FILTER_NUMERIC },
        { "nfd", "Normalize the input to NFD", mjbsh_normalize_command, MJB_NORMALIZATION_NFD },
        { "nfkd", "Normalize the input to NFKD", mjbsh_normalize_command, MJB_NORMALIZATION_NFKD },
        { "nfc", "Normalize the input to NFC", mjbsh_normalize_command, MJB_NORMALIZATION_NFC },
        { "nfkc", "Normalize the input to NFKC", mjbsh_normalize_command, MJB_NORMALIZATION_NFKC },
        { "upper", "Convert the input to uppercase", mjbsh_case_command, MJB_CASE_UPPER },
        { "lower", "Convert the input to lowercase", mjbsh_case_command, MJB_CASE_LOWER },
        { "title", "Convert the input to titlecase", mjbsh_case_command, MJB_CASE_TITLE },
        { NULL, NULL, NULL, 0 }
    };

    if(isatty(STDOUT_FILENO)) {
        /*struct winsize w;

        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

        if(w.ws_col > 0) {
            columns = w.ws_col;
        }*/

        const char *no_color = getenv("NO_COLOR");

#ifdef _WIN32
        // On Windows, TERM is usually not set, so enable colors by default if NO_COLOR is not set
        cmd_show_colors = no_color == NULL;
#else
        // On Unix, check TERM environment variable
        const char *term = getenv("TERM");
        cmd_show_colors = no_color == NULL && term != NULL && strcmp(term, "dumb") != 0;
#endif
    }

#ifdef _WIN32
    if(cmd_show_colors) {
        // On Windows, we need to enable ANSI escape codes for stdout and stderr
        HANDLE h_out = GetStdHandle(STD_OUTPUT_HANDLE);
        HANDLE h_err = GetStdHandle(STD_ERROR_HANDLE);
        DWORD mode_out, mode_err;

        // Enable ANSI escape codes for stdout
        if(h_out != INVALID_HANDLE_VALUE && GetConsoleMode(h_out, &mode_out)) {
            mode_out |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;

            if(!SetConsoleMode(h_out, mode_out)) {
                // If we can't enable ANSI codes, disable colors
                cmd_show_colors = 0;
            }
        } else {
            // If we can't get console mode, disable colors
            cmd_show_colors = 0;
        }

        // Enable ANSI escape codes for stderr
        if(cmd_show_colors && h_err != INVALID_HANDLE_VALUE && GetConsoleMode(h_err, &mode_err)) {
            mode_err |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(h_err, mode_err);
        }
    }
#endif

    while((option = getopt_long(argc, argv, "hj:co:vVw:", long_options, &option_index)) != -1) {
        char *endptr = NULL;

        switch(option) {
            case 'h':
                mjbsh_show_help(long_options, descriptions, commands, NULL);
                return 0;
            case 'c':
                cmd_interpret_mode = INTERPRET_MODE_CODEPOINT;
                break;
            case 'j': {
                endptr = NULL;
                cmd_json_indent = strtoul(optarg, &endptr, 10);

                if(endptr == optarg || *endptr != '\0' || cmd_json_indent > 10) {
                    fprintf(stderr, "JSON indent level must be a number between 0 and 10.\n");
                    mjbsh_show_help(long_options, descriptions, commands, NULL);

                    return 1;
                }

                break;
            }
            case 'o':
                if(strcmp(optarg, "plain") == 0) {
                    cmd_output_mode = OUTPUT_MODE_PLAIN;
                } else if(strcmp(optarg, "json") == 0) {
                    cmd_output_mode = OUTPUT_MODE_JSON;
                } else {
                    fprintf(stderr, "Invalid output mode: %s\n", optarg);
                    mjbsh_show_help(long_options, descriptions, commands, NULL);

                    return 1;
                }
                break;
            case 'v':
                ++cmd_verbose;
                break;
            case 'V':
                return mjbsh_show_version();
            case 'w':
                endptr = NULL;
                cmd_width = strtoul(optarg, &endptr, 10);

                if(endptr == optarg || *endptr != '\0' || cmd_width == 0 || cmd_width > 100) {
                    fprintf(stderr, "Output width must be a number between 1 and 100.\n");
                    mjbsh_show_help(long_options, descriptions, commands, NULL);

                    return 1;
                }

                break;
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
        mjbsh_show_help(long_options, descriptions, commands, NULL);

        return 1;
    }

    if(argc - optind == 1) {
        // Break command has a realtime mode
        if(!(
            strcmp(argv[optind], "break") == 0 ||
            strcmp(argv[optind], "filter") == 0 ||
            strcmp(argv[optind], "segment") == 0
        )) {
            fprintf(stderr, "No command value specified.\n");
            mjbsh_show_help(long_options, descriptions, commands, NULL);

            return 1;
        }
    }

    int next_argc = argc - optind - 1;
    char *const *next_argv = argv + optind + 1;

    for(int i = 0; commands[i].name != NULL; ++i) {
        if(strcmp(argv[optind], commands[i].name) == 0) {
            return commands[i].function(next_argc, next_argv, commands[i].flags);
        }
    }

    fprintf(stderr, "Unknown command: %s\n", argv[optind]);
    mjbsh_show_help(long_options, descriptions, commands, NULL);

#ifdef _WIN32
    SetConsoleOutputCP(originalCP);
#endif

    return 0;
}
