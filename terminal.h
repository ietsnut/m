#ifndef TERMINAL_H
#define TERMINAL_H

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

/* ANSI Escape Codes */
#define ANSI_ESCAPE                   "\x1b"
#define ANSI_CSI                      "\x1b["
#define ANSI_ALT_SCREEN_ON            "\x1b[?1049h"
#define ANSI_ALT_SCREEN_OFF           "\x1b[?1049l"
#define ANSI_CLEAR_SCREEN             "\x1b[2J"
#define ANSI_CURSOR_HOME              "\x1b[H"
#define ANSI_CLEAR_SCROLLBACK         "\x1b[3J"
#define ANSI_RESET_SCROLL_REGION      "\x1b[r"
#define ANSI_HIDE_CURSOR              "\x1b[?25l"
#define ANSI_SHOW_CURSOR              "\x1b[?25h"
#define ANSI_ERASE_TO_EOL             "\x1b[K"
#define ANSI_SET_CURSOR_POSITION      "\x1b[%d;%dH"
#define ANSI_SET_TITLE                "\x1b]0;%s\x07"
#define ANSI_GET_CURSOR_POSITION      "\x1b[6n"
#define ANSI_CURSOR_TO_BOTTOM_RIGHT   "\x1b[999C\x1b[999B"

// New ANSI escape codes
#define ANSI_CURSOR_UP                "\x1b[%dA"
#define ANSI_CURSOR_DOWN              "\x1b[%dB"
#define ANSI_CURSOR_FORWARD           "\x1b[%dC"
#define ANSI_CURSOR_BACKWARD          "\x1b[%dD"
#define ANSI_CURSOR_NEXT_LINE         "\x1b[%dE"
#define ANSI_CURSOR_PREV_LINE         "\x1b[%dF"
#define ANSI_CURSOR_COLUMN            "\x1b[%dG"
#define ANSI_SAVE_CURSOR              "\x1b[s"
#define ANSI_RESTORE_CURSOR           "\x1b[u"
#define ANSI_CLEAR_FROM_CURSOR        "\x1b[0J"
#define ANSI_CLEAR_TO_CURSOR          "\x1b[1J"
#define ANSI_ERASE_FROM_CURSOR        "\x1b[0K"
#define ANSI_ERASE_TO_CURSOR          "\x1b[1K"
#define ANSI_BOLD                     "\x1b[1m"
#define ANSI_DIM                      "\x1b[2m"
#define ANSI_ITALIC                   "\x1b[3m"
#define ANSI_UNDERLINE                "\x1b[4m"
#define ANSI_BLINK                    "\x1b[5m"
#define ANSI_REVERSE                  "\x1b[7m"
#define ANSI_HIDDEN                   "\x1b[8m"
#define ANSI_STRIKETHROUGH            "\x1b[9m"
#define ANSI_RESET_ATTRIBUTES         "\x1b[0m"
#define ANSI_SCROLL_UP                "\x1b[%dS"
#define ANSI_SCROLL_DOWN              "\x1b[%dT"

#define ANSI_ARROW_UP                 'A'
#define ANSI_ARROW_DOWN               'B'
#define ANSI_ARROW_RIGHT              'C'
#define ANSI_ARROW_LEFT               'D'

#define ANSI_PINK                      "\x1b[95m"

#define WRITE_ANSI(code) write(STDOUT_FILENO, code, strlen(code))

typedef struct terminal_t terminal_t;

typedef enum {
    EVENT_RESIZE,
    EVENT_INPUT,
    EVENT_COUNT 
} terminal_event_t;

struct terminal_t {

    int x, y, rows, cols;
    struct termios state;

    char *buffer;
    int buffer_length;

    void (*append)          (const char *data);
    void (*show)            (void);
    void (*free_buffer)     (void);
    void (*start)           (void);
    void (*stop)            (void);
    void (*die)             (const char *s);
    void (*title)           (const char *t);
    char (*input)           (void);
    void (*cursor)          (int x, int y);
    void (*draw)            (const char *str, int x, int y);
    void (*listen)          (terminal_event_t event, void *handler);
    void (*write)           (const char *str, const char *format_start, int x, int y);

};

// METHODS

