#include <stdbool.h>
#include <stdint.h>

#ifdef __AVR__
    #include <avr/io.h>
    #include <avr/interrupt.h>
    #include <avr/sleep.h>
    #include <util/delay.h>
    #define wait sleep_mode()
#else
    #include <stdio.h>
    #include <string.h>
    #include <signal.h> 
    #define wait pause()
    #ifdef _WIN32
        #include <windows.h>
        #define write(ptr, size) fwrite(ptr, size, 1, stdout)
        #define read(ptr, size)  fread(ptr, 1, size, stdin)
    #else
        #include <unistd.h>
        #define write(ptr, size) write(STDOUT_FILENO, ptr, size)
        #define read(ptr, size)  read(STDIN_FILENO, ptr, size)
    #endif
#endif

#define until(cond) while (!(cond))
#define loop for(;;)

#pragma pack(push, 1)
typedef struct {
    uint8_t id      : 8; 
    uint8_t state   : 8;
} Automata;
#pragma pack(pop)

#define COUNT(...) (sizeof((Automata[]){ __VA_ARGS__ }) / sizeof(Automata))
#define AUTOMATAS(...) Automata automatas[COUNT(__VA_ARGS__)] = { __VA_ARGS__ }

static inline void print(Automata a) {
    printf("ID: %d, STATE: %d\n",  a.id, a.state); 
}

static inline void prints(Automata a[]) {
    for (int i = 0; i < 4; i++) {
        print(a[i]);
    }
}

static inline void out(Automata a) {
    write(&a, 2);
    fflush(stdout);
}

static inline void in(Automata* a) {
    until(read(a, sizeof(*a)) == 2);
}

volatile bool running = true;

// SIGUSR1, SIGUSR2

void interrupt(int sig) { 
    printf("Caught signal %d\n", sig); 
    running = false;
} 

int main(int argc, char *argv[]) {

    signal(SIGINT, interrupt); 

    loop {

        wait;
        raise(SIGINT);
        printf("Breaking\n");
        break;

    }

}

    /*

    printf("You have entered %d arguments:\n", argc);
 
    for (int i = 0; i < argc; i++) {
        printf("%s\n", argv[i]);
    }

    if (argc >= 2 && strcmp(argv[1], "debug") == 0) {
          printf("DEBUGGING MODE ON\n");
    }

    AUTOMATAS(
        { 0, 0 },
        { 1, 0 },
        { 2, 2 },
        { 3, 1 }
    );

    printf("size == %zu \n" , sizeof(Automata) * 8);

    prints(automatas);*/