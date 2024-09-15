#include "m.h"

program {

    ENTITIES(
        { 0, 2 },
        { 1, 0 },
        { 2, 0 },
        { 3, 1 }
    );

    printf("size == %zu \n" , sizeof(Entity) * 8);

    prints(entities);

    switch (entities[0].state) {
      case 0:
        printf("zero");
        break;
      case 1:
        printf("one");
        break;
      default:
        printf("wildcard");
    }

    printf("\n");
/*
    for (int i = 0; i < 4; i++) {
        print(entities[i]);
    }
*/
    return 0;

}
