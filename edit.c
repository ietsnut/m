#define _POSIX_C_SOURCE 200809L

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

#include "terminal.h"

#define VERSION "0.0.1"

const char *top_left      = "╭";
const char *top_right     = "╮";
const char *bottom_left   = "╰";
const char *bottom_right  = "╯";
const char *horizontal    = "─";
const char *vertical      = "│";

void borders() {

    terminal.append_buffer(ANSI_HIDE_CURSOR);
    terminal.append_buffer(ANSI_CURSOR_HOME);

    int y;
    for (y = 0; y < terminal.rows; y++) {
        if (y == 0) {
            terminal.append_buffer(top_left);
            int content_width = terminal.cols - 3;
            int welcomelen = strlen("┤ MEDITOR ├") - 4;
            if (welcomelen > content_width) {
                welcomelen = content_width;
            }
            int padding_left = (content_width - welcomelen) / 2;
            int padding_right = content_width - welcomelen - padding_left;

            for (int i = 0; i < padding_left; i++) {
                terminal.append_buffer(horizontal);
            }

            if (welcomelen > 0) {
                terminal.append_buffer("┤ MEDITOR ├");
            }

            for (int i = 0; i < padding_right; i++) {
                terminal.append_buffer(horizontal);
            }
            terminal.append_buffer(top_right);
        } else if (y == terminal.rows - 1) {
            terminal.append_buffer(bottom_left);
            for (int i = 0; i < terminal.cols - 3; i++) {
                terminal.append_buffer(horizontal);
            }
            terminal.append_buffer(bottom_right);
        } else {
            terminal.append_buffer(vertical);
            for (int i = 0; i < terminal.cols - 3; i++) {
                terminal.append_buffer(" ");
            }
            terminal.append_buffer(vertical);
        }
        terminal.append_buffer(ANSI_ERASE_TO_EOL);
        if (y < terminal.rows - 1) {
            terminal.append_buffer("\r\n");
        }
    }

    terminal.cursor(terminal.y + 2, terminal.x + 2);

    terminal.append_buffer(ANSI_SHOW_CURSOR);

    terminal.write_buffer();

}


void processKey(char c) {
    switch (c) {
        case '\x1b':
            WRITE_ANSI(ANSI_CLEAR_SCREEN);
            WRITE_ANSI(ANSI_CURSOR_HOME);
            exit(0);
            break;
        case 'a':
            if (terminal.x > 0) {
                terminal.x--;
            }
            break;
        case 'd':
            if (terminal.x < terminal.cols - 3) {
                terminal.x++;
            }
            break;
        case 'w':
            if (terminal.y > 0) {
                terminal.y--;
            }
            break;
        case 's':
            if (terminal.y < terminal.rows - 3) {
                terminal.y++;
            }
            break;
    }
}

volatile sig_atomic_t resize = 0;

static void sigwinchHandler(int sig) {
    resize = 1;
}

static void listen(void) {
    struct sigaction sa;
    sa.sa_handler = sigwinchHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGWINCH, &sa, NULL) == -1) {
        terminal.die("sigaction");
    }
}

int main(void) {

    terminal.start();
    listen();
    borders();

    fd_set readfds;
    struct timeval tv;

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);

        tv.tv_sec = 0;
        tv.tv_usec = 100000; // 100ms timeout

        int ret = select(STDIN_FILENO + 1, &readfds, NULL, NULL, &tv);

        if (ret == -1 && errno != EINTR) {
            terminal.die("select");
        }

        if (resize) {
            terminal.resize();
            if (terminal.y >= terminal.rows) terminal.y = terminal.rows - 1;
            if (terminal.x >= terminal.cols) terminal.x = terminal.cols - 1;
            resize = 0;
            borders();
        }

        if (ret > 0 && FD_ISSET(STDIN_FILENO, &readfds)) {
            char c = terminal.input();
            if (c != 0) {
                processKey(c);
                borders();
            }
        }
    }

    return 0;
}