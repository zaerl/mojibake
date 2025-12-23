/**
 * The Mojibake shell
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <stdbool.h>

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
    #include <signal.h>
    #include <sys/select.h>
    #include <termios.h>
    #include <unistd.h>

    // Unix terminal state structure
    typedef struct termios terminal_state;
#endif

#include "screen.h"
#include "shell.h"

// Global state for signal handling
static bool in_raw_mode = false;
static terminal_state *saved_term_state = NULL;

// Cleanup function to restore terminal and show cursor
static void cleanup_terminal(void) {
    if(in_raw_mode && saved_term_state != NULL) {
        mjbsh_show_cursor(true);
        fflush(stdout);

#ifdef _WIN32
        SetConsoleMode(saved_term_state->h_stdin, saved_term_state->orig_mode);
#else
        tcsetattr(STDIN_FILENO, TCSAFLUSH, saved_term_state);
#endif

        in_raw_mode = false;
        saved_term_state = NULL;
    }
}

#ifdef _WIN32
// Windows console control handler
static BOOL WINAPI console_ctrl_handler(DWORD ctrl_type) {
    switch(ctrl_type) {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
        case CTRL_CLOSE_EVENT:
        case CTRL_LOGOFF_EVENT:
        case CTRL_SHUTDOWN_EVENT:
            cleanup_terminal();

            return FALSE;  // Let default handler terminate
        default:
            return FALSE;
    }
}
#else
// POSIX signal handler
static void signal_handler(int signum) {
    cleanup_terminal();

    // Re-raise the signal with default handler
    signal(signum, SIG_DFL);
    raise(signum);
}
#endif

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

void mjbsh_clear_screen(void) {
    printf("\033[2J\033[H");
    fflush(stdout);
}

void mjbsh_screen_mode(mjbsh_screen_fn fn) {
    terminal_state term_state;

#ifdef _WIN32
    // Windows-specific initialization
    term_state.h_stdin = GetStdHandle(STD_INPUT_HANDLE);
    GetConsoleMode(term_state.h_stdin, &term_state.orig_mode);
#else
    // POSIX-specific initialization
    tcgetattr(STDIN_FILENO, &term_state);
#endif

    // Set up signal handling
    saved_term_state = &term_state;

#ifdef _WIN32
    SetConsoleCtrlHandler(console_ctrl_handler, TRUE);
#else
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGHUP, signal_handler);
#endif

    char input_buffer[1024] = {0};
    size_t buffer_pos = 0;
    char c;

    fn("");
    set_raw_mode(&term_state);

    in_raw_mode = true;
    mjbsh_show_cursor(false);

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

    // Show cursor and restore terminal mode
    in_raw_mode = false;

    mjbsh_show_cursor(true);
    restore_mode(&term_state);
    mjbsh_clear_screen();

    // Restore signal handlers
#ifdef _WIN32
    SetConsoleCtrlHandler(console_ctrl_handler, FALSE);
#else
    signal(SIGINT, SIG_DFL);
    signal(SIGTERM, SIG_DFL);
    signal(SIGHUP, SIG_DFL);
#endif

    saved_term_state = NULL;
}

void mjbsh_table_top(void) {
    printf("┌");

    for(unsigned int i = 0; i < cmd_width; i++) {
        printf("─");
    }

    printf("┐\n");
}

void mjbsh_table_bottom(void) {
    printf("└");

    for(unsigned int i = 0; i < cmd_width; i++) {
        printf("─");
    }

    printf("┘\n");
}
