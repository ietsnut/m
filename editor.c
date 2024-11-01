#include "lib/terminal.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <ctype.h>

typedef struct Note {
    int x, y; // Coordinates on the infinite canvas
    char *text;
    struct Note *next;
} Note;

Note *notes = NULL;

int viewport_x = 0, viewport_y = 0; // The top-left corner of the viewport
int need_redraw = 1;

void add_note(int x, int y, const char *text) {
    Note *new_note = (Note *)malloc(sizeof(Note));
    new_note->x = x;
    new_note->y = y;
    new_note->text = strdup(text);
    new_note->next = notes;
    notes = new_note;
}

Note *find_note(int x, int y) {
    Note *current = notes;
    while (current) {
        if (current->x == x && current->y == y)
            return current;
        current = current->next;
    }
    return NULL;
}

void free_notes(void) {
    Note *current = notes;
    while (current) {
        Note *next = current->next;
        free(current->text);
        free(current);
        current = next;
    }
}

void draw_canvas(void) {
    // Clear the terminal buffer
    terminal.clear();

    int center_x = terminal.cols / 2;
    int center_y = terminal.rows / 2;

    // Draw crosshair lines
    for (int x = 0; x < terminal.cols; x++) {
        if (x < center_x - 2 || x > center_x + 2) { // Skip the center to make it hollow
            terminal.write(horizontal, x, center_y);
        }
    }
    for (int y = 0; y < terminal.rows; y++) {
        if (y < center_y - 1 || y > center_y + 1) { // Skip the center to make it hollow
            terminal.write(vertical, center_x, y);
        }
    }

    // Draw hollow square at the center
    terminal.box(center_x - 2, center_y - 1, 5, 3);

    terminal.write(top_join, center_x, center_y - 1);
    terminal.write(bottom_join, center_x, center_y + 1);
    terminal.write(left_join, center_x - 2, center_y);
    terminal.write(right_join, center_x + 2, center_y);

    // Draw any notes that are within the viewport
    Note *current = notes;
    while (current) {
        int screen_x = current->x - viewport_x + center_x;
        int screen_y = current->y - viewport_y + center_y;
        if (screen_x >= 0 && screen_x < terminal.cols &&
            screen_y >= 0 && screen_y < terminal.rows) {
            // Draw a dot at (screen_x, screen_y)
            terminal.write("¤", screen_x, screen_y);
        }
        current = current->next;
    }

    // Flush the buffer to the terminal
    terminal.draw();
    terminal.setting(ANSI_HIDE_CURSOR);
}

