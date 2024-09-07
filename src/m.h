#pragma once

#include <stdio.h>
#ifdef _WIN32
    #include <windows.h>
    #define write(ptr, size) fwrite(ptr, size, 1, stdout)
#elif __APPLE__
    #include <unistd.h>
    #define write(ptr, size) write(STDOUT_FILENO, ptr, size)
#elif __linux__
    #include <unistd.h>
    #define write(ptr, size) write(STDOUT_FILENO, ptr, size)
#endif

#define program int main()
#define loop    for(;;)

typedef struct { 
    unsigned char scene : 8; 
    unsigned char shape : 8; 
    unsigned char state : 8;  
} Entity;

static inline void print(Entity entity) {
    printf("%d %d %d\n", entity.scene, entity.shape, entity.state);
}

static inline void process(Entity entity) {
    write(&entity, sizeof(entity));
}