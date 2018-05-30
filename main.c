// main.c
// 
// Blinky for ATtiny85, with interrupt.

#include <stdint.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
 
ISR(PCINT0_vect)
{
	PORTB ^= (1 << PB3);  // toggle pins PB0 and PB2, on logical change PCINT4 pin
}

int main (void)
{
	GIMSK |= (1 << PCIE);   // pin change interrupt enable
	PCMSK |= (1 << PCINT4); // pin change interrupt enabled for PCINT4
	sei();                  // enable interrupts

	DDRB = (1 << PB3) | (1 << PB2) | (1 << PB1) | (1 << PB0);;
	for (uint64_t i = 0;; ++i) {
		int n = i % 3;
		// pin n high
		PORTB ^= 1 << n; 
		_delay_ms(1000);
		// all pins low
		PORTB ^= 1 << n;
		_delay_ms(1000);
	}
 
	return 1;
}