static void terminal_append(const char *data);
static void terminal_show(void);
static void terminal_free_buffer(void);
static void terminal_start(void);
static void terminal_stop(void);
static void terminal_die(const char *s);
static void terminal_title(const char *t);
static int  terminal_resize(void);
static char terminal_input();
static void terminal_cursor(int x, int y);
static void terminal_draw(const char *str, int x, int y);
static void terminal_listen(terminal_event_t event, void *handler);
static void terminal_write(const char *str, const char *format_start, int x, int y);

static void (*event_handlers[EVENT_COUNT])(void) = {NULL};
static void (*input_handler)(char) = NULL;

static terminal_t terminal = {
    .x = 0,
    .y = 0,
    .rows = 0,
    .cols = 0,
    .buffer = NULL,
    .buffer_length = 0,
    .append = terminal_append,
    .show = terminal_show,
    .free_buffer = terminal_free_buffer,
    .start = terminal_start,
    .stop = terminal_stop,
    .die = terminal_die,
    .title = terminal_title,
    .input = terminal_input,
    .cursor = terminal_cursor,
    .draw = terminal_draw,
    .listen = terminal_listen,
    .write = terminal_write
};

// IMPLEMENTATIONS

static void signal_handler(int sig) {
    if (sig == SIGWINCH && event_handlers[EVENT_RESIZE]) {
        terminal_resize();
        if (terminal.y >= terminal.rows) terminal.y = terminal.rows - 1;
        if (terminal.x >= terminal.cols) terminal.x = terminal.cols - 1;
        event_handlers[EVENT_RESIZE]();
    }
}

static void terminal_listen(terminal_event_t event, void *handler) {
    if (event < EVENT_COUNT) {
        switch (event) {
            case EVENT_RESIZE:
                event_handlers[EVENT_RESIZE] = (void (*)(void))handler;
                struct sigaction sa;
                sa.sa_handler = signal_handler;
                sigemptyset(&sa.sa_mask);
                sa.sa_flags = SA_RESTART;
                if (sigaction(SIGWINCH, &sa, NULL) == -1) {
                    terminal.die("sigaction for SIGWINCH");
                }
                break;
            case EVENT_INPUT:
                input_handler = (void (*)(char))handler;
                break;
            default:
                // No action for unknown event types
                break;
        }
    }
}

static void terminal_write(const char *str, const char *format_start, int x, int y) {
    char formatted_str[1024]; // Adjust size as needed
    snprintf(formatted_str, sizeof(formatted_str), "%s%s%s", format_start, str, ANSI_RESET_ATTRIBUTES);
    terminal_draw(formatted_str, x, y);
}

static void terminal_cursor(int x, int y) {
    char cbuf[32];
    snprintf(cbuf, sizeof(cbuf), ANSI_SET_CURSOR_POSITION, y + 1, x + 1);
    terminal.append(cbuf);
}

static void terminal_draw(const char *str, int x, int y) {
    terminal.cursor(x, y);
    terminal.append(str);
    char buf[32];
    snprintf(buf, sizeof(buf), ANSI_CURSOR_BACKWARD, strlen(str));
    terminal.append(buf);
}

static char terminal_input(void) {
    char c;
    if (read(STDIN_FILENO, &c, 1) == 1) {
        if (c == '\x1b') {
            char seq[3];
            
            if (read(STDIN_FILENO, &seq[0], 1) != 1) {
                if (input_handler) {
                    input_handler('\x1b');
                }
                return '\x1b';
            }
            if (read(STDIN_FILENO, &seq[1], 1) != 1) {
                if (input_handler) {
                    input_handler('\x1b');
                }
                return '\x1b';
            }

            if (seq[0] == '[') {
                switch (seq[1]) {
                    case ANSI_ARROW_UP:    c = 'w'; break;
                    case ANSI_ARROW_DOWN:  c = 's'; break;
                    case ANSI_ARROW_RIGHT: c = 'd'; break;
                    case ANSI_ARROW_LEFT:  c = 'a'; break;
                    default: 
                        if (input_handler) {
                            input_handler('\x1b');
                        }
                        return '\x1b';
                }
            } else {
                if (input_handler) {
                    input_handler('\x1b');
                }
                return '\x1b';
            }
        }
        
        if (input_handler) {
            input_handler(c);
        }
        return c;
    }
    return 0;
}

static void terminal_title(const char *t) {
    char buf[512];
    snprintf(buf, sizeof(buf), ANSI_SET_TITLE, t);
    write(STDOUT_FILENO, buf, strlen(buf));
}

