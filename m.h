#pragma once

#ifndef M_H
#define M_H

#include <stdio.h>
#include <stdint.h>

#ifdef _WIN32
    #include <windows.h>
    #define write(ptr, size) fwrite(ptr, size, 1, stdout)
    #define read(ptr, size)  fread(ptr, 1, size, stdin)
#else
    #include <unistd.h>
    #define write(ptr, size) write(STDOUT_FILENO, ptr, size)
    #define read(ptr, size)  read(STDIN_FILENO, ptr, size)
#endif

#define program     int main(void)
#define loop        for(;;)
#define until(cond) while (!(cond))

typedef struct {
    uint8_t start : 8;
    uint8_t scene : 8; 
    uint8_t shape : 8; 
    uint8_t state : 8;
} Entity;

static inline void print(Entity entity) {
    printf("%d %d %d\n", entity.scene, entity.shape, entity.state);
}

static inline void start(void) {
    #ifdef _WIN32
        _setmode(_fileno(stdout), _O_BINARY);
    #endif
}

static inline void out(Entity entity) {
    write(&entity, 4);
    fflush(stdout);
}

static inline void in(Entity* in) {
    until (read(in, sizeof(*in)) == sizeof(Entity) && in->start == 0);
}

#endif
