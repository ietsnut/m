#pragma once

#include <stdio.h>
#include <stdint.h>

#define program     int main()
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