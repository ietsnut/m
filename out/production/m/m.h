#pragma once

#include <stdio.h>
#include <fcntl.h>

#ifdef _WIN32
    #include <windows.h>
    #define write(ptr, size) fwrite(ptr, size, 1, stdout)
    #define read(ptr, size)  fread(ptr, 1, size, stdin)
#elif __APPLE__ || __linux__
    #include <unistd.h>
    #define write(ptr, size) write(STDOUT_FILENO, ptr, size)
    #define read(ptr, size)  read(STDIN_FILENO, ptr, size)
#endif

#define program     int main()
#define loop        for(;;)
#define until(cond) while (!(cond))

typedef struct {
    byte start;
    byte scene; 
    byte shape; 
    byte state;
} Entity;

static inline void print(Entity entity) {
    printf("%d %d %d\n", entity.scene, entity.shape, entity.state);
}

static inline void out(Entity entity) {
    write(&entity, 4);
    fflush(stdout);
}

static inline void in(Entity* in) {
    until (read(in, sizeof(*in)) == sizeof(Entity) && in->start == 0);
}

int endian() {
    unsigned int x = 1;
    char *c = (char*)&x;
    return (*c == 1) ? 1 : 0;
}

