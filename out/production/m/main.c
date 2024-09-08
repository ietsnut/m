#include "process.h"

static Entity self = { 0, 1, 1, 1 };
static Entity other;

program {

    start();

    //printf("sizeof(Entity) == %zu \n" , sizeof(Entity));

    loop {

        in(&other);
        self.state += other.state;
        out(self);

    }

    return 0;

}
