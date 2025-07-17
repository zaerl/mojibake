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
#include <ctype.h>

#include "../src/mojibake.h"
#include "maps.h"

typedef enum {
    INTERPRET_MODE_CODEPOINT,
    INTERPRET_MODE_DECIMAL,
    INTERPRET_MODE_CHARACTER
} interpret_mode;

typedef enum {
    OUTPUT_MODE_PLAIN,
    OUTPUT_MODE_JSON
} output_mode;

static int cmd_show_colors = 0;
static bool cmd_verbose = false;
static interpret_mode cmd_interpret_mode = INTERPRET_MODE_CODEPOINT;
static output_mode cmd_output_mode = OUTPUT_MODE_PLAIN;
static mjb_codepoint current_codepoint = MJB_CODEPOINT_NOT_VALID;

// Color formatting helper functions
static const char* color_green_start(void) {
    return cmd_show_colors ? "\x1B[32m" : "";
}

static const char* color_reset(void) {
    return cmd_show_colors ? "\x1B[0m" : "";
}

void print_value(const char* label, const char* format, ...) {
    va_list args;
    va_start(args, format);

    if(cmd_output_mode == OUTPUT_MODE_JSON) {
        char json_label[256];

        json_label[0] = tolower(label[0]);
        strcpy(json_label + 1, label + 1);

        printf("  \"%s\": \"%s", json_label, color_green_start());
    } else {
        printf("%s: %s", label, color_green_start());
    }

    vprintf(format, args);
    printf("%s%s", color_reset(), cmd_output_mode == OUTPUT_MODE_JSON ? "\"" : "");

    if(cmd_output_mode == OUTPUT_MODE_JSON && strcmp(label, "Titlecase") != 0) {
        puts(",");
    } else {
        puts("");
    }

    va_end(args);
}

void print_null_value(const char* label) {
    if(cmd_output_mode == OUTPUT_MODE_JSON) {
        char json_label[256];

        json_label[0] = tolower(label[0]);
        strcpy(json_label + 1, label + 1);

        printf("  \"%s\": %snull%s", json_label, color_green_start(), color_reset());
    } else {
        printf("%s: %sN/A%s", label, color_green_start(), color_reset());
    }

    if(cmd_output_mode == OUTPUT_MODE_JSON && strcmp(label, "Titlecase") != 0) {
        puts(",");
    } else {
        puts("");
    }
}

bool next_character(mjb_character *character) {
    printf("%sU+%04X%s ", color_green_start(), (unsigned int)character->codepoint, color_reset());

    return true;
}

bool next_string_character(mjb_character *character) {
    char buffer_utf8[5];
    size_t size = mjb_codepoint_encode(character->codepoint, buffer_utf8, 5, MJB_ENCODING_UTF_8);

    if(!size) {
        return false;
    }

    printf("%s", buffer_utf8);

    return true;
}

bool next_current_character(mjb_character *character) {
    printf("next_current_character %d", character->codepoint);
    current_codepoint = character->codepoint;

    return false;
}

bool parse_codepoint(const char *input, mjb_codepoint *codepoint) {
    char *endptr;
    mjb_codepoint value = 0;

    if(cmd_interpret_mode == INTERPRET_MODE_CODEPOINT) {
        if(strncmp(input, "U+", 2) == 0 || strncmp(input, "u+", 2) == 0) {
            // Parse as hex after "U+" prefix
            value = strtoul(input + 2, &endptr, 16);
        } else {
            // Try parsing as hex
            value = strtoul(input, &endptr, 16);
        }
    } else if(cmd_interpret_mode == INTERPRET_MODE_DECIMAL) {
        value = strtoul(input, &endptr, 10);
    } else {
        mjb_next_character(input, strlen(input), MJB_ENCODING_UTF_8, next_current_character);

        if(current_codepoint == MJB_CODEPOINT_NOT_VALID) {
            return false;
        }

        *codepoint = current_codepoint;
        current_codepoint = MJB_CODEPOINT_NOT_VALID;

        return true;
    }

    if(*endptr != '\0') {
        return false;
    }

    *codepoint = value;

    return true;
}

bool parse_character(const char *input, mjb_character *character) {
    mjb_codepoint value = 0;

    if(!parse_codepoint(input, &value)) {
        return false;
    }

    return mjb_codepoint_character(character, value);
}

