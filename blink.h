#include <avr/io.h>
#include <util/delay.h>

#define LED_PIN PB3

// Define the variadic macro to accept delay as argument
#define blink(...) int main(void) { \
    DDRB |= (1 << LED_PIN);          \
    while (1) {                      \
        PORTB |= (1 << LED_PIN);     \
        _delay_ms(__VA_ARGS__);      \
        PORTB &= ~(1 << LED_PIN);    \
        _delay_ms(__VA_ARGS__);      \
    }                                \
    return 0;                        \
}