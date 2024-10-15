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
    center_box("Test");  // Draw the box at the current coordinates
    locate(box_x + 1, box_y + 1);
    fflush(stdout);
}

int main(void) {
    hidecursor();             // Hide cursor
    saveDefaultColor();        // Save default color settings
    int quit = 0;
    while (!quit) {
        if (kbhit()) {
            int key = getkey();
            switch (key) {
                case KEY_ESCAPE:  // Exit the loop when Escape is pressed
                    quit = 1;
                    break;
                default:
                    break;
            }
        }
        draw();
    }

    cls();           // Clear the screen when exiting
    resetColor();    // Reset color settings
    showcursor();    // Show the cursor again
    fflush(stdout);  // Ensure everything is printed

    return 0;
}
