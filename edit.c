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

    terminal.append(ANSI_HIDE_CURSOR);
    terminal.append(ANSI_CURSOR_HOME);

    int y;
    for (y = 0; y < terminal.rows; y++) {
        if (y == 0) {
            terminal.append(top_left);
            int content_width = terminal.cols - 3;
            int welcomelen = strlen("┤ MEDITOR ├") - 4;
            if (welcomelen > content_width) {
                welcomelen = content_width;
            }
            int padding_left = (content_width - welcomelen) / 2;
            int padding_right = content_width - welcomelen - padding_left;

            for (int i = 0; i < padding_left; i++) {
                terminal.append(horizontal);
            }

            if (welcomelen > 0) {
                terminal.append("┤ MEDITOR ├");
            }

            for (int i = 0; i < padding_right; i++) {
                terminal.append(horizontal);
            }
            terminal.append(top_right);
        } else if (y == terminal.rows - 1) {
            terminal.append(bottom_left);
            for (int i = 0; i < terminal.cols - 3; i++) {
                terminal.append(horizontal);
            }
            terminal.append(bottom_right);
        } else {
            terminal.append(vertical);
            for (int i = 0; i < terminal.cols - 3; i++) {
                terminal.append(" ");
            }
            terminal.append(vertical);
        }
        terminal.append(ANSI_ERASE_TO_EOL);
        if (y < terminal.rows - 1) {
            terminal.append("\r\n");
        }
    }

    //terminal.draw("POWERED BY MICROPENIS", (terminal.cols/2)-(strlen("POWERED BY MICROPENIS")/2), terminal.rows/2);
	terminal.write("POWERED BY MICROPENIS", ANSI_PINK, (terminal.cols/2)-(strlen("POWERED BY MICROPENIS")/2), terminal.rows/2);

    terminal.append(ANSI_SHOW_CURSOR);

    terminal.show();

}

void processKey(char c) {
    switch (c) {
        case '\x1b':
        	terminal.stop();
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
    borders();
}

static void handle_resize(void) {
    borders();
}

int main(void) {
    terminal.start();
    terminal.title("[ MEDITOR ]");
    terminal.listen(EVENT_RESIZE, handle_resize);
    terminal.listen(EVENT_INPUT, processKey);
    borders();
    while (1) {
        terminal.input();

    }
    return 0;
}