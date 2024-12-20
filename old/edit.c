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

//#include "terminal.h"

#define VERSION "0.0.1"

/* ANSI Escape Codes */
#define ANSI_ESCAPE                   "\x1b"
#define ANSI_CSI                      "\x1b["

#define ANSI_ALT_SCREEN_ON            "\x1b[?1049h"     	// Switch to the alternate screen buffer
#define ANSI_ALT_SCREEN_OFF           "\x1b[?1049l"     	// Return to the normal screen buffer
#define ANSI_CLEAR_SCREEN             "\x1b[2J"         	// Clear the entire screen
#define ANSI_CURSOR_HOME              "\x1b[H"          	// Move cursor to the home position (top-left)
#define ANSI_CLEAR_SCROLLBACK         "\x1b[3J"         	// Clear the scrollback buffer
#define ANSI_RESET_SCROLL_REGION      "\x1b[r"          	// Reset scrolling region
#define ANSI_HIDE_CURSOR              "\x1b[?25l"       	// Hide the cursor
#define ANSI_SHOW_CURSOR              "\x1b[?25h"       	// Show the cursor
#define ANSI_ERASE_TO_EOL             "\x1b[K"          	// Erase from cursor to end of line
#define ANSI_SET_CURSOR_POSITION      "\x1b[%d;%dH"     	// Set cursor position (row;column)
#define ANSI_SET_TITLE                "\x1b]0;%s\x07"   	// Set terminal window title
#define ANSI_GET_CURSOR_POSITION      "\x1b[6n"         	// Request cursor position report
#define ANSI_CURSOR_TO_BOTTOM_RIGHT   "\x1b[999C\x1b[999B" 	// Move cursor to bottom-right corner

#define ANSI_ARROW_UP                 'A'
#define ANSI_ARROW_DOWN               'B'
#define ANSI_ARROW_RIGHT              'C'
#define ANSI_ARROW_LEFT               'D'

/* Macro for writing ANSI codes to STDOUT */
#define WRITE_ANSI(code) write(STDOUT_FILENO, code, strlen(code))

struct Terminal {
	int x, y;
	int rows, cols;
	struct termios state;
};

struct TerminalBuffer {
  	char *data;
  	int length;
};

static struct Terminal 			terminal;
static struct TerminalBuffer 	terminal_buffer = {NULL, 0};

void appendBuffer(const char *data, int length) {
    char *new = realloc(terminal_buffer.data, terminal_buffer.length + length);
    if (new == NULL) {
        return;
    }
    memcpy(&new[terminal_buffer.length], data, length);
    terminal_buffer.data = new;
    terminal_buffer.length += length;
}

void writeBuffer() {
    write(STDOUT_FILENO, terminal_buffer.data, terminal_buffer.length);
    terminal_buffer.length = 0;
}

void freeBuffer(void) {
    free(terminal_buffer.data);
    terminal_buffer.data = NULL;
    terminal_buffer.length = 0;
}

void die(const char *s) {
	freeBuffer();
	WRITE_ANSI(ANSI_ALT_SCREEN_OFF);
	WRITE_ANSI(ANSI_CURSOR_HOME);
	WRITE_ANSI(ANSI_CLEAR_SCREEN);
	perror(s);
	exit(1);
}

// get cursor position
int getCursor(int *rows, int *cols) {
	char buf[32];
  	unsigned int i = 0;
  	if (write(STDOUT_FILENO, ANSI_GET_CURSOR_POSITION, strlen(ANSI_GET_CURSOR_POSITION)) != strlen(ANSI_GET_CURSOR_POSITION)) {
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
		return -1;
	}
  	if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) {
  		return -1;
  	}
  	return 0;
}

// get window size
int getSize(int *rows, int *cols) {
	struct winsize ws;
  	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
    	if (write(STDOUT_FILENO, ANSI_CURSOR_TO_BOTTOM_RIGHT, strlen(ANSI_CURSOR_TO_BOTTOM_RIGHT)) != strlen(ANSI_CURSOR_TO_BOTTOM_RIGHT)) {
    		return -1;
    	}
		return getCursor(rows, cols);
  	} else {
    	*cols = ws.ws_col;
    	*rows = ws.ws_row;
    	return 0;
  	}
}

void stop(void) {
	freeBuffer();
	WRITE_ANSI(ANSI_ALT_SCREEN_OFF);
	WRITE_ANSI(ANSI_CLEAR_SCROLLBACK);
  	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &terminal.state) == -1) {
  		die("tcsetattr");
  	}
}

void setTerminalTitle(const char *title) {
    char buf[512];
    snprintf(buf, sizeof(buf), ANSI_SET_TITLE, title);
    write(STDOUT_FILENO, buf, strlen(buf));
}

