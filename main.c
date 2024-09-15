#include "m.h"

program {

    Entity entities[4];

    entities[0] = (Entity) { 0, 0 };
    entities[1] = (Entity) { 1, 0 };
    entities[2] = (Entity) { 2, 0 };
    entities[3] = (Entity) { 3, 0 };

    printf("size == %zu \n" , sizeof(Entity) * 8);

    for (int i = 0; i < 4; i++) {
        print(entities[i]);
    } 
  
    //start(entities);

}
