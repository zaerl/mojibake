/**
 * The Mojibake shell
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>

#include "../src/mojibake.h"

static int cmd_show_colors = 0;
static bool cmd_verbose = false;

int show_version(void) {
    printf(cmd_show_colors ? "Mojibake \x1B[32mv%s\x1B[0m\n" : "Mojibake v%s\n", MJB_VERSION);

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

    for(unsigned long i = 0; i < 2; ++i) {
        fprintf(stream, "  -%c%s, --%s%s\n\t%s\n",
            options[i].val,
            options[i].has_arg == no_argument ? "" : " ARG",
            options[i].name,
            options[i].has_arg == no_argument ? "" : "=ARG",
            descriptions[i]);
    }
}

int valid_codepoint_command(int argc, char * const argv[]) {
    char *endptr;
    long value = strtol(argv[0], &endptr, 16);

    if(*endptr != '\0' || !mjb_codepoint_is_valid(value)) {
        fprintf(stderr, cmd_verbose ? "Invalid\n" : "N\n");

        return 1;
    }

    printf(cmd_verbose ? "Valid\n" : "Y\n");

    return 0;
}

int character_command(int argc, char * const argv[]) {
    char *endptr;
    long value = strtol(argv[0], &endptr, 16);

    mjb_character character;

    /*if(*endptr != '\0' || !mjb_codepoint_character(&character, value)) {
        fprintf(stderr, cmd_verbose ? "Invalid\n" : "N\n");

        return 1;
    }*/
    if(*endptr != '\0' || !mjb_codepoint_is_valid(value)) {
        fprintf(stderr, cmd_verbose ? "Invalid\n" : "N\n");

        return 1;
    }

    if(!mjb_codepoint_character(&character, value)) {
        fprintf(stderr, cmd_verbose ? "Invalid\n" : "N\n");

        return 1;
    }

    printf(cmd_show_colors ? "Codepoint: \x1B[32m%04X\x1B[0m\n" : "Codepoint: %04X\n", (unsigned int)character.codepoint);
    printf(cmd_show_colors ? "Name: \x1B[32m%s\x1B[0m\n" : "Name: %s\n", character.name);
    printf(cmd_show_colors ? "Category: \x1B[32m%d\x1B[0m\n" : "Category: %d\n", character.category);
    printf(cmd_show_colors ? "Combining: \x1B[32m%d\x1B[0m\n" : "Combining: %d\n", character.combining);
    printf(cmd_show_colors ? "Bidirectional: \x1B[32m%d\x1B[0m\n" : "Bidirectional: %d\n", character.bidirectional);
    printf(cmd_show_colors ? "Decomposition: \x1B[32m%d\x1B[0m\n" : "Decomposition: %d\n", character.decomposition);
    printf(cmd_show_colors ? "Decimal: \x1B[32m%d\x1B[0m\n" : "Decimal: %d\n", character.decimal);
    printf(cmd_show_colors ? "Digit: \x1B[32m%d\x1B[0m\n" : "Digit: %d\n", character.digit);
    printf(cmd_show_colors ? "Numeric: \x1B[32m%s\x1B[0m\n" : "Numeric: %s\n", character.numeric ? character.numeric : "N/A");
    printf(cmd_show_colors ? "Mirrored: \x1B[32m%s\x1B[0m\n" : "Mirrored: %s\n", character.mirrored ? "Y" : "N");

    if(character.uppercase != 0) {
        printf(cmd_show_colors ? "Uppercase: \x1B[32m%04X\x1B[0m\n" : "Uppercase: %04X\n", (unsigned int)character.uppercase);
    }

    if(character.lowercase != 0) {
        printf(cmd_show_colors ? "Lowercase: \x1B[32m%04X\x1B[0m\n" : "Lowercase: %04X\n", (unsigned int)character.lowercase);
    }

    if(character.titlecase != 0) {
        printf(cmd_show_colors ? "Titlecase: \x1B[32m%04X\x1B[0m\n" : "Titlecase: %04X\n", (unsigned int)character.titlecase);
    }

    mjb_codepoint_block block;

    bool valid_block = mjb_character_block(character.codepoint, &block);

    if(valid_block) {
        printf(cmd_show_colors ? "Block: \x1B[32m%s\x1B[0m\n" : "Block: %s\n", block.name);
    }

    return 0;
}

int main(int argc, char * const argv[]) {
    int option = 0;
    int option_index = 0;
    // unsigned int columns = 80;

    struct option long_options[] = {
        { "help", no_argument, NULL, 'h' },
        { "version", no_argument, NULL, 'V' },
        { NULL, 0, NULL, 0 }
    };
    const char *descriptions[] = {
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

    while((option = getopt_long(argc, argv, "hvV", long_options, &option_index)) != -1) {
        switch(option) {
            case 'h':
                show_help(argv[0], long_options, descriptions, NULL);
                return 0;
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

    if(strcmp(argv[optind], "valid-codepoint") == 0) {
        return valid_codepoint_command(next_argc, next_argv);
    } else if(strcmp(argv[optind], "character") == 0) {
        return character_command(next_argc, next_argv);
    } else {
        fprintf(stderr, "Unknown command: %s\n", argv[optind]);
        show_help(argv[0], long_options, descriptions, NULL);

        return 1;
    }

    return 0;
}
