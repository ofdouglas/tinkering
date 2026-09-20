#include <avr/io.h>
#include <util/delay.h>


#ifndef F_CPU
#error F_CPU not defined
#endif


int main(void) {
    DDRD |= (1 << PD6);
    while (1) {
        PORTD |= (1 << PD6);
        _delay_ms(1000);
        PORTD &= ~(1 << PD6);
        _delay_ms(1000);
    }
    return 0;
}