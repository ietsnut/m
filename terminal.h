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

#define ANSI_ARROW_UP                 'A'
#define ANSI_ARROW_DOWN               'B'
#define ANSI_ARROW_RIGHT              'C'
#define ANSI_ARROW_LEFT               'D'

#define WRITE_ANSI(code) write(STDOUT_FILENO, code, strlen(code))

typedef struct terminal_t terminal_t;

struct terminal_t {

    int x, y, rows, cols;
    struct termios state;

    char *buffer;
    int buffer_length;

    void (*append_buffer)   (const char *data);
    void (*write_buffer)    (void);
    void (*free_buffer)     (void);

    void (*start)           (void);
    void (*stop)            (void);
    void (*die)             (const char *s);

    void (*title)           (const char *t);

    int  (*resize)          (void);

    char (*input)           (void);

    void (*cursor)          (int x, int y);

};

// METHODS

static void terminal_append_buffer(const char *data);
static void terminal_write_buffer(void);
static void terminal_free_buffer(void);

static void terminal_start(void);
static void terminal_stop(void);
static void terminal_die(const char *s);

static void terminal_title(const char *t);
static int  terminal_resize(void);
static char terminal_input();
static void terminal_cursor(int x, int y);

static terminal_t terminal = {
    .x = 0,
    .y = 0,
    .rows = 0,
    .cols = 0,
    .buffer = NULL,
    .buffer_length = 0,
    .append_buffer = terminal_append_buffer,
    .write_buffer = terminal_write_buffer,
    .free_buffer = terminal_free_buffer,
    .start = terminal_start,
    .stop = terminal_stop,
    .die = terminal_die,
    .title = terminal_title,
    .resize = terminal_resize,
    .input = terminal_input,
    .cursor = terminal_cursor
};

// IMPLEMENTATIONS

static void terminal_cursor(int x, int y) {
    char cbuf[32];
    snprintf(cbuf, sizeof(cbuf), ANSI_SET_CURSOR_POSITION, terminal.y + 2, terminal.x + 2);
    terminal.append_buffer(cbuf);
}

static char terminal_input(void) {
    char c;
    if (read(STDIN_FILENO, &c, 1) == 1) {
        if (c == '\x1b') {
            char seq[3];
            
            if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
            if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';

            if (seq[0] == '[') {
                switch (seq[1]) {
                    case ANSI_ARROW_UP:    return 'w';
                    case ANSI_ARROW_DOWN:  return 's';
                    case ANSI_ARROW_RIGHT: return 'd';
                    case ANSI_ARROW_LEFT:  return 'a';
                }
            }
            return '\x1b';
        } else {
            return c;
        }
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

static void terminal_append_buffer(const char *data) {
    int length = strlen(data);
    char *new = realloc(terminal.buffer, terminal.buffer_length + length);
    if (new == NULL) {
        return;
    }
    memcpy(&new[terminal.buffer_length], data, length);
    terminal.buffer= new;
    terminal.buffer_length += length;
}

static void terminal_write_buffer() {
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
    WRITE_ANSI(ANSI_CURSOR_HOME);
    WRITE_ANSI(ANSI_CLEAR_SCREEN);
    perror(s);
    exit(1);
}

static void terminal_stop(void) {
    terminal.free_buffer();
    WRITE_ANSI(ANSI_ALT_SCREEN_OFF);
    WRITE_ANSI(ANSI_CLEAR_SCROLLBACK);
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &terminal.state) == -1) {
        terminal.die("tcsetattr");
    }
}

static void terminal_start(void) {
    if (tcgetattr(STDIN_FILENO, &terminal.state) == -1) {
        terminal.die("tcgetattr");
    }
    atexit(terminal.stop);
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
    terminal.resize();
    terminal.title("[ MEDITOR ]");
    WRITE_ANSI(ANSI_ALT_SCREEN_ON);  // Use alternate screen buffer
    WRITE_ANSI(ANSI_CLEAR_SCROLLBACK);   // Clear scrollback buffer
    WRITE_ANSI(ANSI_RESET_SCROLL_REGION);       // Disable scrolling for entire screen
}

#endif // TERMINAL_H