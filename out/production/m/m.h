#pragma once

#include <stdio.h>
#ifdef _WIN32
    #include <windows.h>
    #define write(ptr, size) fwrite(ptr, size, 1, stdout)
    #define read(ptr, size)  fread(ptr, 1, size, stdin)
#elif __APPLE__ || __linux__
    #include <unistd.h>
    #define write(ptr, size) write(STDOUT_FILENO, ptr, size)
    #define read(ptr, size)  read(STDIN_FILENO, ptr, size)
#endif

#define program int main()
#define loop    for(;;)

#pragma pack(1)
typedef struct {
    byte start;
    byte scene; 
    byte shape; 
    byte state;
} Entity;
#pragma pack()

static inline void print(Entity entity) {
    printf("%d %d %d\n", entity.scene, entity.shape, entity.state);
}

static inline void out(Entity entity) {
    write(&entity, sizeof(entity));
    fflush(stdout);
}
