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
#define MAX_LINES 1000
#define MAX_LINE_LENGTH 1000

const char *ascii_image[] = {
    "      ┌───┐         ┌───┐      ",
    "      │ O └─────────┘ O │      ",
    "      └─┐  ┌───U───┐  ┌─┘      ",
    "RESET  0│O─┤       ├─O│4  VCC  ",
    "XTAL1  1│O─┤  AVR  ├─O│5  SCK  ",
    "XTAL2  2│O─┤  T85  ├─O│6  MISO ",
    "  GND  3│O─┤       ├─O│7  MOSI ",
    "      ┌─┘  └───────┘  └─┐      ",
    "      │ O ┌─────────┐ O │      ",
    "      └───┘         └───┘      "
};

#define ASCII_IMAGE_HEIGHT (sizeof(ascii_image) / sizeof(ascii_image[0]))
#define ASCII_IMAGE_WIDTH 32

typedef enum State {
    DEFAULT,
    HELPING,
    EXTRACTING
} State;

State state = DEFAULT;

// Text editor data
char **text_buffer;
int num_lines = 1; // Start with one empty line
int scroll_offset = 0;

void init_text_buffer() {
    text_buffer = malloc(MAX_LINES * sizeof(char*));
    for (int i = 0; i < MAX_LINES; i++) {
        text_buffer[i] = malloc(MAX_LINE_LENGTH * sizeof(char));
        text_buffer[i][0] = '\0';
    }
}

void free_text_buffer() {
    for (int i = 0; i < MAX_LINES; i++) {
        free(text_buffer[i]);
    }
    free(text_buffer);
}

void insert_char(char c) {
    if (strlen(text_buffer[terminal.y]) < MAX_LINE_LENGTH - 1) {
        memmove(&text_buffer[terminal.y][terminal.x + 1], &text_buffer[terminal.y][terminal.x], 
                strlen(&text_buffer[terminal.y][terminal.x]) + 1);
        text_buffer[terminal.y][terminal.x] = c;
        terminal.x++;
    }
}

void delete_char() {
    if (terminal.x > 0) {
        memmove(&text_buffer[terminal.y][terminal.x - 1], &text_buffer[terminal.y][terminal.x], 
                strlen(&text_buffer[terminal.y][terminal.x]) + 1);
        terminal.x--;
    } else if (terminal.y > 0) {
        int prev_len = strlen(text_buffer[terminal.y - 1]);
        strcat(text_buffer[terminal.y - 1], text_buffer[terminal.y]);
        memmove(&text_buffer[terminal.y], &text_buffer[terminal.y + 1], 
                (MAX_LINES - terminal.y - 1) * sizeof(char*));
        text_buffer[MAX_LINES - 1][0] = '\0';
        terminal.y--;
        terminal.x = prev_len;
        num_lines--;
    }
}

void delete_char_forward() {
    if (terminal.x < strlen(text_buffer[terminal.y])) {
        memmove(&text_buffer[terminal.y][terminal.x], &text_buffer[terminal.y][terminal.x + 1], 
                strlen(&text_buffer[terminal.y][terminal.x + 1]) + 1);
    } else if (terminal.y < num_lines - 1) {
        strcat(text_buffer[terminal.y], text_buffer[terminal.y + 1]);
        memmove(&text_buffer[terminal.y + 1], &text_buffer[terminal.y + 2], 
                (MAX_LINES - terminal.y - 2) * sizeof(char*));
        text_buffer[MAX_LINES - 1][0] = '\0';
        num_lines--;
    }
}

void insert_newline() {
    if (num_lines < MAX_LINES - 1) {
        memmove(&text_buffer[terminal.y + 2], &text_buffer[terminal.y + 1], 
                (MAX_LINES - terminal.y - 2) * sizeof(char*));
        text_buffer[terminal.y + 1] = malloc(MAX_LINE_LENGTH * sizeof(char));
        strcpy(text_buffer[terminal.y + 1], &text_buffer[terminal.y][terminal.x]);
        text_buffer[terminal.y][terminal.x] = '\0';
        terminal.y++;
        terminal.x = 0;
        num_lines++;
    }
}