void enter_text_mode(int x, int y, char *initial_text) {
    
    int box_width = terminal.cols / 2;
    int box_height = terminal.rows / 2;
    int box_x = (terminal.cols - box_width) / 2;
    int box_y = (terminal.rows - box_height) / 2;

    // Save the current terminal buffer
    char *saved_buffer = malloc(terminal.buffer_length);
    memcpy(saved_buffer, terminal.buffer, terminal.buffer_length);
    int saved_length = terminal.buffer_length;

    // Draw the box
    terminal.box(box_x, box_y, box_width, box_height);
    terminal.write(top_join, box_x + box_width/2 + 1, box_y);
    terminal.write(bottom_join, box_x + box_width/2 + 1, box_y + box_height - 1);
    terminal.write(left_join, box_x, box_y + box_height/2 + 1);
    terminal.write(right_join, box_x + box_width - 1, box_y + box_height/2 + 1);

    // Prepare the text input area
    int text_x = box_x + 4;
    int text_y = box_y + 2;
    int text_width = box_width - 8;
    int text_height = box_height - 4;

    char buffer[1024] = {0};
    int len = 0;
    if (initial_text) {
        strncpy(buffer, initial_text, sizeof(buffer) - 1);
        len = strlen(buffer);
    }

    int cursor_pos = len;

    terminal.cursor(text_x, text_y);
    terminal.draw();
    terminal.setting(ANSI_SHOW_CURSOR);

    int done = 0;
    while (!done) {
        // Clear the text area

        for (int x = -3; x < text_width + 3; ++x) {
            for (int y = -1; y < text_height + 1; ++y) {
                terminal.write(" ", text_x + x,  text_y + y);
            }
        }

        // Write the text buffer
        int line = 0, col = 0;
        for (int i = 0; i < len; i++) {
            if (buffer[i] == '\n' || col >= text_width) {
                line++;
                col = 0;
            }
            if (line >= text_height) break;
            if (buffer[i] != '\n') {
                terminal.write((char[]){buffer[i], '\0'}, text_x + col, text_y + line);
                col++;
            }
        }

        // Position the cursor
        int cursor_line = 0, cursor_col = 0;
        for (int i = 0; i < cursor_pos; i++) {
            if (buffer[i] == '\n' || cursor_col >= text_width) {
                cursor_line++;
                cursor_col = 0;
            } else {
                cursor_col++;
            }
        }
        if (cursor_line >= text_height) cursor_line = text_height - 1;
        if (cursor_col >= text_width) cursor_col = text_width - 1;

        terminal.cursor(text_x + cursor_col, text_y + cursor_line);
        terminal.draw();

        char c = terminal.input();

        if (c == 'q') {
            // Save and exit
            done = 1;
        } else if (c == 27) { // Esc key
            // Cancel and exit
            len = 0;
            buffer[0] = '\0';
            done = 1;
        } else if (c == 127 || c == '\b') { // Handle backspace
            if (cursor_pos > 0) {
                memmove(&buffer[cursor_pos - 1], &buffer[cursor_pos], len - cursor_pos);
                cursor_pos--;
                len--;
                buffer[len] = '\0';
            }
        } else if (c == ARROW_LEFT) {
            if (cursor_pos > 0) cursor_pos--;
        } else if (c == ARROW_RIGHT) {
            if (cursor_pos < len) cursor_pos++;
        } else if (c == ARROW_UP || c == ARROW_DOWN) {
            // Optional: handle moving cursor up/down in multi-line text
        } else if (isprint(c) || c == '\n') {
            if (len < sizeof(buffer) - 1) {
                memmove(&buffer[cursor_pos + 1], &buffer[cursor_pos], len - cursor_pos);
                buffer[cursor_pos] = c;
                cursor_pos++;
                len++;
                buffer[len] = '\0';
            }
        }
    }

    if (len > 0) {
        // Save the text at (x, y)
        Note *existing_note = find_note(x, y);
        if (existing_note) {
            // Update the text
            free(existing_note->text);
            existing_note->text = strdup(buffer);
        } else {
            // Add new note
            add_note(x, y, buffer);
        }
    }

    // Restore the terminal buffer
    memcpy(terminal.buffer, saved_buffer, saved_length);
    terminal.buffer_length = saved_length;
    free(saved_buffer);

    terminal.setting(ANSI_HIDE_CURSOR);
}

void on_resize(void) {
    need_redraw = 1;
}

int main(void) {
    terminal.open();
    terminal.title("Infinite Canvas");

    terminal.listen(RESIZE, on_resize);

    int running = 1;
    char c;

    while (running) {
        if (need_redraw) {
            draw_canvas();
            need_redraw = 0;
        }

        c = terminal.input();

        switch (c) {
            case ARROW_UP:
                viewport_y -= 1;
                need_redraw = 1;
                break;
            case ARROW_DOWN:
                viewport_y += 1;
                need_redraw = 1;
                break;
            case ARROW_LEFT:
                viewport_x -= 1;
                need_redraw = 1;
                break;
            case ARROW_RIGHT:
                viewport_x += 1;
                need_redraw = 1;
                break;
            case 27: // Escape key
                running = 0;
                break;
            case ' ': {
                int center_x = viewport_x;
                int center_y = viewport_y;

                Note *note = find_note(center_x, center_y);
                if (note) {
                    // Edit the existing note
                    enter_text_mode(center_x, center_y, note->text);
                } else {
                    // Enter text mode with empty text
                    enter_text_mode(center_x, center_y, NULL);
                }
                need_redraw = 1;
                break;
            }
            default:
                break;
        }
    }

    terminal.close();
    free_notes();

    return 0;
}
