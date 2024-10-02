#pragma once

#ifndef M_H
#define M_H

#define loop        for(;;)
#define until(cond) while (!(cond))

#include <stdint.h>
#include <stdlib.h>

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

#pragma pack(push, 1)
typedef struct {
    uint8_t id      : 8; 
    uint8_t state   : 8;
} Entity;
#pragma pack(pop)

#define COUNT(...) (sizeof((Entity[]){ __VA_ARGS__ }) / sizeof(Entity))
#define ENTITIES(...) \
    Entity entities[COUNT(__VA_ARGS__)] = { __VA_ARGS__ }

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
#define ABSDIFFERENCE(a, b) ((a) > (b) ? (a) - (b) : (b) - (a))

/*
static inline int start(Entity entities[]) {
    #ifdef _WIN32
        _setmode(_fileno(stdout), _O_BINARY);
    #endif
    loop {
        in(&incoming);
        self.state += incoming.state;
        out(self);
    }

    return 0;
}
*/

static inline void print(Entity entity) {
    printf("ID: %d, STATE: %d\n",  entity.id, entity.state); 
}

static inline void prints(Entity entities[]) {
    for (int i = 0; i < 4; i++) {
        print(entities[i]);
    }
}

static inline void out(Entity entity) {
    write(&entity, 2);
    fflush(stdout);
}

static inline void in(Entity* entity) {
    until(read(entity, sizeof(*entity)) == 2);
}

#endif
