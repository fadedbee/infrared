// main.c
// 
// Blinky for ATtiny85, with interrupt.

#include <stdint.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
 
void wait_init() {
	// set timer to use 
	TCCR0B = (1 << CS02 | 1 << CS00);
}

void wait(int delay) {
	TCNT0 = 255 - delay;
	for (;;) {
		if (TIFR & (1 << TOV0)) {	// timer timed out?
			TIFR |= (1 << TOV0);	// reset timer flag
			return;
		}
	}
}

int main(void)
{
	DDRB = (1 << PB3) | (1 << PB2) | (1 << PB1) | (1 << PB0);;
	wait_init();
	for (;;) {
		wait(100);
		PORTB ^= (1 << PB4);  // toggle pin PB2, on expiry of timer
	}
}


/*
ISR(PCINT0_vect)
{
	PORTB ^= (1 << PB3);  // toggle pin PB3, on logical change PCINT4 pin
}


ISR(TIM0_OVF_vect)
{
	PORTB ^= (1 << PB2);  // toggle pin PB2, on expiry of timer
}
int main (void)
{
	// init counter for ISR(TIM0_OVF_vect)
	//TCCR0B = (1 << CS02 | 1 << CS00);
	//TIMSK = (1 << TOIE0);

	// init match
	//TCCR0A = 0;      
	//OCR0A  = 0x10;      // number to count up to
	//TCCR0B = (1 << CS02 | 1 << CS00);

	// init pin interrupt
	//GIMSK |= (1 << PCIE);   // pin change interrupt enable
	//PCMSK |= (1 << PCINT4); // pin change interrupt enabled for PCINT4

	DDRB = (1 << PB3) | (1 << PB2) | (1 << PB1) | (1 << PB0);;
	for (uint64_t i = 0;; ++i) {
		int n = i % 2;
		// pin n high
		PORTB ^= 1 << n; 
		_delay_ms(1000);
		// all pins low
		PORTB ^= 1 << n;
		_delay_ms(1000);
	}
 
	return 1;
}
*/
