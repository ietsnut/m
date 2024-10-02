#include <stdio.h>
#include <stdlib.h>

int main() {
    // Step 1: Download cosmocc.zip to the current directory
    printf("Downloading cosmocc.zip...\n");

    char downloadCommand[] = "curl -O https://cosmo.zip/pub/cosmocc/cosmocc.zip";
    int downloadResult = system(downloadCommand);
    
    if (downloadResult != 0) {
        printf("Failed to download cosmocc.zip. Error code: %d\n", downloadResult);
        return 1;
    }

    // Step 2: Unzip cosmocc.zip to extract cosmocc
    printf("Unzipping cosmocc.zip...\n");
    
    char unzipCommand[] = "unzip cosmocc.zip";
    int unzipResult = system(unzipCommand);
    
    if (unzipResult != 0) {
        printf("Failed to unzip cosmocc.zip. Error code: %d\n", unzipResult);
        return 1;
    }

    // Step 3: Compile m.c using cosmocc in the current directory
    printf("Compiling m.c using cosmocc...\n");

    char compileCommand[] = "./cosmocc -o m.exe m.c";
    int compileResult = system(compileCommand);
    
    // Step 4: Check if the compilation was successful
    if (compileResult == 0) {
        printf("Compilation successful!\n");
    } else {
        printf("Compilation failed with error code: %d\n", compileResult);
    }

    return 0;
}
