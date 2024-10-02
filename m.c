#include "rogueutil.h"
#include <stdlib.h> /* for srand() / rand() */
#include <stdio.h>
#include <ctype.h>  /* for isalpha() */

int box_x = 1;   // Initial x position of the box
int box_y = 1;   // Initial y position of the box
int box_width = 20;
int box_height = 5;

void draw() {
    cls();  // Clear the screen
    drawBox(box_x, box_y, box_width, box_height);  // Draw the box at the current coordinates
    locate(box_x + 1, box_y + 1);
    fflush(stdout);
}

int main(void) {
    hidecursor();             // Hide cursor
    saveDefaultColor();        // Save default color settings
    draw();
    int quit = 0;
    while (!quit) {
        if (kbhit()) {
            int key = getkey();

            switch (key) {
                case KEY_ESCAPE:  // Exit the loop when Escape is pressed
                    quit = 1;
                    break;
                case KEY_LEFT:  // Move left
                    if (box_x > 0) {
                        box_x--;
                        draw();
                    }
                    break;
                case KEY_RIGHT:  // Move right
                    if (box_x < tcols() - 3) {
                        box_x++;
                        draw();
                    }
                    break;
                case KEY_UP:  // Move up
                    if (box_y > 0) {
                        box_y--;
                        draw();
                    }
                    break;
                case KEY_DOWN:  // Move down
                    if (box_y < trows() - box_height) {
                        box_y++;
                        draw();
                    }
                    break;
                default:
                    break;
            }
        }
    }

    cls();           // Clear the screen when exiting
    resetColor();    // Reset color settings
    showcursor();    // Show the cursor again
    fflush(stdout);  // Ensure everything is printed

    return 0;
}
