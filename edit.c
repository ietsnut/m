#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h> 

#define VERSION "0.0.1"
#define CTRL_KEY(k) ((k) & 0x1f)

struct terminal {
	int cx, cy;
	int rows, cols;
	struct termios state;
};

struct buffer {
  	char *b;
  	int len;
};

#define BUFFER_INIT {NULL, 0}

struct terminal T;

void appendBuffer(struct buffer *buf, const char *s, int len) {
  	char *new = realloc(buf->b, buf->len + len);
  	if (new == NULL) {
  		return;
  	}
  	memcpy(&new[buf->len], s, len);
  	buf->b = new;
  	buf->len += len;
}

void freeBuffer(struct buffer *buf) {
  	free(buf->b);
}

void die(const char *s) {
	write(STDOUT_FILENO, "\x1b[H", 3);
	write(STDOUT_FILENO, "\x1b[2J", 4);
	perror(s);
	exit(1);
}

// get cursor position
int getCursor(int *rows, int *cols) {
	char buf[32];
  	unsigned int i = 0;
  	if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) {
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
  return -1;
}

// get window size
int getSize(int *rows, int *cols) {
	struct winsize ws;
  	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
    	if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) {
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
  	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &T.state) == -1) {
  		die("tcsetattr");
  	}
}

void setTerminalTitle(const char *title) {
    char buf[512];
    snprintf(buf, sizeof(buf), "\x1b]0;%s\x07", title);
    write(STDOUT_FILENO, buf, strlen(buf));
}

void start(void) {
	if (tcgetattr(STDIN_FILENO, &T.state) == -1) {
		die("tcgetattr");
	}
	atexit(stop);
	struct termios raw = T.state;
	raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);	// TURN OFF CTRL-S/Q/M
	raw.c_oflag &= ~(OPOST);									// TURN OFF OUTPUT PROCESSING
	raw.c_cflag |= (CS8);
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);			// TURN OFF ECHO, CANONICAL MODE, CTRL-C/Z/V
	raw.c_cc[VMIN] 	= 0;
	raw.c_cc[VTIME] = 1;
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
		die("tcsetattr");
	}
	T.cx = 0;
  	T.cy = 0;
	if (getSize(&T.rows, &T.cols) == -1) {
		die("getSize");
	}
    setTerminalTitle("MEDITOR");
}

void draw() {
	struct buffer buf = BUFFER_INIT;
	appendBuffer(&buf, "\x1b[?25l", 6);
  	appendBuffer(&buf, "\x1b[H", 3);
	int y;
  	for (y = 0; y < T.rows; y++) {
  		if (y == T.rows / 2) {
  			char welcome[80];
  			int welcomelen = snprintf(welcome, sizeof(welcome), "MEDITOR -- version %s", VERSION);
  			if (welcomelen > T.cols) {
  				welcomelen = T.cols;
  			}
  			int padding = (T.cols - welcomelen) / 2;
			if (padding) {
				appendBuffer(&buf, "~", 1);
				padding--;
			}
			while (padding--) appendBuffer(&buf, " ", 1);
  			appendBuffer(&buf, welcome, welcomelen);
  		} else {
  			appendBuffer(&buf, "~", 1);
  		}
  		appendBuffer(&buf, "\x1b[K", 3);
    	if (y < T.rows - 1) {
      		appendBuffer(&buf, "\r\n", 2);
    	}
  	}
  	appendBuffer(&buf, "\x1b[H", 3);

	char cbuf[32];
	snprintf(cbuf, sizeof(cbuf), "\x1b[%d;%dH", T.cy + 1, T.cx + 1);
	appendBuffer(&buf, cbuf, strlen(cbuf));

  	appendBuffer(&buf, "\x1b[?25h", 6);
  	write(STDOUT_FILENO, buf.b, buf.len);
  	freeBuffer(&buf);
}

void moveCursor(char key) {
	switch (key) {
		case 'a':
			T.cx--;
			break;
		case 'd':
			T.cx++;
			break;
		case 'w':
			T.cy--;
			break;
    	case 's':
			T.cy++;
			break;
	}
    if (T.cy >= T.rows) T.cy = T.rows - 1;
    if (T.cy <= 0) T.cy = 0;
    if (T.cx >= T.cols) T.cx = T.cols - 1;
    if (T.cx <= 0) T.cx = 0;
}

char readKey() {
    int nread;
    char c;
    while (1) {
        nread = read(STDIN_FILENO, &c, 1);
        if (nread == 1) {
            break; // Character read successfully
        } else if (nread == 0) {
            // No input, read() timed out
            return 0; // Indicate no input was read
        } else if (nread == -1) {
            if (errno == EINTR) {
                // Interrupted by signal (e.g., SIGWINCH)
                return 0; // Indicate no input was read
            } else if (errno == EAGAIN) {
                continue; // Try reading again
            } else {
                die("read");
            }
        }
    }

    if (c == '\x1b') {
        // Handle escape sequences as before
        char seq[3];
        if (read(STDIN_FILENO, &seq[0], 1) != 1) {
            return '\x1b';
        }
        if (read(STDIN_FILENO, &seq[1], 1) != 1) {
            return '\x1b';
        }
        if (seq[0] == '[') {
            switch (seq[1]) {
                case 'A': return 'w';
                case 'B': return 's';
                case 'C': return 'd';
                case 'D': return 'a';
            }
        }
        return '\x1b';
    } else {
        return c;
    }
}

// read input (blocking)
void getInput() {
    char c = readKey();
    if (c == 0) {
        // No input was read; return to allow main loop to proceed
        return;
    }
    switch (c) {
        case CTRL_KEY('q'):
        case 27:
            write(STDOUT_FILENO, "\x1b[H", 3);
            write(STDOUT_FILENO, "\x1b[2J", 4);
            exit(0);
            break;
        case 'w':
        case 's':
        case 'a':
        case 'd':
            moveCursor(c);
            break;
    }
}

volatile sig_atomic_t window_resized = 0;

static void sig_handler(int sig) {
    if (sig == SIGWINCH) {
        window_resized = 1;
    }
}

void resizeEvent() {
    struct sigaction sa;
    sa.sa_handler = sig_handler;
    sa.sa_flags = 0; // No flags needed
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGWINCH, &sa, NULL) == -1) {
        die("sigaction");
    }
}

int main(void) {
    resizeEvent();

    start();
    draw();
    while (1) {
        if (window_resized) {
            if (getSize(&T.rows, &T.cols) == -1) {
                die("getSize");
            }
            // Adjust cursor position if necessary
            if (T.cy >= T.rows) T.cy = T.rows - 1;
            if (T.cx >= T.cols) T.cx = T.cols - 1;
            window_resized = 0;
            draw();
        }
        
        getInput();
        draw();
    }

    return 0;
}