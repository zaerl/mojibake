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
    mjb_grapheme_break_state grapheme;
    mjb_word_break_state word;
    mjb_line_break_state line;
    mjb_sentence_break_state sentence;
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

// Return the next break type and store the byte offset it refers to in position.
typedef mjb_break_type (*mjbsh_break_function)(const char *input, size_t input_size,
    mjbsh_break_state *state, size_t *position);

static mjb_break_type mjbsh_next_grapheme_break(const char *input, size_t input_size,
    mjbsh_break_state *state, size_t *position) {
    mjb_text text = mjb_text_utf8(input, input_size);
    mjb_break_type type = mjb_grapheme_break_next(&text, &state->grapheme);
    *position = state->grapheme.position;

    return type;
}

static mjb_break_type mjbsh_next_word_break(const char *input, size_t input_size,
    mjbsh_break_state *state, size_t *position) {
    mjb_text text = mjb_text_utf8(input, input_size);
    mjb_break_type type = mjb_word_break_next(&text, &state->word);
    *position = state->word.position;

    return type;
}

static mjb_break_type mjbsh_next_line_break(const char *input, size_t input_size,
    mjbsh_break_state *state, size_t *position) {
    mjb_text text = mjb_text_utf8(input, input_size);
    mjb_break_type type = mjb_line_break_next(&text, &state->line);
    *position = state->line.position;

    return type;
}

static mjb_break_type mjbsh_next_sentence_break(const char *input, size_t input_size,
    mjbsh_break_state *state, size_t *position) {
    mjb_text text = mjb_text_utf8(input, input_size);
    mjb_break_type type = mjb_sentence_break_next(&text, &state->sentence);
    *position = state->sentence.position;

    return type;
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
    size_t position = 0;

    memset(&state, 0, sizeof(state));

    mjbsh_print_break_json_field(name, first_field);
    putchar('[');

    while((type = next_break(input, input_size, &state, &position)) != MJB_BT_NOT_SET) {
        printf("%s%s\"%s\"", first_break ? "" : ",", first_break || cmd_json_indent == 0 ? "" : " ",
            mjbsh_break_type_name(type));
        first_break = false;
    }

    putchar(']');
}

static void mjbsh_print_break_json(const char *input, size_t input_size, size_t input_real_size,
    size_t terminal_width, mjb_status terminal_width_status, mjbsh_break_mode mode) {
    bool first_field = true;

    putchar('{');

    mjbsh_print_break_json_field("raw_input_size", &first_field);
    printf("%zu", input_size);

    mjbsh_print_break_json_field("real_input_size", &first_field);
    printf("%zu", input_real_size);

    mjbsh_print_break_json_field("terminal_width", &first_field);

    if(terminal_width_status == MJB_STATUS_OK) {
        printf("%zu", terminal_width);
    } else {
        printf("null");
    }

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

// Print the codepoints of input[start, end), with U+FFFD for malformed bytes.
static void mjbsh_print_break_segment(const char *input, size_t start, size_t end) {
    mjb_text text = mjb_text_utf8(input + start, end - start);
    size_t offset = 0;
    mjb_codepoint codepoint;

    while(mjb_decode_next(&text, MJB_MALFORMED_REPLACE, &offset, &codepoint, NULL) ==
        MJB_STATUS_OK) {
        mjbsh_print_codepoint(codepoint);
    }
}

// Print the input with the break type of each position between its codepoints.
static void mjbsh_print_breaks(const char *title, const char *input, size_t input_size,
    mjbsh_break_function next_break, mjb_break_type first_bt) {
    mjbsh_break_state state;
    mjb_break_type bt;
    size_t previous = 0;
    size_t position = 0;

    memset(&state, 0, sizeof(state));
    printf("\n\n%s:\n", title);
    mjbsh_print_break_symbol(first_bt);

    while((bt = next_break(input, input_size, &state, &position)) != MJB_BT_NOT_SET) {
        mjbsh_print_break_segment(input, previous, position);
        mjbsh_print_break_symbol(bt);
        previous = position;
    }

    // An incomplete sequence at the end of the input has no break position.
    mjbsh_print_break_segment(input, previous, input_size);
}

static void mjbsh_print_break_analysis(const char *input, mjbsh_break_mode mode) {
    size_t input_size = strlen(input);
    size_t input_real_size = 0;

    mjb_text text = mjb_text_utf8(input, input_size);

    if(mjb_codepoint_count(&text, MJB_MALFORMED_STOP, &input_real_size, NULL) != MJB_STATUS_OK) {
        input_real_size = 0;
    }
    size_t terminal_width = 0;
    text = mjb_text_utf8(input, input_size);
    mjb_status terminal_width_status = mjb_terminal_width(&text, MJB_MALFORMED_STOP,
        MJB_TERMINAL_WIDTH_NARROW, &terminal_width, NULL);

    if(cmd_output_mode == OUTPUT_MODE_JSON) {
        mjbsh_print_break_json(input, input_size, input_real_size, terminal_width,
            terminal_width_status, mode);
        return;
    }

    printf("Raw input size: %s%zu%s\n", mjbsh_red(), input_size, mjbsh_reset());
    printf("Real input size: %s%zu%s\n", mjbsh_yellow(), input_real_size, mjbsh_reset());
    if(terminal_width_status == MJB_STATUS_OK) {
        printf("Terminal width: %s%zu%s\n", mjbsh_green(), terminal_width, mjbsh_reset());
    } else {
        printf("Terminal width: %sunsupported%s (%s)\n", mjbsh_yellow(), mjbsh_reset(),
            mjb_status_message(terminal_width_status));
    }

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
        mjbsh_print_breaks("Grapheme cluster segmentation", input, input_size,
            mjbsh_next_grapheme_break, MJB_BT_ALLOWED);
    }

    if(mjbsh_break_should_print(mode, MJBSH_BREAK_MODE_WORD)) {
        mjbsh_print_breaks("Word breaking", input, input_size, mjbsh_next_word_break,
            MJB_BT_ALLOWED);
    }

    if(mjbsh_break_should_print(mode, MJBSH_BREAK_MODE_LINE)) {
        mjbsh_print_breaks("Line breaking", input, input_size, mjbsh_next_line_break,
            MJB_BT_NO_BREAK);
    }

    if(mjbsh_break_should_print(mode, MJBSH_BREAK_MODE_SENTENCE)) {
        mjbsh_print_breaks("Sentence breaking", input, input_size, mjbsh_next_sentence_break,
            MJB_BT_ALLOWED);
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
    // The break view does not react to arrow keys yet.
    (void)key;
}

int mjbsh_break_command(int argc, char *const argv[], unsigned int flags) {
    (void)flags;

    if(argc != 0) {
        mjbsh_break_mode mode = MJBSH_BREAK_MODE_ALL;
        const char *input = argv[0];

        if(argc > 1) {
            if(!mjbsh_break_parse_mode(argv[0], &mode)) {
                return mjbsh_error("break: unknown mode: %s", argv[0]);
            }

            input = argv[1];
        }

        mjbsh_print_break_analysis(input, mode);

        return 0;
    }

    if(cmd_output_mode == OUTPUT_MODE_JSON) {
        return mjbsh_error("break: JSON output requires an input");
    }

    mjbsh_screen_mode(mjbsh_display_break_output, mjbsh_handle_key);

    return 0;
}
