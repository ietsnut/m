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

#define ARROW_UP                      '\x41'
#define ARROW_DOWN                    '\x42'
#define ARROW_RIGHT                   '\x43'
#define ARROW_LEFT                    '\x44'
#define DELETE_KEY                    '\x7E'

const char *top_left      = "┌";
const char *top_right     = "┐";
const char *bottom_left   = "└";
const char *bottom_right  = "┘";
const char *horizontal    = "─";
const char *vertical      = "│";

const char *all_join      = "┼";
const char *top_join      = "┴";
const char *bottom_join   = "┬";
const char *right_join    = "├";
const char *left_join     = "┤";

const char *square_fill   = "█";

typedef struct terminal_t terminal_t;

typedef enum {
    RESIZE,
    EVENT_COUNT 
} terminal_event_t;

struct terminal_t {

    int x, y, rows, cols;
    struct termios state;

    char *buffer;
    int buffer_length;

    void (*append)          (const char *data);
    void (*free_buffer)     (void);
    void (*open)            (void);
    void (*close)           (void);
    void (*die)             (const char *s);
    void (*title)           (const char *t);
    char (*input)           (void);
    void (*cursor)          (int x, int y);
    void (*draw)            (void);
    void (*listen)          (terminal_event_t event, void *handler);
    void (*write)           (const char *str, int x, int y);
    void (*box)             (int x, int y, int width, int height);
    void (*clear)           (void);
    void (*setting)         (const char *op);

};

// METHODS

static void terminal_append(const char *data);
static void terminal_draw(void);
static void terminal_free_buffer(void);
static void terminal_open(void);
static void terminal_close(void);
static void terminal_die(const char *s);
static void terminal_title(const char *t);
static int  terminal_resize(void);
static char terminal_input();
static void terminal_cursor(int x, int y);
static void terminal_listen(terminal_event_t event, void *handler);
static void terminal_box(int x, int y, int width, int height);
static void terminal_clear(void);
static void terminal_write(const char *str, int x, int y);
static void terminal_setting(const char *op);

static void (*event_handlers[EVENT_COUNT])(void) = {NULL};

static terminal_t terminal = {
    .x = 0,
    .y = 0,
    .rows = 0,
    .cols = 0,
    .buffer = NULL,
    .buffer_length = 0,
    .append = terminal_append,
    .draw = terminal_draw,
    .free_buffer = terminal_free_buffer,
    .open = terminal_open,
    .close = terminal_close,
    .die = terminal_die,
    .title = terminal_title,
    .input = terminal_input,
    .cursor = terminal_cursor,
    .listen = terminal_listen,
    .write = terminal_write,
    .box = terminal_box,
    .clear = terminal_clear,
    .setting = terminal_setting
};

// IMPLEMENTATIONS

static void terminal_box(int x, int y, int width, int height) {
    terminal.write(top_left, x, y);
    for (int i = 1; i < width - 1; i++) {
        terminal.write(horizontal, x + i, y);
    }
    terminal.write(top_right, x + width - 1, y);
    for (int i = 1; i < height - 1; i++) {
        terminal.write(vertical, x, y + i);
        terminal.write(vertical, x + width - 1, y + i);
    }
    terminal.write(bottom_left, x, y + height - 1);
    for (int i = 1; i < width - 1; i++) {
        terminal.write(horizontal, x + i, y + height - 1);
    }
    terminal.write(bottom_right, x + width - 1, y + height - 1);
}

static void signal_handler(int sig) {
    if (sig == SIGWINCH && event_handlers[RESIZE]) {
        terminal_resize();
        if (terminal.y >= terminal.rows) terminal.y = terminal.rows - 1;
        if (terminal.x >= terminal.cols) terminal.x = terminal.cols - 1;
        event_handlers[RESIZE]();
    }
}

