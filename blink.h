#include <avr/io.h>
#include <util/delay.h>

#define LED_PIN PB3
#define DELAY_TIME 1000  // 500ms delay

#define led int main(void){blink();}

int blink(void)
{
    DDRB |= (1 << LED_PIN);  // Set PB3 as output

    while (1)
    {
        PORTB |= (1 << LED_PIN);   // Turn LED on
        _delay_ms(DELAY_TIME);     // Wait

        PORTB &= ~(1 << LED_PIN);  // Turn LED off
        _delay_ms(DELAY_TIME);     // Wait
    }
    return 0;

}