#include <stdio.h>
#include <stdlib.h>

int main() {
    // Define the command to compile m.c using cosmocc
    char command[] = "cosmocc -o m.exe m.c";
    
    // Execute the command using system()
    int result = system(command);
    
    // Check if the command executed successfully
    if (result == 0) {
        printf("Compilation successful!\n");
    } else {
        printf("Compilation failed with error code: %d\n", result);
    }

    return 0;
}