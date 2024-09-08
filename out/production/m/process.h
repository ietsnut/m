#pragma once

#include "m.h"

#include <fcntl.h>

#ifdef _WIN32
    #include <windows.h>
    #define write(ptr, size) fwrite(ptr, size, 1, stdout)
    #define read(ptr, size)  fread(ptr, 1, size, stdin)
#else
    #include <unistd.h>
    #define write(ptr, size) write(STDOUT_FILENO, ptr, size)
    #define read(ptr, size)  read(STDIN_FILENO, ptr, size)
#endif

static inline void start(void) {
    #ifdef _WIN32
        _setmode(_fileno(stdout), _O_BINARY);
    #else
        int flags = fcntl(STDOUT_FILENO, F_GETFL);
        fcntl(STDOUT_FILENO, F_SETFL, flags | O_BINARY);
    #endif
}

static inline void out(Entity entity) {
    write(&entity, 4);
    fflush(stdout);
}

static inline void in(Entity* in) {
    until (read(in, sizeof(*in)) == sizeof(Entity) && in->start == 0);
}