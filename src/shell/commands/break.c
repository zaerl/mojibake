/**
 * The Mojibake shell
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "../shell.h"

typedef enum {
    MJBSH_BREAK_MODE_ALL,
    MJBSH_BREAK_MODE_GRAPHEME,
    MJBSH_BREAK_MODE_WORD,
    MJBSH_BREAK_MODE_LINE,
    MJBSH_BREAK_MODE_SENTENCE
} mjbsh_break_mode;

typedef struct {
    mjb_next_state grapheme;
    mjb_next_word_state word;
    mjb_next_line_state line;
    mjb_next_sentence_state sentence;
} mjbsh_break_state;

static bool mjbsh_break_parse_mode(const char *value, mjbsh_break_mode *mode) {
    if(strcmp(value, "all") == 0) {
        *mode = MJBSH_BREAK_MODE_ALL;

        return true;
    }

    if(strcmp(value, "grapheme") == 0 || strcmp(value, "graphemes") == 0) {
        *mode = MJBSH_BREAK_MODE_GRAPHEME;
        return true;
    }

    if(strcmp(value, "word") == 0 || strcmp(value, "words") == 0) {
        *mode = MJBSH_BREAK_MODE_WORD;

        return true;
    }

    if(strcmp(value, "line") == 0 || strcmp(value, "lines") == 0) {
        *mode = MJBSH_BREAK_MODE_LINE;

        return true;
    }

    if(strcmp(value, "sentence") == 0 || strcmp(value, "sentences") == 0) {
        *mode = MJBSH_BREAK_MODE_SENTENCE;

        return true;
    }

    return false;
}

static bool mjbsh_break_should_print(mjbsh_break_mode selected, mjbsh_break_mode current) {
    return selected == MJBSH_BREAK_MODE_ALL || selected == current;
}

typedef mjb_break_type (*mjbsh_break_function)(const char *input, size_t input_size,
    mjbsh_break_state *state);

static mjb_break_type mjbsh_next_grapheme_break(const char *input, size_t input_size,
    mjbsh_break_state *state) {
    return mjb_next_grapheme_break(input, input_size, MJB_ENC_UTF_8, &state->grapheme);
}

static mjb_break_type mjbsh_next_word_break(const char *input, size_t input_size,
    mjbsh_break_state *state) {
    return mjb_next_word_break(input, input_size, MJB_ENC_UTF_8, &state->word);
}

static mjb_break_type mjbsh_next_line_break(const char *input, size_t input_size,
    mjbsh_break_state *state) {
    return mjb_next_line_break(input, input_size, MJB_ENC_UTF_8, &state->line);
}

static mjb_break_type mjbsh_next_sentence_break(const char *input, size_t input_size,
    mjbsh_break_state *state) {
    return mjb_next_sentence_break(input, input_size, MJB_ENC_UTF_8, &state->sentence);
}

static const char *mjbsh_break_type_name(mjb_break_type type) {
    switch(type) {
        case MJB_BT_MANDATORY:
            return "mandatory";
        case MJB_BT_NO_BREAK:
            return "no_break";
        case MJB_BT_ALLOWED:
            return "allowed";
        case MJB_BT_NOT_SET:
        default:
            return "not_set";
    }
}

static void mjbsh_print_break_json_field(const char *name, bool *first) {
    printf("%s%s%s\"%s\":%s", *first ? "" : ",", mjbsh_jnl(), mjbsh_ji(), name,
        cmd_json_indent == 0 ? "" : " ");
    *first = false;
}

static void mjbsh_print_break_json_array(const char *name, const char *input, size_t input_size,
    mjbsh_break_function next_break, bool *first_field) {
    mjbsh_break_state state;
    bool first_break = true;
    mjb_break_type type;

    memset(&state, 0, sizeof(state));

    mjbsh_print_break_json_field(name, first_field);
    putchar('[');

    while((type = next_break(input, input_size, &state)) != MJB_BT_NOT_SET) {
        printf("%s%s\"%s\"", first_break ? "" : ",", first_break || cmd_json_indent == 0 ? "" : " ",
            mjbsh_break_type_name(type));
        first_break = false;
    }

    putchar(']');
}

static void mjbsh_print_break_json(const char *input, size_t input_size, size_t input_real_size,
    size_t display_width, mjbsh_break_mode mode) {
    bool first_field = true;

    putchar('{');

    mjbsh_print_break_json_field("raw_input_size", &first_field);
    printf("%zu", input_size);

    mjbsh_print_break_json_field("real_input_size", &first_field);
    printf("%zu", input_real_size);

    mjbsh_print_break_json_field("display_width", &first_field);
    printf("%zu", display_width);

    mjbsh_print_break_json_field("raw_bytes", &first_field);
    putchar('[');

    for(size_t i = 0; i < input_size; ++i) {
        printf("%s%s%u", i == 0 ? "" : ",", i == 0 || cmd_json_indent == 0 ? "" : " ",
            (unsigned char)input[i]);
    }

    putchar(']');

    if(mjbsh_break_should_print(mode, MJBSH_BREAK_MODE_GRAPHEME)) {
        mjbsh_print_break_json_array("grapheme_breaks", input, input_size,
            mjbsh_next_grapheme_break, &first_field);
    }

    if(mjbsh_break_should_print(mode, MJBSH_BREAK_MODE_WORD)) {
        mjbsh_print_break_json_array("word_breaks", input, input_size, mjbsh_next_word_break,
            &first_field);
    }

    if(mjbsh_break_should_print(mode, MJBSH_BREAK_MODE_LINE)) {
        mjbsh_print_break_json_array("line_breaks", input, input_size, mjbsh_next_line_break,
            &first_field);
    }

    if(mjbsh_break_should_print(mode, MJBSH_BREAK_MODE_SENTENCE)) {
        mjbsh_print_break_json_array("sentence_breaks", input, input_size,
            mjbsh_next_sentence_break, &first_field);
    }

    printf("%s}\n", mjbsh_jnl());
}

static void mjbsh_print_first_iteration(mjb_break_type first_bt, mjb_break_type bt, bool is_eot,
    mjb_codepoint previous_codepoint, mjb_codepoint current_codepoint) {
    mjbsh_print_break_symbol(first_bt);

    // First iteration: print the starting codepoint
    mjbsh_print_codepoint(previous_codepoint != MJB_CODEPOINT_NOT_VALID ? previous_codepoint :
                                                                          current_codepoint);

    mjbsh_print_break_symbol(bt);

    // If previous was valid, print current; if not, we already printed current
    if(previous_codepoint != MJB_CODEPOINT_NOT_VALID && !is_eot) {
        mjbsh_print_codepoint(current_codepoint);
    }
}

static void mjbsh_print_iteration(bool is_eot, mjb_break_type bt, mjb_codepoint current_codepoint) {
    mjbsh_print_break_symbol(bt);

    if(!is_eot) {
        mjbsh_print_codepoint(current_codepoint);
    }
}

static void mjbsh_print_grapheme_breaks(const char *input, size_t input_size) {
    bool first = true;
    mjb_break_type bt;

    printf("\n\nGrapheme cluster segmentation:\n");

    mjb_next_state segment_state;
    segment_state.index = 0;

    while((bt = mjb_next_grapheme_break(input, input_size, MJB_ENC_UTF_8, &segment_state)) !=
        MJB_BT_NOT_SET) {
        bool is_eot = (segment_state.index > input_size);

        if(first) {
            mjbsh_print_first_iteration(MJB_BT_ALLOWED, bt, is_eot,
                segment_state.previous_codepoint, segment_state.current_codepoint);

            first = false;
        } else {
            mjbsh_print_iteration(is_eot, bt, segment_state.current_codepoint);
        }
    }
}

static void mjbsh_print_word_breaks(const char *input, size_t input_size) {
    bool first = true;
    mjb_break_type bt;

    printf("\n\nWord breaking:\n");

    mjb_next_word_state word_state;
    word_state.index = 0;

    while((bt = mjb_next_word_break(input, input_size, MJB_ENC_UTF_8, &word_state)) !=
        MJB_BT_NOT_SET) {
        bool is_eot = (word_state.index > input_size);

        if(first) {
            mjbsh_print_first_iteration(MJB_BT_ALLOWED, bt, is_eot, word_state.previous_codepoint,
                word_state.current_codepoint);

            first = false;
        } else {
            mjbsh_print_iteration(is_eot, bt, word_state.current_codepoint);
        }
    }
}

static void mjbsh_print_line_breaks(const char *input, size_t input_size) {
    bool first = true;
    mjb_break_type bt;

    printf("\n\nLine breaking:\n");

    mjb_next_line_state line_state;
    line_state.index = 0;

    while((bt = mjb_next_line_break(input, input_size, MJB_ENC_UTF_8, &line_state)) !=
        MJB_BT_NOT_SET) {
        bool is_eot = (line_state.index > input_size);

        if(first) {
            mjbsh_print_first_iteration(MJB_BT_NO_BREAK, bt, is_eot, line_state.previous_codepoint,
                line_state.current_codepoint);

            first = false;
        } else {
            mjbsh_print_iteration(is_eot, bt, line_state.current_codepoint);
        }
    }
}

static void mjbsh_print_sentence_breaks(const char *input, size_t input_size) {
    bool first = true;
    mjb_break_type bt;

    printf("\n\nSentence breaking:\n");

    mjb_next_sentence_state sentence_state;
    sentence_state.index = 0;

    while((bt = mjb_next_sentence_break(input, input_size, MJB_ENC_UTF_8, &sentence_state)) !=
        MJB_BT_NOT_SET) {
        bool is_eot = (sentence_state.index > input_size);

        if(first) {
            mjbsh_print_first_iteration(MJB_BT_ALLOWED, bt, is_eot,
                sentence_state.previous_codepoint, sentence_state.current_codepoint);

            first = false;
        } else {
            mjbsh_print_iteration(is_eot, bt, sentence_state.current_codepoint);
        }
    }
}

static void mjbsh_print_break_analysis(const char *input, mjbsh_break_mode mode) {
    size_t input_size = strlen(input);
    size_t input_real_size = mjb_count_codepoints(input, input_size, MJB_ENC_UTF_8);
    size_t display_width;

    if(mjb_display_width(input, input_size, MJB_ENC_UTF_8, MJB_WIDTH_CONTEXT_AUTO,
           &display_width) != MJB_STATUS_OK) {
        display_width = 0;
    }

    if(cmd_output_mode == OUTPUT_MODE_JSON) {
        mjbsh_print_break_json(input, input_size, input_real_size, display_width, mode);
        return;
    }

    printf("Raw input size: %s%zu%s\n", mjbsh_red(), input_size, mjbsh_reset());
    printf("Real input size: %s%zu%s\n", mjbsh_yellow(), input_real_size, mjbsh_reset());
    printf("Display width: %s%zu%s\n", mjbsh_green(), display_width, mjbsh_reset());

    printf("\nRaw bytes: ");

    for(size_t i = 0; i < input_size; ++i) {
        unsigned char byte = (unsigned char)input[i];

        if(byte >= 0x21 && byte <= 0x7E) {
            printf("%c", byte);
        } else if(byte == 0x0A || byte == 0x0D) {
            printf("%s<%02X>%s%c", mjbsh_yellow(), byte, mjbsh_reset(), byte);
        } else {
            printf("%s<%02X>%s", mjbsh_yellow(), byte, mjbsh_reset());
        }
    }

    if(mjbsh_break_should_print(mode, MJBSH_BREAK_MODE_GRAPHEME)) {
        mjbsh_print_grapheme_breaks(input, input_size);
    }

    if(mjbsh_break_should_print(mode, MJBSH_BREAK_MODE_WORD)) {
        mjbsh_print_word_breaks(input, input_size);
    }

    if(mjbsh_break_should_print(mode, MJBSH_BREAK_MODE_LINE)) {
        mjbsh_print_line_breaks(input, input_size);
    }

    if(mjbsh_break_should_print(mode, MJBSH_BREAK_MODE_SENTENCE)) {
        mjbsh_print_sentence_breaks(input, input_size);
    }
}

static void mjbsh_display_break_output(const char *input) {
    mjbsh_clear_screen();
    printf("Break the input\n");
    printf("Ctrl+C to exit\n");

    if(input == NULL || strlen(input) == 0) {
        fflush(stdout);

        return;
    }

    mjbsh_print_break_analysis(input, MJBSH_BREAK_MODE_ALL);
    fflush(stdout);
}

static void mjbsh_handle_key(mjbsh_key key) {
    switch(key) {
        case MJBSH_KEY_LEFT:
            break;
        case MJBSH_KEY_RIGHT:
            break;
        case MJBSH_KEY_UP:
        case MJBSH_KEY_DOWN:
        default:
            break;
    }
}

int mjbsh_break_command(int argc, char *const argv[], unsigned int flags) {
    if(argc != 0) {
        mjbsh_break_mode mode = MJBSH_BREAK_MODE_ALL;
        const char *input = argv[0];

        if(argc > 1) {
            if(!mjbsh_break_parse_mode(argv[0], &mode)) {
                fprintf(stderr, "break: unknown mode: %s\n", argv[0]);
                fprintf(stderr, "break: expected all, grapheme, word, line, or sentence\n");

                return 1;
            }

            input = argv[1];
        }

        mjbsh_print_break_analysis(input, mode);

        return 0;
    }

    if(cmd_output_mode == OUTPUT_MODE_JSON) {
        fprintf(stderr, "break: JSON output requires an input\n");

        return 1;
    }

    mjbsh_screen_mode(mjbsh_display_break_output, mjbsh_handle_key);

    return 0;
}