static int terminal_resize(void) {
    struct winsize ws;
    
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        if (write(STDOUT_FILENO, ANSI_CURSOR_TO_BOTTOM_RIGHT, strlen(ANSI_CURSOR_TO_BOTTOM_RIGHT)) != strlen(ANSI_CURSOR_TO_BOTTOM_RIGHT)) {
            terminal.die("resize");
            return -1;
        }
        
        char buf[32];
        unsigned int i = 0;
        if (write(STDOUT_FILENO, ANSI_GET_CURSOR_POSITION, strlen(ANSI_GET_CURSOR_POSITION)) != strlen(ANSI_GET_CURSOR_POSITION)) {
            terminal.die("resize");
            return -1;
        }
        while (i < sizeof(buf) - 1) {
            if (read(STDIN_FILENO, &buf[i], 1) != 1) {
                break;
            }
            if (buf[i] == 'R') { 
                break;
            }
            i++;
        }
        buf[i] = '\0';
        
        if (buf[0] != '\x1b' || buf[1] != '[') {
            terminal.die("resize");
            return -1;
        }
        if (sscanf(&buf[2], "%d;%d", &terminal.rows, &terminal.cols) != 2) {
            terminal.die("resize");
            return -1;
        }
    } else {
        terminal.cols = ws.ws_col;
        terminal.rows = ws.ws_row;
    }
    
    return 0;
}

static void terminal_append(const char *data) {
    int length = strlen(data);
    char *new = realloc(terminal.buffer, terminal.buffer_length + length);
    if (new == NULL) {
        return;
    }
    memcpy(&new[terminal.buffer_length], data, length);
    terminal.buffer= new;
    terminal.buffer_length += length;
}

static void terminal_show() {
    terminal.cursor(terminal.x + 1, terminal.y + 1);
    write(STDOUT_FILENO, terminal.buffer, terminal.buffer_length);
    terminal.buffer_length = 0;
}

static void terminal_free_buffer(void) {
    free(terminal.buffer);
    terminal.buffer = NULL;
    terminal.buffer_length = 0;
}

static void terminal_die(const char *s) {
    terminal.free_buffer();
    WRITE_ANSI(ANSI_ALT_SCREEN_OFF);
    WRITE_ANSI(ANSI_CLEAR_SCREEN);
    WRITE_ANSI(ANSI_CURSOR_HOME);
    perror(s);
    exit(1);
}

static void terminal_cleanup(void) {
    terminal.free_buffer();
    WRITE_ANSI(ANSI_ALT_SCREEN_OFF);
    WRITE_ANSI(ANSI_CLEAR_SCROLLBACK);
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &terminal.state) == -1) {
        terminal.die("tcsetattr");
    }
}

static void terminal_stop(void) {
    WRITE_ANSI(ANSI_CLEAR_SCREEN);
    WRITE_ANSI(ANSI_CURSOR_HOME);
    terminal.free_buffer();
    WRITE_ANSI(ANSI_ALT_SCREEN_OFF);
    WRITE_ANSI(ANSI_CLEAR_SCROLLBACK);
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &terminal.state) == -1) {
        terminal.die("tcsetattr");
    }
    exit(0);
}

static void terminal_start(void) {
    if (tcgetattr(STDIN_FILENO, &terminal.state) == -1) {
        terminal.die("tcgetattr");
    }
    atexit(terminal_cleanup);
    struct termios raw = terminal.state;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);   // TURN OFF CTRL-S/Q/M
    raw.c_oflag &= ~(OPOST);                                    // TURN OFF OUTPUT PROCESSING
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);            // TURN OFF ECHO, CANONICAL MODE, CTRL-C/Z/V
    raw.c_cc[VMIN]  = 0;
    raw.c_cc[VTIME] = 1;
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        terminal.die("tcsetattr");
    }
    terminal.x = 0;
    terminal.y = 0;
    terminal_resize();

    WRITE_ANSI(ANSI_ALT_SCREEN_ON);  // Use alternate screen buffer
    WRITE_ANSI(ANSI_CLEAR_SCROLLBACK);   // Clear scrollback buffer
    WRITE_ANSI(ANSI_RESET_SCROLL_REGION);       // Disable scrolling for entire screen
}

#endif // TERMINAL_H