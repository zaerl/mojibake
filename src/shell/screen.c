/**
 * The Mojibake shell
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>

#ifndef _WIN32
    #include <sys/select.h>
#endif

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <conio.h>
    #include <io.h>
    #define STDIN_FILENO _fileno(stdin)

    // Windows terminal state structure
    typedef struct mjb_terminal_state {
        HANDLE h_stdin;
        DWORD orig_mode;
    } terminal_state;
#else
    #include <termios.h>
    #include <unistd.h>

    // Unix terminal state structure
    typedef struct termios terminal_state;
#endif

#include "screen.h"
#include "shell.h"

#ifdef _WIN32
void set_raw_mode(terminal_state *term_state) {
    term_state->h_stdin = GetStdHandle(STD_INPUT_HANDLE);
    GetConsoleMode(term_state->h_stdin, &term_state->orig_mode);

    // Disable echo and line input
    DWORD mode = term_state->orig_mode;
    mode &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT);
    SetConsoleMode(term_state->h_stdin, mode);
}

void restore_mode(terminal_state *term_state) {
    SetConsoleMode(term_state->h_stdin, term_state->orig_mode);
}
#else
void set_raw_mode(terminal_state *term_state) {
    terminal_state raw = *term_state;
    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void restore_mode(terminal_state *term_state) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, term_state);
}
#endif

void clear_screen(void) {
    printf("\033[2J\033[H");
    fflush(stdout);
}

void screen_mode(screen_fn fn) {
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

    fn("");
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
                    fn(input_buffer);
                }
            } else {
                if(buffer_pos < sizeof(input_buffer) - 1) {
                    input_buffer[buffer_pos] = c;
                    ++buffer_pos;
                    input_buffer[buffer_pos] = '\0';

                    fn(input_buffer);
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
                        fn(input_buffer);
                    }
                } else {
                    if(buffer_pos < sizeof(input_buffer) - 1) {
                        input_buffer[buffer_pos] = c;
                        ++buffer_pos;
                        input_buffer[buffer_pos] = '\0';

                        fn(input_buffer);
                    }
                }
            }
        }
#endif
    }

    restore_mode(&term_state);
    clear_screen();
}

void table_top(void) {
    printf("┌");

    for(unsigned int i = 0; i < cmd_width; i++) {
        printf("─");
    }

    printf("┐\n");
}

void table_bottom(void) {
    printf("└");

    for(unsigned int i = 0; i < cmd_width; i++) {
        printf("─");
    }

    printf("┘\n");
}
