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
#include "resource.h"

#define VERSION "0.0.1"

const char *top_left      = "╭";
const char *top_right     = "╮";
const char *bottom_left   = "╰";
const char *bottom_right  = "╯";
const char *horizontal    = "─";
const char *vertical      = "│";

const char *ascii_image[] = {
    "      ┌───┐       ┌───┐      ",
    "      │ O └───────┘ O │      ",
    "      └─┐  ┌──U──┐  ┌─┘      ",
    "RESET  0│O─┤     ├─O│4  VCC  ",
    "XTAL1  1│O─┤ AVR ├─O│5  SCK  ",
    "XTAL2  2│O─┤ T85 ├─O│6  MISO ",
    "  GND  3│O─┤     ├─O│7  MOSI ",
    "      ┌─┘  └─────┘  └─┐      ",
    "      │ O ┌───────┐ O │      ",
    "      └───┘       └───┘      "
};

#define ASCII_IMAGE_HEIGHT (sizeof(ascii_image) / sizeof(ascii_image[0]))
#define ASCII_IMAGE_WIDTH 30

typedef enum State {
    DEFAULT     = 0,
    HELPING     = 1,
    EXTRACTING  = 2,
} State;

State state = DEFAULT;

void borders() {

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

    //terminal.draw("*", (terminal.cols/2)-(strlen("POWERED BY MICROPENIS")/2), terminal.rows/2);

}

void draw_ascii_image() {

    int left_padding = 4;
    int right_padding = 4;  // New: Added right padding
    int top_padding = 2;
    int box_width = ASCII_IMAGE_WIDTH + left_padding + right_padding + 1;  // Updated: Include right padding in box width
    int box_height = ASCII_IMAGE_HEIGHT + (2 * top_padding) + 2;
    int start_x = (terminal.cols - box_width) / 2;
    int start_y = (terminal.rows - box_height) / 2;

    // Draw top border
    terminal.draw(top_left, start_x, start_y);
    for (int i = 1; i < box_width - 1; i++) {
        terminal.draw(horizontal, start_x + i, start_y);
    }
    terminal.draw(top_right, start_x + box_width - 1, start_y);

    // Draw top padding
    char empty_line[box_width - 1];
    memset(empty_line, ' ', box_width - 2);
    empty_line[box_width - 2] = '\0';
    for (int i = 0; i < top_padding + 1; i++) {
        terminal.draw(vertical, start_x, start_y + i + 1);
        terminal.draw(empty_line, start_x + 1, start_y + i + 1);
        terminal.draw(vertical, start_x + box_width - 1, start_y + i + 1);
    }

    terminal.draw("╭─────╮", start_x + (box_width - 7) / 2, start_y - 1);
    terminal.draw("┤ MCU ├", start_x + (box_width - 7) / 2, start_y);
    terminal.draw("╰─────╯", start_x + (box_width - 7) / 2, start_y + 1);

    // Draw image and side borders
    for (int i = 0; i < ASCII_IMAGE_HEIGHT; i++) {
        terminal.draw(vertical, start_x, start_y + top_padding + i + 1);
        for (int j = 0; j < left_padding; j++) {
            terminal.draw(" ", start_x + 1 + j, start_y + top_padding + i + 1);
        }
        terminal.draw(ascii_image[i], start_x + 1 + left_padding, start_y + top_padding + i + 1);
        for (int j = 0; j < right_padding; j++) {  // New: Add right padding
            terminal.draw(" ", start_x + 1 + left_padding + ASCII_IMAGE_WIDTH + j, start_y + top_padding + i + 1);
        }
        terminal.draw(vertical, start_x + box_width - 1, start_y + top_padding + i + 1);
    }

    // Draw bottom padding
    for (int i = 0; i < top_padding; i++) {
        terminal.draw(vertical, start_x, start_y + top_padding + ASCII_IMAGE_HEIGHT + i + 1);
        terminal.draw(empty_line, start_x + 1, start_y + top_padding + ASCII_IMAGE_HEIGHT + i + 1);
        terminal.draw(vertical, start_x + box_width - 1, start_y + top_padding + ASCII_IMAGE_HEIGHT + i + 1);
    }

    // Draw bottom border
    terminal.draw(bottom_left, start_x, start_y + box_height - 1);
    for (int i = 1; i < box_width - 1; i++) {
        terminal.draw(horizontal, start_x + i, start_y + box_height - 1);
    }
    terminal.draw(bottom_right, start_x + box_width - 1, start_y + box_height - 1);
}


void refresh() {
    terminal.append(ANSI_HIDE_CURSOR);
    terminal.append(ANSI_CURSOR_HOME);
    switch(state) {
        case HELPING:
            borders();
            draw_ascii_image();
            break;
        case EXTRACTING:
            borders();
            terminal.write("*", ANSI_PINK, (terminal.cols/2)-(strlen("*")/2), terminal.rows/2);
            break;
        case DEFAULT:
            borders();
            break;
    }
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
        case 'h':
        case '?':
            if (state == HELPING) {
                state = DEFAULT;
            } else {
                state = HELPING;
            }
            refresh();
            break;
    }
    char cbuf[32];
    snprintf(cbuf, sizeof(cbuf), ANSI_SET_CURSOR_POSITION, terminal.y + 2, terminal.x + 2);
    WRITE_ANSI(cbuf);
}

static void handle_resize(void) {
    refresh();
}

int main(int argc, char *argv[]) {
    terminal.start();
    terminal.title("[ MEDITOR ]");
    terminal.listen(EVENT_RESIZE, handle_resize);
    terminal.listen(EVENT_INPUT, processKey);
    state = EXTRACTING;
    refresh();
    if (resource.exists() == 0) {
        if (resource.extract(argv[0]) != 0) {
            perror("extract");
            exit(1);
        }
    }
    state = DEFAULT;
    refresh();
    while (1) {
        terminal.input();
    }
    return 0;
}