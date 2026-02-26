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
    #include <io.h>
    #include "../utf16.h"
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
static void mjbsh_cleanup_terminal(void) {
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
static BOOL WINAPI mjbsh_console_ctrl_handler(DWORD ctrl_type) {
    switch(ctrl_type) {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
        case CTRL_CLOSE_EVENT:
        case CTRL_LOGOFF_EVENT:
        case CTRL_SHUTDOWN_EVENT:
            mjbsh_cleanup_terminal();

            return FALSE;  // Let default handler terminate
        default:
            return FALSE;
    }
}
#else
// POSIX signal handler
static void mjbsh_signal_handler(int signum) {
    mjbsh_cleanup_terminal();

    // Re-raise the signal with default handler
    signal(signum, SIG_DFL);
    raise(signum);
}
#endif

#ifdef _WIN32
static void mjbsh_set_raw_mode(terminal_state *term_state) {
    term_state->h_stdin = GetStdHandle(STD_INPUT_HANDLE);
    GetConsoleMode(term_state->h_stdin, &term_state->orig_mode);

    // Disable echo and line input
    DWORD mode = term_state->orig_mode;
    mode &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT);
    SetConsoleMode(term_state->h_stdin, mode);
}

static void mjbsh_restore_mode(terminal_state *term_state) {
    SetConsoleMode(term_state->h_stdin, term_state->orig_mode);
}
#else
static void mjbsh_set_raw_mode(terminal_state *term_state) {
    terminal_state raw = *term_state;
    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

static void mjbsh_restore_mode(terminal_state *term_state) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, term_state);
}
#endif

void mjbsh_clear_screen(void) {
    printf("\033[2J\033[H");
    fflush(stdout);
}

void mjbsh_screen_mode(mjbsh_screen_fn fn, mjbsh_key_fn key_fn) {
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
    SetConsoleCtrlHandler(mjbsh_console_ctrl_handler, TRUE);
#else
    signal(SIGINT, mjbsh_signal_handler);
    signal(SIGTERM, mjbsh_signal_handler);
    signal(SIGHUP, mjbsh_signal_handler);
#endif

    char input_buffer[1024] = {0};
    size_t buffer_pos = 0;
    if(fn != NULL) {
        fn("");
    }

    mjbsh_set_raw_mode(&term_state);

    in_raw_mode = true;
    mjbsh_show_cursor(false);

#ifdef _WIN32
    // UTF-16 decode state for surrogate pair tracking across key events
    uint8_t utf16_state = MJB_UTF_ACCEPT;
    mjb_codepoint utf16_cp = 0;
#endif

    while(1) {
#ifdef _WIN32
        INPUT_RECORD ir;
        DWORD events_read;
        DWORD wait_result = WaitForSingleObject(term_state.h_stdin, 10);

        if(
            wait_result == WAIT_OBJECT_0 &&
            ReadConsoleInput(term_state.h_stdin, &ir, 1, &events_read) &&
            events_read > 0 &&
            ir.EventType == KEY_EVENT &&
            ir.Event.KeyEvent.bKeyDown
        ) {
            WCHAR wc = ir.Event.KeyEvent.uChar.UnicodeChar;
            WORD vk = ir.Event.KeyEvent.wVirtualKeyCode;

            if(wc == 3) { // Ctrl+C
                break;
            } else if(wc == 0) { // Extended key (arrow keys, function keys, etc.)
                if(key_fn != NULL) {
                    switch(vk) {
                        case VK_UP:
                            key_fn(MJBSH_KEY_UP);
                            break;
                        case VK_DOWN:
                            key_fn(MJBSH_KEY_DOWN);
                            break;
                        case VK_RIGHT:
                            key_fn(MJBSH_KEY_RIGHT);
                            break;
                        case VK_LEFT:
                            key_fn(MJBSH_KEY_LEFT);
                            break;
                        default:
                            break;
                    }
                }
            } else if(wc == 8) { // BACKSPACE
                if(buffer_pos > 0) {
                    --buffer_pos;

                    // Walk back over UTF-8 continuation bytes
                    while(buffer_pos > 0 && ((unsigned char)input_buffer[buffer_pos] & 0xC0) == 0x80) {
                        --buffer_pos;
                    }

                    input_buffer[buffer_pos] = '\0';

                    if(fn != NULL) {
                        fn(input_buffer);
                    }
                }
            } else {
                // Decode UTF-16 unit to codepoint; handles surrogate pairs across events
                utf16_state = mjb_utf16_decode_step(utf16_state,
                    (uint8_t)(wc & 0xFF), (uint8_t)((wc >> 8) & 0xFF), &utf16_cp, false);

                if(utf16_state == MJB_UTF_ACCEPT) {
                    char utf8[5] = {0};
                    unsigned int utf8_len = mjb_codepoint_encode(utf16_cp, utf8, 5, MJB_ENCODING_UTF_8);

                    if(utf8_len > 0 && buffer_pos + utf8_len < sizeof(input_buffer) - 1) {
                        for(unsigned int i = 0; i < utf8_len; i++) {
                            input_buffer[buffer_pos++] = utf8[i];
                        }

                        input_buffer[buffer_pos] = '\0';

                        if(fn != NULL) {
                            fn(input_buffer);
                        }
                    }
                }
            }
        }
#else
        // Unix input handling with select()
        char c;
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
                if(c == 3) { // Ctrl+C
                    break;
                } else if(c == 27) { // Escape sequence (arrow keys, etc.)
                    fd_set seq_fds;
                    FD_ZERO(&seq_fds);
                    FD_SET(STDIN_FILENO, &seq_fds);

                    struct timeval seq_timeout;
                    seq_timeout.tv_sec = 0;
                    seq_timeout.tv_usec = 50000;

                    if(select(STDIN_FILENO + 1, &seq_fds, NULL, NULL, &seq_timeout) > 0) {
                        char seq[2];
                        ssize_t n = read(STDIN_FILENO, seq, 1);

                        if(n > 0 && seq[0] == '[') {
                            n = read(STDIN_FILENO, seq, 1);

                            if(n > 0 && key_fn != NULL) {
                                switch(seq[0]) {
                                    case 'A':
                                        key_fn(MJBSH_KEY_UP);
                                        break;
                                    case 'B':
                                        key_fn(MJBSH_KEY_DOWN);
                                        break;
                                    case 'C':
                                        key_fn(MJBSH_KEY_RIGHT);
                                        break;
                                    case 'D':
                                        key_fn(MJBSH_KEY_LEFT);
                                        break;
                                    default:
                                        break;
                                }
                            }
                        }
                    }
                } else if(c == 127 || c == 8) { // DELETE or BACKSPACE
                    if(buffer_pos > 0) {
                        --buffer_pos;
                        input_buffer[buffer_pos] = '\0';

                        if(fn != NULL) {
                            fn(input_buffer);
                        }
                    }
                } else {
                    if(buffer_pos < sizeof(input_buffer) - 1) {
                        input_buffer[buffer_pos] = c;
                        ++buffer_pos;
                        input_buffer[buffer_pos] = '\0';

                        if(fn != NULL) {
                            fn(input_buffer);
                        }
                    }
                }
            }
        }
#endif
    }

    // Show cursor and restore terminal mode
    in_raw_mode = false;

    mjbsh_show_cursor(true);
    mjbsh_restore_mode(&term_state);
    mjbsh_clear_screen();

    // Restore signal handlers
#ifdef _WIN32
    SetConsoleCtrlHandler(mjbsh_console_ctrl_handler, FALSE);
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
