#pragma once

#ifndef M_H
#define M_H

#define program     int main(void)
#define loop        for(;;)
#define until(cond) while (!(cond))

#include <stdint.h>

#ifdef __AVR__
    #include <avr/io.h>
    #include <util/delay.h>
#else
    #include <stdio.h>
    #ifdef _WIN32
        #include <windows.h>
        #define write(ptr, size) fwrite(ptr, size, 1, stdout)
        #define read(ptr, size)  fread(ptr, 1, size, stdin)
    #else
        #include <unistd.h>
        #define write(ptr, size) write(STDOUT_FILENO, ptr, size)
        #define read(ptr, size)  read(STDIN_FILENO, ptr, size)
    #endif
#endif

typedef struct {
    uint8_t id      : 8; 
    uint8_t state   : 8;
} Entity;

Entity incoming;

#define EQUALS(a, b)        ((a).id == (b).id)
#define MIN(a, b)           ((a) + (((b) - (a)) & -((b) < (a))))
#define MAX(a, b)           ((a) - (((a) - (b)) & -((a) < (b))))
#define CLAMP(x, mi, ma)    (MIN(MAX((x), (mi)), (ma)))
#define IN_RANGE(x, mi, ma) (((x) >= (mi)) && ((x) <= (ma)))
#define COND(cond, x, y)    ((y) ^ (((x) ^ (y)) & -!!(cond)))
#define IS_EVEN(x)          (!((x) & 1))
#define IS_ODD(x)           ((x) & 1)
#define IS_POWER_OF_TWO(x)  ((x) && !((x) & ((x) - 1)))
#define IS_MULTIPLE(x, n)   (!((x) & ((n) - 1)))
#define SWAP(a, b)          ((a) ^= (b), (b) ^= (a), (a) ^= (b))
#define TOGGLE(x)           ((x) ^= 1)
#define DIFFERENCE(a, b)    ((a) > (b) ? (a) - (b) : (b) - (a))

static inline int start(void) {
    #ifdef _WIN32
        _setmode(_fileno(stdout), _O_BINARY);
    #endif
    //printf("size == %zu \n" , sizeof(Entity));
    //print(self);
    loop {
        in(&incoming);
        for(size_t i = 0; i < sizeof(entities) / 2; i++) {
            change_to_highest(entities[i]);
        }
        //self.state += other.state;
        out(self);
    }

    return 0;
}

static inline void print(Entity entity) {
    printf("%d %d %d\n", entity.start, entity.id, entity.state);
}

static inline void out(Entity entity) {
    write(&entity, 2);
    fflush(stdout);
}

static inline void in(Entity* entity) {
    until(read(entity, sizeof(*entity)) == 2);
}

#endif