void start(void) {
	if (tcgetattr(STDIN_FILENO, &terminal.state) == -1) {
		die("tcgetattr");
	}
	atexit(stop);
	struct termios raw = terminal.state;
	raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);	// TURN OFF CTRL-S/Q/M
	raw.c_oflag &= ~(OPOST);									// TURN OFF OUTPUT PROCESSING
	raw.c_cflag |= (CS8);
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);			// TURN OFF ECHO, CANONICAL MODE, CTRL-C/Z/V
	raw.c_cc[VMIN] 	= 0;
	raw.c_cc[VTIME] = 1;
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
		die("tcsetattr");
	}
	terminal.x = 0;
  	terminal.y = 0;
	if (getSize(&terminal.rows, &terminal.cols) == -1) {
		die("getSize");
	}
    setTerminalTitle("-[ MEDITOR ]-");
    WRITE_ANSI(ANSI_ALT_SCREEN_ON);  // Use alternate screen buffer
    WRITE_ANSI(ANSI_CLEAR_SCROLLBACK);	 // Clear scrollback buffer
    WRITE_ANSI(ANSI_RESET_SCROLL_REGION);       // Disable scrolling for entire screen
}

const char *top_left 		= "╭";
const char *top_right 		= "╮";
const char *bottom_left 	= "╰";
const char *bottom_right 	= "╯";
const char *horizontal 		= "─";
const char *vertical 		= "│";

void draw() {

	appendBuffer(ANSI_HIDE_CURSOR, strlen(ANSI_HIDE_CURSOR));  		// Hide cursor
	appendBuffer(ANSI_CURSOR_HOME, strlen(ANSI_CURSOR_HOME));     	// Move cursor to top-left

    int y;
    for (y = 0; y < terminal.rows; y++) {
        if (y == 0) {
            // Top border with corners
            appendBuffer(top_left, strlen(top_left));
            int content_width = terminal.cols - 3;  // Exclude corners
            int welcomelen = strlen("[ MEDITOR ]");
            if (welcomelen > content_width) {
                welcomelen = content_width;
            }
            int padding_left = (content_width - welcomelen) / 2;
            int padding_right = content_width - welcomelen - padding_left;

            for (int i = 0; i < padding_left; i++) {
                appendBuffer(horizontal, strlen(horizontal));
            }

            if (welcomelen > 0) {
                appendBuffer("[ MEDITOR ]", strlen("[ MEDITOR ]"));
            }

            for (int i = 0; i < padding_right; i++) {
                appendBuffer(horizontal, strlen(horizontal));
            }
            appendBuffer(top_right, strlen(top_right));
        } else if (y == terminal.rows - 1) {
            // Bottom border with corners
            appendBuffer(bottom_left, strlen(bottom_left));
            for (int i = 0; i < terminal.cols - 3; i++) {
                appendBuffer(horizontal, strlen(horizontal));
            }
            appendBuffer(bottom_right, strlen(bottom_right));
        } else {
            // Middle lines with vertical borders
            appendBuffer(vertical, strlen(vertical));
            for (int i = 0; i < terminal.cols - 3; i++) {
                appendBuffer(" ", 1);
            }
            appendBuffer(vertical, strlen(vertical));
        }
        appendBuffer(ANSI_ERASE_TO_EOL, strlen(ANSI_ERASE_TO_EOL));
        if (y < terminal.rows - 1) {
            appendBuffer("\r\n", 2);
        }
    }

    // Adjust cursor position for borders
    char cbuf[32];
    snprintf(cbuf, sizeof(cbuf), ANSI_SET_CURSOR_POSITION, terminal.y + 2, terminal.x + 2);
    appendBuffer(cbuf, strlen(cbuf));

    appendBuffer(ANSI_SHOW_CURSOR, strlen(ANSI_SHOW_CURSOR));  // Show cursor

    writeBuffer();

}

char readKey() {
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
		    if (terminal.x < terminal.cols - 3) { // Adjusted for border
		        terminal.x++;
		    }
		    break;
		case 'w':
		    if (terminal.y > 0) {
		        terminal.y--;
		    }
		    break;
		case 's':
		    if (terminal.y < terminal.rows - 3) { // Adjusted for border
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
        die("sigaction");
    }
}

int main(void) {

	start();
    listen();
    draw();

    fd_set readfds;
    struct timeval tv;

    while (1) {

        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);

        tv.tv_sec = 0;
        tv.tv_usec = 10000; // 10ms timeout

        int ret = select(STDIN_FILENO + 1, &readfds, NULL, NULL, &tv);

        if (ret == -1 && errno != EINTR) {
            die("select");
        }

        if (resize) {
            if (getSize(&terminal.rows, &terminal.cols) == -1) {
                die("getSize");
            }
            if (terminal.y >= terminal.rows) terminal.y = terminal.rows - 1;
            if (terminal.x >= terminal.cols) terminal.x = terminal.cols - 1;
            resize = 0;
            draw();
        }

        if (ret > 0 && FD_ISSET(STDIN_FILENO, &readfds)) {
            char c = readKey();
            if (c != 0) {
                processKey(c);
                draw();
            }
        }

    }

    return 0;

}