static void terminal_listen(terminal_event_t event, void *handler) {
    if (event < EVENT_COUNT) {
        switch (event) {
            case RESIZE:
                event_handlers[RESIZE] = (void (*)(void))handler;
                struct sigaction sa;
                sa.sa_handler = signal_handler;
                sigemptyset(&sa.sa_mask);
                sa.sa_flags = SA_RESTART;
                if (sigaction(SIGWINCH, &sa, NULL) == -1) {
                    terminal.die("sigaction for SIGWINCH");
                }
                break;
            default:
                // No action for unknown event types
                break;
        }
    }
}

static void terminal_setting(const char *op) {
    write(STDOUT_FILENO, op, strlen(op));
}

static void terminal_write(const char *str, int x, int y) {
    terminal.cursor(x, y);
    terminal.append(str);
}

static void terminal_cursor(int x, int y) {
    char cbuf[32];
    snprintf(cbuf, sizeof(cbuf), ANSI_SET_CURSOR_POSITION, y + 1, x + 1);
    terminal.append(cbuf);
}

static char terminal_input(void) {
    char c;
    while (read(STDIN_FILENO, &c, 1) != 1) {
        // Keep waiting for input
    }
    if (c == '\x1b') {
        char seq[3];
        
        if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';

        if (seq[0] == '[') {
            switch (seq[1]) {
                case 'A': return ARROW_UP;
                case 'B': return ARROW_DOWN;
                case 'C': return ARROW_RIGHT;
                case 'D': return ARROW_LEFT;
                case '3': 
                    if (read(STDIN_FILENO, &seq[2], 1) == 1 && seq[2] == '~')
                        return DELETE_KEY;
            }
        }
        return '\x1b';
    }
    return c;
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
    terminal.buffer = new;
    terminal.buffer_length += length;
}

static void terminal_clear(void) {
    terminal.append(ANSI_HIDE_CURSOR);
    terminal.buffer_length = 0;
    terminal.append(ANSI_CLEAR_SCREEN);    // Clear entire screen
    terminal.append(ANSI_CURSOR_HOME);     // Reset cursor to home position
}

static void terminal_draw(void) {
    //terminal.cursor(terminal.x + 1, terminal.y + 1);
    write(STDOUT_FILENO, terminal.buffer, terminal.buffer_length);
    terminal.append(ANSI_SHOW_CURSOR);
    terminal.buffer_length = 0;
}

static void terminal_free_buffer(void) {
    free(terminal.buffer);
    terminal.buffer = NULL;
    terminal.buffer_length = 0;
}

static void terminal_die(const char *s) {
    terminal.free_buffer();
    terminal.setting(ANSI_ALT_SCREEN_OFF);
    //terminal.setting(ANSI_CLEAR_SCREEN);
    terminal.setting(ANSI_CURSOR_HOME);
    perror(s);
    exit(1);
}

static void terminal_cleanup(void) {
    terminal.free_buffer();
    terminal.setting(ANSI_ALT_SCREEN_OFF);
    terminal.setting(ANSI_CLEAR_SCROLLBACK);
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &terminal.state) == -1) {
        terminal.die("tcsetattr");
    }
}

static void terminal_close(void) {
    terminal.setting(ANSI_ALT_SCREEN_OFF);
    terminal.setting(ANSI_CLEAR_SCROLLBACK);
    //terminal.setting(ANSI_CLEAR_SCREEN);
    terminal.setting(ANSI_CURSOR_HOME);
    terminal.free_buffer();
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &terminal.state) == -1) {
        terminal.die("tcsetattr");
    }
}

static void terminal_open(void) {
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

    terminal.setting(ANSI_ALT_SCREEN_ON);  // Use alternate screen buffer
    terminal.setting(ANSI_CLEAR_SCROLLBACK);   // Clear scrollback buffer
    terminal.setting(ANSI_RESET_SCROLL_REGION);       // Disable scrolling for entire screen
}

#endif // TERMINAL_H