#include "lib/terminal.h"
#include <string.h>

#define CANVAS_WIDTH 40
#define CANVAS_HEIGHT 20
#define PALETTE_SIZE 6
#define MAX_SYMBOL_SIZE 4  // Maximum bytes for a UTF-8 character

typedef struct {
    char symbol[MAX_SYMBOL_SIZE];
    int x, y;
    char canvas[CANVAS_HEIGHT][CANVAS_WIDTH][MAX_SYMBOL_SIZE];
    int current_color;
    char palette[PALETTE_SIZE][MAX_SYMBOL_SIZE];
    int is_drawing;
} Editor;

// Function prototypes
void init_editor(Editor *editor);
void draw_interface(Editor *editor);
void draw_canvas(Editor *editor);
void draw_palette(Editor *editor);
void draw_status(Editor *editor);
void handle_input(Editor *editor, char input);
void draw_pixel(Editor *editor);
void handle_resize(void);
void set_symbol(char *dest, const char *src);

// Global terminal instance from your library
extern terminal_t terminal;
Editor editor;

void set_symbol(char *dest, const char *src) {
    strncpy(dest, src, MAX_SYMBOL_SIZE);
}

void init_editor(Editor *editor) {
    editor->x = 0;
    editor->y = 0;
    editor->current_color = 0;
    editor->is_drawing = 0;
    
    // Initialize palette with different characters
    set_symbol(editor->palette[0], "█");
    set_symbol(editor->palette[1], "▒");
    set_symbol(editor->palette[2], "░");
    set_symbol(editor->palette[3], "●");
    set_symbol(editor->palette[4], "◆");
    set_symbol(editor->palette[5], "◯");
    
    // Set initial symbol
    set_symbol(editor->symbol, editor->palette[0]);
    
    // Initialize canvas with spaces
    for (int y = 0; y < CANVAS_HEIGHT; y++) {
        for (int x = 0; x < CANVAS_WIDTH; x++) {
            set_symbol(editor->canvas[y][x], " ");
        }
    }
}

void draw_interface(Editor *editor) {
    terminal.clear();
    
    // Draw main canvas border
    terminal.box(2, 1, CANVAS_WIDTH + 2, CANVAS_HEIGHT + 2);
    
    // Draw palette border
    terminal.box(2, CANVAS_HEIGHT + 3, PALETTE_SIZE + 2, 3);
    
    draw_canvas(editor);
    draw_palette(editor);
    draw_status(editor);
    
    terminal.draw();
}

void draw_canvas(Editor *editor) {
    // Draw the canvas content
    for (int y = 0; y < CANVAS_HEIGHT; y++) {
        for (int x = 0; x < CANVAS_WIDTH; x++) {
            terminal.write(editor->canvas[y][x], x + 3, y + 2);
        }
    }
    
    // Draw cursor
    terminal.write(editor->symbol, editor->x + 3, editor->y + 2);
}

void draw_palette(Editor *editor) {
    for (int i = 0; i < PALETTE_SIZE; i++) {
        terminal.write(editor->palette[i], i + 3, CANVAS_HEIGHT + 4);
    }
    
    // Highlight current color
    terminal.write("^", editor->current_color + 3, CANVAS_HEIGHT + 5);
}

void draw_status(Editor *editor) {
    char status[64];
    snprintf(status, sizeof(status), "Position: %d,%d | Space: toggle draw | 1-6: change symbol", 
             editor->x, editor->y);
    terminal.write(status, 3, CANVAS_HEIGHT + 7);
}

void draw_pixel(Editor *editor) {
    if (editor->is_drawing) {
        set_symbol(editor->canvas[editor->y][editor->x], editor->symbol);
    }
}

void handle_input(Editor *editor, char input) {
    switch (input) {
        case ARROW_UP:
            if (editor->y > 0) editor->y--;
            draw_pixel(editor);
            break;
        case ARROW_DOWN:
            if (editor->y < CANVAS_HEIGHT - 1) editor->y++;
            draw_pixel(editor);
            break;
        case ARROW_LEFT:
            if (editor->x > 0) editor->x--;
            draw_pixel(editor);
            break;
        case ARROW_RIGHT:
            if (editor->x < CANVAS_WIDTH - 1) editor->x++;
            draw_pixel(editor);
            break;
        case ' ':
            editor->is_drawing = !editor->is_drawing;
            draw_pixel(editor);
            break;
        default:
            if (input >= '1' && input <= '6') {
                editor->current_color = input - '1';
                set_symbol(editor->symbol, editor->palette[editor->current_color]);
            }
            break;
    }
}

void handle_resize(void) {
    draw_interface(&editor);
}

int main(void) {
    // Initialize terminal
    terminal.open();
    terminal.title("Pixel Art Editor");
    
    // Initialize editor
    init_editor(&editor);
    
    // Set up resize handler
    terminal.listen(RESIZE, handle_resize);
    
    // Main loop
    while (1) {
        draw_interface(&editor);
        
        char input = terminal.input();
        if (input == 'q') break;
        
        handle_input(&editor, input);
    }
    
    terminal.close();
    return 0;
}