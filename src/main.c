#include "m.h"

Entity self = { 1, 1, 2 };

program {

    //printf("entity struct: %zu bytes\n", sizeof(Entity));

    print(self);

    loop {

        int scene, shape, state;
        if (scanf("%d %d %d", &scene, &shape, &state) != 3) {
            break;
        }
        Entity entity = { scene, shape, state };
        process(entity);

    }

    return 0;

}