void draw_text() {
    int editor_height = terminal.rows - 2;
    int editor_width = terminal.cols - 2;

    for (int i = 0; i < editor_height; i++) {
        int line = i + scroll_offset;
        if (line < num_lines) {
            int len = strlen(text_buffer[line]);
            for (int j = 0; j < editor_width; j++) {
                if (j < len) {
                    terminal.draw(&text_buffer[line][j], j + 1, i + 1);
                } else {
                    terminal.draw(" ", j + 1, i + 1);
                }
            }
        }
    }
}

void clear() {
    for (int i = 0; i < terminal.rows; i++) {
        for (int j = 0; j < terminal.cols; j++) {
            terminal.draw(" ", j, i);
        }
    }
}

void draw() {
    terminal.box(0, 0, terminal.cols, terminal.rows);

    // Draw the title
    int content_width = terminal.cols - 3;
    int welcomelen = strlen("┤ MEDITOR ├") - 4;
    if (welcomelen > content_width) {
        welcomelen = content_width;
    }
    int padding_left = (content_width - welcomelen) / 2;
    int padding_right = content_width - welcomelen - padding_left;

    for (int i = 0; i < padding_left; i++) {
        terminal.draw(horizontal, 1 + i, 0);
    }

    if (welcomelen > 0) {
        terminal.draw("┤ MEDITOR ├", 1 + padding_left, 0);
    }

    for (int i = 0; i < padding_right; i++) {
        terminal.draw(horizontal, 1 + padding_left + welcomelen + i, 0);
    }

    draw_text();
}

void draw_ascii_image() {
    int left_padding = 4;
    int right_padding = 4;
    int top_padding = 3;
    int box_width = ASCII_IMAGE_WIDTH + left_padding + right_padding + 1;
    int box_height = ASCII_IMAGE_HEIGHT + (2 * top_padding) + 1;
    int start_x = (terminal.cols - box_width) / 2;
    int start_y = (terminal.rows - box_height) / 2;

    // Draw the box
    terminal.box(start_x, start_y, box_width, box_height);

    // Draw top title
    terminal.draw("╭─────────────────╮", start_x + (box_width - 18) / 2, start_y - 1);
    terminal.draw("┤ MICROCONTROLLER ├", start_x + (box_width - 18) / 2, start_y);
    terminal.draw("╰─────────────────╯", start_x + (box_width - 18) / 2, start_y + 1);

    // Draw image
    for (int i = 0; i < ASCII_IMAGE_HEIGHT; i++) {
        for (int j = 0; j < left_padding; j++) {
            terminal.draw(" ", start_x + 1 + j, start_y + top_padding + i + 1);
        }
        terminal.draw(ascii_image[i], start_x + 1 + left_padding, start_y + top_padding + i + 1);
        for (int j = 0; j < right_padding - 1; j++) {
            terminal.draw(" ", start_x + 1 + left_padding + ASCII_IMAGE_WIDTH + j, start_y + top_padding + i + 1);
        }
    }
}

void refresh() {
    terminal.reset();
    switch(state) {
        case HELPING:
            draw();
            draw_ascii_image();
            break;
        case EXTRACTING:
            draw();
            terminal.write("*", ANSI_PINK, (terminal.cols/2)-(strlen("*")/2), terminal.rows/2);
            break;
        case DEFAULT:
            draw();
            break;
    }
    terminal.cursor(terminal.x + 1, terminal.y - scroll_offset + 1);
    terminal.show();
}

void write_headers_to_file(FILE *fp) {
    fprintf(fp, "#define F_CPU 8000000UL\n");
    fprintf(fp, "#include \"blink.h\"\n\n");
}

