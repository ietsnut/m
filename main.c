#include "m.h"

program {

    ENTITIES(
        { 0, 0 },
        { 1, 0 },
        { 2, 0 },
        { 3, 1 }
    );

    printf("size == %zu \n" , sizeof(Entity) * 8);

    prints(entities);
/*
    for (int i = 0; i < 4; i++) {
        print(entities[i]);
    }
*/
    return 0;

}
