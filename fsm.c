#include <stdio.h>
#include "lib/automata.h"

INITIAL(Locked)

void trigger_unlock(void) { printf("Unlocking turnstile\n"); }
void trigger_alert(void) {  printf("Alert! Attempted pass through locked turnstile\n"); }
void thank_user(void) {     printf("Thank you for inserting coin\n"); }
void trigger_lock(void) {   printf("Locking turnstile\n"); }

TRANSITION(Locked,  Coin, Unlocked, trigger_unlock())
TRANSITION(Locked, Pass, Locked,    trigger_alert())
TRANSITION(Unlocked, Coin, Unlocked, thank_user())
TRANSITION(Unlocked, Pass, Locked, trigger_lock())

int main() {

    INIT();
    
    EVENT(Coin);
    EVENT(Pass);
    EVENT(Pass);
    EVENT(Coin);
    EVENT(Coin);
    
    return 0;

}