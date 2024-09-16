#include "./include/m.h"

int main(int argc, char *argv[]) {

    printf("You have entered %d arguments:\n", argc);
 
    for (int i = 0; i < argc; i++) {
        printf("%s\n", argv[i]);
    }

    ENTITIES(
        { 0, 0 },
        { 1, 0 },
        { 2, 2 },
        { 3, 1 }
    );

    printf("size == %zu \n" , sizeof(Entity) * 8);

    prints(entities);

    //print(COND(entities[0].state == entities[1].state, entities[0],  entities[1]));

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
