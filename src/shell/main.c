/**
 * The Mojibake shell
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "../mojibake.h"
#include "characters.h"
#include "commands/commands.h"
#include "maps.h"
#include "shell.h"

int show_version(void) {
     printf("Mojibake %sv%s%s\n", color_green_start(), MJB_VERSION, color_reset());

     return 0;
 }

void show_help(struct option options[], const char *descriptions[], command commands[], const char *error) {
    FILE *stream = error ? stderr : stdout;

    fprintf(stream, "%s%sUsage: mojibake [options...] <command> [<args>]\n\nMojibake client [v%s]\n\n",
        error ? error : "",
        error ? "\n\n" : "",
        MJB_VERSION);
    fprintf(stream, "Options:\n");

    for(unsigned long i = 0; i < 5; ++i) {
        fprintf(stream, "  -%c%s, --%s%s\n\t%s\n",
            options[i].val,
            options[i].has_arg == no_argument ? "" : " <arg>",
            options[i].name,
            options[i].has_arg == no_argument ? "" : "=<arg>",
            descriptions[i]);
    }

    fprintf(stream, "\nCommands:\n");

    for(unsigned long i = 0; i < 5; ++i) {
        fprintf(stream, "  %s\n\t%s\n", commands[i].name, commands[i].description);
    }
}

bool get_interpret_mode(const char *input) {
    if(strcmp(input, "code") == 0) {
        cmd_interpret_mode = INTERPRET_MODE_CODEPOINT;
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

    // unsigned int columns = 80;

    struct option long_options[] = {
        { "help", no_argument, NULL, 'h' },
        { "json-indent", required_argument, NULL, 'j' },
        { "codepoint", no_argument, NULL, 'c' },
        { "output", required_argument, NULL, 'o' },
        { "verbose", no_argument, NULL, 'v' },
        { "version", no_argument, NULL, 'V' },
        { NULL, 0, NULL, 0 }
    };

    const char *descriptions[] = {
        "Print help",
        "JSON indent level (0-10). Default: 0\n",
        "Interpret input as a list of codepoints\n",
        "Output mode: plain, json. Default: plain\n"
        "\t\tplain: print the result in plain text\n"
        "\t\tjson: print the result in JSON format",
        "Verbose output",
        "Print version"
    };

    command commands[] = {
        { "char", "Print the character for the given codepoint", character_command, 0 },
        { "nfd", "Normalize the input to NFD", normalize_command, MJB_NORMALIZATION_NFD },
        { "nfkd", "Normalize the input to NFKD", normalize_command, MJB_NORMALIZATION_NFKD },
        { "nfc", "Normalize the input to NFC", normalize_command, MJB_NORMALIZATION_NFC },
        { "nfkc", "Normalize the input to NFKC", normalize_command, MJB_NORMALIZATION_NFKC }
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

    while((option = getopt_long(argc, argv, "hj:co:vV", long_options, &option_index)) != -1) {
        switch(option) {
            case 'h':
                show_help(long_options, descriptions, commands, NULL);
                return 0;
            case 'c':
                cmd_interpret_mode = INTERPRET_MODE_CODEPOINT;
                break;
            case 'j': {
                char *endptr = NULL;
                cmd_json_indent = strtoul(optarg, &endptr, 10);

                if(endptr == optarg || *endptr != '\0' || cmd_json_indent < 0 || cmd_json_indent > 10) {
                    fprintf(stderr, "JSON indent level must be a number between 0 and 10.\n");
                    show_help(long_options, descriptions, commands, NULL);

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
                    show_help(long_options, descriptions, commands, NULL);

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
        show_help(long_options, descriptions, commands, NULL);

        return 1;
    }

    if(argc - optind == 1) {
        fprintf(stderr, "No command value specified.\n");
        show_help(long_options, descriptions, commands, NULL);

        return 1;
    }

    int next_argc = argc - optind - 1;
    char *const *next_argv = argv + optind + 1;

    for(int i = 0; i < 5; ++i) {
        if(strcmp(argv[optind], commands[i].name) == 0) {
            return commands[i].function(next_argc, next_argv, commands[i].flags);
        }
    }

    fprintf(stderr, "Unknown command: %s\n", argv[optind]);
    show_help(long_options, descriptions, commands, NULL);

    return 0;
}