int character_command(int argc, char * const argv[]) {
    mjb_character character = {0};

    if(!parse_character(argv[0], &character)) {
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

    if(cmd_output_mode == OUTPUT_MODE_JSON) {
        puts("{");
    }

    print_value("Codepoint", "U+%04X", (unsigned int)character.codepoint);
    print_value("Name", "%s", character.name);

    print_value("Character", "%s", buffer_utf8);

    if(cmd_output_mode == OUTPUT_MODE_JSON) {
        printf("  \"hex_utf-8\": \"%s", color_green_start());
    } else {
        printf("Hex UTF-8: %s", color_green_start());
    }

    for(size_t i = 0; i < utf8_length; ++i) {
        printf("%02X ", (unsigned char)buffer_utf8[i]);
    }

    printf("%s%s\n", color_reset(), cmd_output_mode == OUTPUT_MODE_JSON ? "\"," : "");
    print_value(cmd_output_mode == OUTPUT_MODE_JSON ? "nfd" : "NFD", "%s", nfd);

    if(cmd_output_mode == OUTPUT_MODE_JSON) {
        printf("  \"nfd_normalization\": \"%s", color_green_start());
    } else {
        printf("NFD normalization: %s", color_green_start());
    }

    mjb_next_character(nfd, nfd_length, MJB_ENCODING_UTF_8, next_character);
    printf("%s%s\n", color_reset(), cmd_output_mode == OUTPUT_MODE_JSON ? "\"," : "");

    print_value(cmd_output_mode == OUTPUT_MODE_JSON ? "nfkd" : "NFKD", "%s", nfkd);

    if(cmd_output_mode == OUTPUT_MODE_JSON) {
        printf("  \"nfkd_normalization\": \"%s", color_green_start());
    } else {
        printf("NFKD normalization: %s", color_green_start());
    }

    mjb_next_character(nfkd, nfkd_length, MJB_ENCODING_UTF_8, next_character);
    printf("%s%s\n", color_reset(), cmd_output_mode == OUTPUT_MODE_JSON ? "\"," : "");

    print_value("Category", "[%d] %s", character.category, category_name(character.category));

    char *cc_name = ccc_name(character.combining);
    print_value("Combining", "[%d] %s", character.combining, cc_name);
    free(cc_name);

    const char *bi_name = bidi_name(character.bidirectional);
    print_value("Bidirectional", "[%d] %s", character.bidirectional, bi_name);

    mjb_plane plane = mjb_codepoint_plane(character.codepoint);
    const char *plane_name = mjb_plane_name(plane, false);
    print_value("Plane", "[%d] %s", plane, plane_name);

    mjb_codepoint_block block = {0};

    bool valid_block = mjb_character_block(character.codepoint, &block);

    if(valid_block) {
        print_value("Block", "[%d %X-%X] %s", block.id, block.start, block.end, block.name);
    }

    const char *d_name = decomposition_name(character.decomposition);
    print_value("Decomposition", "[%d] %s", character.decomposition, d_name);

    if(character.decimal == MJB_NUMBER_NOT_VALID) {
        print_null_value("Decimal");
    } else {
        print_value("Decimal", "%d", character.decimal);
    }

    if(character.digit == MJB_NUMBER_NOT_VALID) {
        print_null_value("Digit");
    } else {
        print_value("Digit", "%d", character.digit);
    }

    if(character.numeric[0] != '\0') {
        print_value("Numeric", "%s", character.numeric);
    } else {
        print_null_value("Numeric");
    }

    print_value("Mirrored", "%s", character.mirrored ? "Y" : "N");

    if(character.uppercase != 0) {
        print_value("Uppercase", "%04X", character.uppercase);
    } else {
        print_null_value("Uppercase");
    }

    if(character.lowercase != 0) {
        print_value("Lowercase", "%04X", (unsigned int)character.lowercase);
    } else {
        print_null_value("Lowercase");
    }

    if(character.titlecase != 0) {
        print_value("Titlecase", "%04X", (unsigned int)character.titlecase);
    } else {
        print_null_value("Titlecase");
    }

    if(cmd_output_mode == OUTPUT_MODE_JSON) {
        puts("}");
    }

    mjb_free(nfd);
    mjb_free(nfkd);

    return 0;
}

int normalize_string_command(int argc, char * const argv[], mjb_normalization form) {
    size_t size = 0;
    char *normalized = mjb_normalize(argv[0], strlen(argv[0]), &size, MJB_ENCODING_UTF_8, form);

    if(normalized == NULL) {
        fprintf(stderr, cmd_verbose ? "Invalid\n" : "N\n");

        return 1;
    }

    printf("%s", color_green_start());
    mjb_next_character(normalized, size, MJB_ENCODING_UTF_8, next_string_character);
    printf("%s", color_reset());
    puts("");

    mjb_free(normalized);

    return 0;
}

int normalize_command(int argc, char * const argv[], mjb_normalization form) {
    if(cmd_interpret_mode == INTERPRET_MODE_CHARACTER) {
        return normalize_string_command(argc, argv, form);
    }

    unsigned int index = 0;
    // 5 bytes per codepoint is more than enough.
    char codepoints[argc * 5];

    for(int i = 0; i < argc; ++i) {
        mjb_codepoint codepoint = 0;

        if(!parse_codepoint(argv[i], &codepoint)) {
            fprintf(stderr, cmd_verbose ? "Invalid\n" : "N\n");

            return 1;
        }

        index += mjb_codepoint_encode(codepoint, codepoints + index, (argc * 5) - index, MJB_ENCODING_UTF_8);
    }

    codepoints[++index] = '\0';

    size_t normalized_size;
    char *normalized = mjb_normalize(codepoints, index, &normalized_size, MJB_ENCODING_UTF_8, form);

    if(normalized == NULL) {
        fprintf(stderr, cmd_verbose ? "Invalid\n" : "N\n");
        mjb_free(normalized);

        return 1;
    }

    mjb_next_character(normalized, normalized_size, MJB_ENCODING_UTF_8, next_character);
    puts("");

    mjb_free(normalized);

    return 0;
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