void compile_and_program() {
    // Write text buffer to file
    FILE *fp = fopen("blink.c", "w");
    if (fp == NULL) {
        perror("Error opening file");
        return;
    }
    write_headers_to_file(fp);
    for (int i = 0; i < num_lines; i++) {
        fprintf(fp, "%s\n", text_buffer[i]);
    }
    fclose(fp);

    // Commands to run
    const char *commands[] = {
        "./resource/windows/avrgcc/bin/avr-gcc.exe -g -Os -mmcu=attiny85 -DF_CPU=8000000UL -o blink.elf blink.c",
        "./resource/windows/avrgcc/bin/avr-objcopy.exe -O ihex blink.elf blink.hex",
        "./resource/windows/avrdude/avrdude.exe -P usb:04d8:00dd -c stk500v1 -p t85 -b 19200 -U lfuse:w:0xE2:m -U hfuse:w:0xDF:m",
        "./resource/windows/avrdude/avrdude.exe -P usb:04d8:00dd -c stk500v1 -p t85 -b 19200 -U flash:w:blink.hex:i"
    };

    // Buffer to store command output
    int return_codes[4];
    for (int i = 0; i < 4; i++) {
        return_codes[i] = system(commands[i]);
        if (return_codes[i] != 0) {
            break; // Stop executing further commands if one fails
        }
    }

    // Display output in centered window
    int window_width = 40;
    int window_height = 10;
    int start_x = (terminal.cols - window_width) / 2;
    int start_y = (terminal.rows - window_height) / 2;

    clear();
    terminal.box(start_x, start_y, window_width, window_height);

    // Draw title
    const char *title = "Compilation and Programming Results";
    terminal.draw(title, start_x + (window_width - strlen(title)) / 2, start_y);

    // Draw return codes
    for (int i = 0; i < 4; i++) {
        char result[40];
        snprintf(result, sizeof(result), "Command %d return code: %d", i + 1, return_codes[i]);
        terminal.draw(result, start_x + 2, start_y + 2 + i);
    }

    // Wait for user input to close the window
    terminal.draw("Press any key to continue...", start_x + 2, start_y + window_height - 2);
    terminal.input();

    // Redraw the main screen
    refresh();
}

void processKey(char c) {
    clear();
    char debug_msg[100];
    snprintf(debug_msg, sizeof(debug_msg), "Key pressed: %d", c);
    terminal.draw(debug_msg, 2, terminal.rows - 2);
    switch (c) {
        case '\x1b':
            terminal.close();
            break;
        case '?':
            if (state == HELPING) {
                state = DEFAULT;
            } else {
                state = HELPING;
            }
            break;
        case '!':
            compile_and_program();
            break;
        case '\r':
            insert_newline();
            break;
        case 127: // Backspace
        case '\b': // Also handle the actual backspace character
            delete_char();
            break;
        case DELETE_KEY:
            delete_char_forward();
            break;
        case ARROW_UP:
            if (terminal.y > 0) terminal.y--;
            break;
        case ARROW_DOWN:
            if (terminal.y < num_lines - 1) terminal.y++;
            break;
        case ARROW_LEFT:
            if (terminal.x > 0) terminal.x--;
            break;
        case ARROW_RIGHT:
            if (terminal.x < strlen(text_buffer[terminal.y])) terminal.x++;
            break;
        default:
            if (isprint(c)) {
                insert_char(c);
            }
            break;
    }
    refresh();
}

static void handle_resize(void) {
    clear();
    refresh();
}

// run these commands but use the internal text_buffer instead of blink.c
// ./resource/windows/avrgcc/bin/avr-gcc.exe  -g -Os -mmcu=attiny85 -DF_CPU=8000000UL -o blink.elf blink.c
// ./resource/windows/avrgcc/bin/avr-objcopy.exe -O ihex blink.elf blink.hex
// ./resource/windows/avrdude/avrdude.exe -c stk500v1 -P ch340 -p t85 -b 19200 -U flash:w:blink.hex:i
// ./resource/windows/avrdude/avrdude.exe -c stk500v1 -P ch340 -p t85 -b 19200 -U lfuse:w:0xE2:m -U hfuse:w:0xDF:m

int main(int argc, char *argv[]) {
    terminal.open();
    terminal.title("[ MEDITOR ]");
    terminal.listen(RESIZE, handle_resize);
    terminal.listen(INPUT, processKey);
    
    init_text_buffer();
    
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
    free_text_buffer();
    return 0;
}