#include "m.h"

static Entity self;
static Entity other;

program {

    if (read(&self, sizeof(self)) != sizeof(Entity)) {
        return 0;
    }

    out(self);

    loop {

        if (read(&other, sizeof(other)) != sizeof(Entity)) {
            break;
        }

        self.state += other.state;

        out(self);

    }

    return 0;

}