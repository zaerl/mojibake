/**
 * The Mojibake shell
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
    #include <sys/select.h>
#endif

#include "../../mojibake.h"
#include "../../utf8.h"
#include "../shell.h"
#include "commands.h"

static void flush_line(unsigned int column) {
    // Fill remaining space to reach cmd_width total content
    while(column <= cmd_width) {
        printf(" ");
        ++column;
    }

    if(column > cmd_width + 1) {
        puts("");
    } else {
        printf("│\n");
    }
}

static mjb_codepoint control_picture_codepoint(mjb_codepoint codepoint) {
    if(codepoint < 0x20) {
        // Add 0x2400 to the codepoint to make it a printable character by using the
        // "Control Pictures" block.
        codepoint += 0x2400;
    } else if(codepoint == 0x20) {
        codepoint = 0x2423;
    } else if(codepoint == 0x7F) {
        // The delete character.
        codepoint = 0x2421;
    }

    return codepoint;
}

static void print_break_analysis(const char* input) {
    size_t input_size = strlen(input);
    size_t input_real_size = mjb_strnlen(input, input_size, MJB_ENCODING_UTF_8);
    size_t output_size = 0;
    mjb_line_break *line_breaks = mjb_break_line(input, input_size, MJB_ENCODING_UTF_8,
        &output_size);

    if(line_breaks == NULL) {
        puts("Could not analyze string");
        fflush(stdout);
        return;
    }

    uint8_t state = MJB_UTF_ACCEPT;
    mjb_codepoint codepoint;

    for(size_t i = 0; i < input_size; ++i) {
        state = mjb_utf8_decode_step(state, input[i], &codepoint);

        if(state == MJB_UTF_ACCEPT) {
            mjb_codepoint picture_codepoint = control_picture_codepoint(codepoint);
            bool is_control_picture = picture_codepoint != codepoint;
            char buffer_utf8[5];
            mjb_codepoint_encode(picture_codepoint, buffer_utf8, 5, MJB_ENCODING_UTF_8);

            if(is_control_picture) {
                printf("%s%s%s", color_green_start(), buffer_utf8, color_reset());
            } else {
                printf("%s", buffer_utf8);
            }
        }
    }

    puts("");

    size_t breaks_index = 0;
    bool check_break = true;
    state = MJB_UTF_ACCEPT;

    if(output_size > 0) {
        for(size_t i = 0; i < input_real_size; ++i) {
            if(check_break && i == line_breaks[breaks_index].index) {
                printf("%s%s%s", color_green_start(), line_breaks[breaks_index].mandatory ? "!" :
                    "÷", color_reset());

                if(breaks_index == output_size - 1) {
                    check_break = false;
                    continue;
                }

                ++breaks_index;
            } else {
                printf("×");
            }
        }

        puts("");
        fflush(stdout);
    }

    printf("┌");

    for(unsigned int i = 0; i < cmd_width; i++) {
        printf("─");
    }

    printf("┐\n");

    unsigned int column = 0;
    unsigned int current_i = 0;
    check_break = output_size > 0;
    breaks_index = 0;
    state = MJB_UTF_ACCEPT;
    codepoint = 0x0;

    for(size_t i = 0; i < input_size; ++i) {
        state = mjb_utf8_decode_step(state, input[i], &codepoint);

        if(state == MJB_UTF_ACCEPT) {
            bool can_break = false;
            bool is_mandatory = false;

            if(check_break && line_breaks[breaks_index].index == current_i) {
                can_break = true;
                is_mandatory = line_breaks[breaks_index].mandatory;

                if(breaks_index == output_size - 1) {
                    check_break = false;
                } else {
                    ++breaks_index;
                }
            }

            if(column == 0) {
                printf("│");
                column = 1;
            }

            if(can_break && is_mandatory) {
                flush_line(column);
                printf("│");
                column = 1;
            }

            unsigned int char_width = 1;
            bool is_overflow = column + char_width > cmd_width + 1;
            mjb_codepoint picture_codepoint = control_picture_codepoint(codepoint);
            bool is_control_picture = picture_codepoint != codepoint;

            char buffer_utf8[5];
            mjb_codepoint_encode(picture_codepoint, buffer_utf8, 5, MJB_ENCODING_UTF_8);

            if(is_overflow) {
                printf("%s%s%s", color_red_start(), buffer_utf8, color_reset());
            } else {
                if(is_control_picture) {
                    printf("%s%s%s", color_green_start(), buffer_utf8, color_reset());
                } else {
                    printf("%s", buffer_utf8);
                }
            }

            column += char_width;
            ++current_i;
        }
    }

    if(column != 0) {
        flush_line(column);
    }

    printf("└");

    for(unsigned int i = 0; i < cmd_width; i++) {
        printf("─");
    }

    printf("┘\n");

    free(line_breaks);
}

static void display_break_output(const char* input) {
    clear_screen();
    printf("Break the input into line breaks\n");
    printf("Ctrl+C or ESC to exit\n");

    if(input == NULL || strlen(input) == 0) {
        fflush(stdout);

        return;
    }

    puts(input);
    print_break_analysis(input);
    fflush(stdout);
}

int break_command(int argc, char * const argv[], unsigned int flags) {
    if(argc != 0) {
        print_break_analysis(argv[0]);

        return 0;
    }

    terminal_state term_state;

#ifdef _WIN32
    // Windows-specific initialization
    term_state.h_stdin = GetStdHandle(STD_INPUT_HANDLE);
    GetConsoleMode(term_state.h_stdin, &term_state.orig_mode);
#else
    // Unix-specific initialization
    tcgetattr(STDIN_FILENO, &term_state);
#endif

    char input_buffer[1024] = {0};
    size_t buffer_pos = 0;
    char c;

    display_break_output("");
    set_raw_mode(&term_state);

    while(1) {
#ifdef _WIN32
        // Windows input handling
        if(_kbhit()) {
            c = (char)_getch();

            if(c == 3 || c == 27) { // Ctrl+C or ESC
                break;
            } else if(c == 127 || c == 8) { // DELETE or BACKSPACE
                if(buffer_pos > 0) {
                    --buffer_pos;
                    input_buffer[buffer_pos] = '\0';
                    display_break_output(input_buffer);
                }
            } else {
                if(buffer_pos < sizeof(input_buffer) - 1) {
                    input_buffer[buffer_pos] = c;
                    ++buffer_pos;
                    input_buffer[buffer_pos] = '\0';

                    display_break_output(input_buffer);
                }
            }
        }

        // Sleep to avoid busy-waiting
        Sleep(10);
#else
        // Unix input handling with select()
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);

        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000;

        int ready = select(STDIN_FILENO + 1, &readfds, NULL, NULL, &timeout);

        if(ready > 0 && FD_ISSET(STDIN_FILENO, &readfds)) {
            ssize_t bytes_read = read(STDIN_FILENO, &c, 1);

            if(bytes_read > 0) {
                if(c == 3 || c == 27) { // Ctrl+C or ESC
                    break;
                } else if(c == 127 || c == 8) { // DELETE or BACKSPACE
                    if(buffer_pos > 0) {
                        --buffer_pos;
                        input_buffer[buffer_pos] = '\0';
                        display_break_output(input_buffer);
                    }
                } else {
                    if(buffer_pos < sizeof(input_buffer) - 1) {
                        input_buffer[buffer_pos] = c;
                        ++buffer_pos;
                        input_buffer[buffer_pos] = '\0';

                        display_break_output(input_buffer);
                    }
                }
            }
        }
#endif
    }

    restore_mode(&term_state);
    clear_screen();

    return 0;
}
