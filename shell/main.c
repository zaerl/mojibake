/**
 * The Mojibake shell
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <getopt.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>

#include "../src/mojibake.h"
#include "maps.h"

static int cmd_show_colors = 0;
static bool cmd_verbose = false;

// Color formatting helper functions
static const char* color_green_start(void) {
    return cmd_show_colors ? "\x1B[32m" : "";
}

static const char* color_reset(void) {
    return cmd_show_colors ? "\x1B[0m" : "";
}

static void print_value(const char* label, const char* format, ...) {
    va_list args;
    va_start(args, format);

    printf("%s%s", label, color_green_start());
    vprintf(format, args);
    printf("%s\n", color_reset());

    va_end(args);
}

bool next_character(mjb_character *character) {
    printf(" %sU+%04X%s", color_green_start(), (unsigned int)character->codepoint, color_reset());

    return true;
}

bool parse_codepoint(const char *input, mjb_codepoint *value) {
    char *endptr;

    if(strncmp(input, "U+", 2) == 0 || strncmp(input, "u+", 2) == 0) {
        // Parse as hex after "U+" prefix
        *value = strtol(input + 2, &endptr, 16);

        return *endptr == '\0';
    } else {
        // Try parsing as hex
        *value = strtol(input, &endptr, 16);
    }

    return *endptr == '\0';
}

int show_version(void) {
    printf("Mojibake %sv%s%s\n", color_green_start(), MJB_VERSION, color_reset());

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

int character_command(int argc, char * const argv[]) {
    mjb_codepoint value = 0;

    if(!parse_codepoint(argv[0], &value)) {
        fprintf(stderr, cmd_verbose ? "Invalid\n" : "N\n");

        return 1;
    }

    mjb_character character = {0};

    if(!mjb_codepoint_character(&character, value)) {
        fprintf(stderr, cmd_verbose ? "Invalid\n" : "N\n");

        return 1;
    }

    char buffer_utf8[5];
    mjb_codepoint_encode(character.codepoint, buffer_utf8, 5, MJB_ENCODING_UTF_8);
    size_t utf8_length = strnlen(buffer_utf8, 5);

    // NFD
    size_t nfd_length = 0;
    char *nfd = mjb_normalize(buffer_utf8, utf8_length, &nfd_length, MJB_ENCODING_UTF_8, MJB_NORMALIZATION_NFD);

    // NFKD
    size_t nfkd_length = 0;
    char *nfkd = mjb_normalize(buffer_utf8, utf8_length, &nfkd_length, MJB_ENCODING_UTF_8, MJB_NORMALIZATION_NFKD);

    print_value("Codepoint: ", "U+%04X", (unsigned int)character.codepoint);
    print_value("Name: ", "%s", character.name);

    print_value("Character: ", "%s", buffer_utf8);

    printf("Hex UTF-8: %s", color_green_start());
    for(size_t i = 0; i < utf8_length; ++i) {
        printf("%02X ", (unsigned char)buffer_utf8[i]);
    }
    printf("%s\n", color_reset());

    print_value("NFD: ", "%s", nfd);
    printf("NFD normalization:");
    mjb_next_character(nfd, nfd_length, MJB_ENCODING_UTF_8, next_character);
    puts("");

    print_value("NFKD: ", "%s", nfkd);
    printf("NFKD normalization:");
    mjb_next_character(nfkd, nfkd_length, MJB_ENCODING_UTF_8, next_character);
    puts("");

    print_value("Category: ", "[%d] %s", character.category, category_name(character.category));

    char *cc_name = ccc_name(character.combining);
    print_value("Combining: ", "[%d] %s", character.combining, cc_name);
    free(cc_name);

    const char *bi_name = bidi_name(character.bidirectional);
    print_value("Bidirectional: ", "[%d] %s", character.bidirectional, bi_name);

    mjb_plane plane = mjb_codepoint_plane(character.codepoint);
    const char *plane_name = mjb_plane_name(plane, false);
    print_value("Plane: ", "[%d] %s", plane, plane_name);

    const char *d_name = decomposition_name(character.decomposition);
    print_value("Decomposition: ", "[%d] %s", character.decomposition, d_name);

    if(character.decimal == MJB_NUMBER_NOT_VALID) {
        print_value("Decimal: ", "N/A");
    } else {
        print_value("Decimal: ", "%d", character.decimal);
    }

    if(character.digit == MJB_NUMBER_NOT_VALID) {
        print_value("Digit: ", "N/A");
    } else {
        print_value("Digit: ", "%d", character.digit);
    }

    print_value("Numeric: ", "%s", character.numeric[0] != '\0' ? character.numeric : "N/A");
    print_value("Mirrored: ", "%s", character.mirrored ? "Y" : "N");

    if(character.uppercase != 0) {
        print_value("Uppercase: ", "%04X", (unsigned int)character.uppercase);
    }

    if(character.lowercase != 0) {
        print_value("Lowercase: ", "%04X", (unsigned int)character.lowercase);
    }

    if(character.titlecase != 0) {
        print_value("Titlecase: ", "%04X", (unsigned int)character.titlecase);
    }

    mjb_codepoint_block block = {0};

    bool valid_block = mjb_character_block(character.codepoint, &block);

    if(valid_block) {
        print_value("Block: ", "[%d %X-%X] %s", block.id, block.start, block.end, block.name);
    }

    mjb_free(nfd);
    mjb_free(nfkd);

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

    if(strcmp(argv[optind], "character") == 0) {
        return character_command(next_argc, next_argv);
    } else {
        fprintf(stderr, "Unknown command: %s\n", argv[optind]);
        show_help(argv[0], long_options, descriptions, NULL);

        return 1;
    }

    return 0;
}
