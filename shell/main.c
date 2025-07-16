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
#include "maps.h"

static int cmd_show_colors = 0;
static bool cmd_verbose = false;

bool next_character(mjb_character *character) {
    printf(cmd_show_colors ? " \x1B[32mU+%04X\x1B[0m" : " U+%04X", (unsigned int)character->codepoint);

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

int character_command(int argc, char * const argv[]) {
    mjb_codepoint value = 0;

    if(!parse_codepoint(argv[0], &value)) {
        fprintf(stderr, cmd_verbose ? "Invalid\n" : "N\n");

        return 1;
    }

    mjb_character character = {0};

    /*if(*endptr != '\0' || !mjb_codepoint_character(&character, value)) {
        fprintf(stderr, cmd_verbose ? "Invalid\n" : "N\n");

        return 1;
    }*/
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

    printf(cmd_show_colors ? "Codepoint: \x1B[32mU+%04X\x1B[0m\n" : "Codepoint: U+%04X\n", (unsigned int)character.codepoint);
    printf(cmd_show_colors ? "Name: \x1B[32m%s\x1B[0m\n" : "Name: %s\n", character.name);

    printf(cmd_show_colors ? "Character: \"\x1B[32m%s\x1B[0m\"\n" : "Character: \"%s\"\n", buffer_utf8);

    printf(cmd_show_colors ? "Hex UTF-8: \x1B[32m" : "Hex UTF-8: ");
    for(size_t i = 0; i < utf8_length; ++i) {
        printf("%02X ", (unsigned char)buffer_utf8[i]);
    }
    puts(cmd_show_colors ? "\x1B[0m" : "");

    printf(cmd_show_colors ? "NFD: \"\x1B[32m%s\x1B[0m\"\n" : "NFD: \"%s\"\n", nfd);
    printf("NFD normalization:");
    mjb_next_character(nfd, nfd_length, MJB_ENCODING_UTF_8, next_character);
    puts("");

    printf(cmd_show_colors ? "NFKD: \"\x1B[32m%s\x1B[0m\"\n" : "NFKD: \"%s\"\n", nfkd);
    printf("NFKD normalization:");
    mjb_next_character(nfkd, nfkd_length, MJB_ENCODING_UTF_8, next_character);
    puts("");

    printf(cmd_show_colors ? "Category: [\x1B[32m%d\x1B[0m] " : "Category: %d ", character.category);
    printf(cmd_show_colors ? "\x1B[32m%s\x1B[0m\n" : "%s\n", category_name(character.category));

    printf(cmd_show_colors ? "Combining: [\x1B[32m%d\x1B[0m] " : "Combining: [%d] ", character.combining);
    char *cc_name = ccc_name(character.combining);
    printf(cmd_show_colors ? "\x1B[32m%s\x1B[0m\n" : "%s\n", cc_name);
    free(cc_name);

    printf(cmd_show_colors ? "Bidirectional: [\x1B[32m%d\x1B[0m] " : "Bidirectional: [%d] ", character.bidirectional);
    const char *bi_name = bidi_name(character.bidirectional);
    printf(cmd_show_colors ? "\x1B[32m%s\x1B[0m\n" : "%s\n", bi_name);

    mjb_plane plane = mjb_codepoint_plane(character.codepoint);
    printf(cmd_show_colors ? "Plane: [\x1B[32m%d\x1B[0m] " : "Plane: [%d] ", plane);
    const char *plane_name = mjb_plane_name(plane, false);
    printf(cmd_show_colors ? "\x1B[32m%s\x1B[0m\n" : "%s\n", plane_name);

    printf(cmd_show_colors ? "Decomposition: [\x1B[32m%d\x1B[0m] " : "Decomposition: [%d] ", character.decomposition);
    const char *d_name = decomposition_name(character.decomposition);
    printf(cmd_show_colors ? "\x1B[32m%s\x1B[0m\n" : "%s\n", d_name);

    printf(cmd_show_colors ? "Decimal: \x1B[32m%d\x1B[0m\n" : "Decimal: %d\n", character.decimal);
    printf(cmd_show_colors ? "Digit: \x1B[32m%d\x1B[0m\n" : "Digit: %d\n", character.digit);
    printf(cmd_show_colors ? "Numeric: \x1B[32m%s\x1B[0m\n" : "Numeric: %s\n", character.numeric[0] != '\0' ? character.numeric : "N/A");
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

    mjb_codepoint_block block = {0};

    bool valid_block = mjb_character_block(character.codepoint, &block);

    if(valid_block) {
        printf(cmd_show_colors ? "Block: [\x1B[32m%d\x1B[0m] " : "Block: [%d] ", block.id);
        printf(cmd_show_colors ? "\x1B[32m%s\x1B[0m\n" : "%s\n", block.name);
        printf(cmd_show_colors ? "Block start: \x1B[32m%X\x1B[0m\n" : "Block start: %X\n", (unsigned int)block.start);
        printf(cmd_show_colors ? "Block end: \x1B[32m%X\x1B[0m\n" : "Block end: %X\n", (unsigned int)block.end);
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
