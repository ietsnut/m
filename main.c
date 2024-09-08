#include "m.h"

static Entity self  = { 0, 1, 1, 1 };
static Entity other;

program {

    #ifdef _WIN32
        _setmode(_fileno(stdout), _O_BINARY);
    #elif __APPLE__ || __linux__
        int flags = fcntl(STDOUT_FILENO, F_GETFL);
        fcntl(STDOUT_FILENO, F_SETFL, flags | O_BINARY);
    #endif

    //printf("sizeof(Entity) == %zu \n" , sizeof(Entity));

    in(&self);
    out(self);

    loop {

        in(&other);
        self.state += other.state;
    
        out(self);

    }

    return 0;

}
