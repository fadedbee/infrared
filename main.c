// main.c
// 
// Blinky for ATtiny85, with interrupt.

#include <stdint.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
 
static uint8_t debounce = 0x55; // 01010101 pattern

void error() {
	for (;;) {
		_delay_ms(500);
		PORTB ^= 1 << PB3; 
	}
}

void expire(int delay_qms) {
	if (delay_qms <= 0) return;
	if (delay_qms < 256) {
		TCCR0B = (1 << CS02); // F_CPU/256, tick = 256us, quarter of a ms, qms
		TCNT0 = 256 - delay_qms;	// just count the remainder
	} else if (delay_qms < 1024) {
		TCCR0B = (1 << CS02) | (1 << CS00); // F_CPU/1024, tick = 1024
		TCNT0 = 256 - (delay_qms / 4);	// just count the remainder
	} else {
		error();
	}

	TIFR |= (1 << TOV0);	// reset timer flag
	for (;;) {
		if (TIFR & (1 << TOV0)) return;	// timer timed out
	}
}

int wait_or_expire(int value, int delay_qms) {
	if (delay_qms <= 0) return 0;
	if (delay_qms < 256) {
		TCCR0B = (1 << CS02); // F_CPU/256, tick = 256us, quarter of a ms, qms
		TCNT0 = 256 - delay_qms;	// just count the remainder
	} else if (delay_qms < 1024) {
		TCCR0B = (1 << CS02) | (1 << CS00); // F_CPU/1024, tick = 1024
		TCNT0 = 256 - (delay_qms / 4);	// just count the remainder
	} else {
		error();
	}

	TIFR |= (1 << TOV0);	// reset timer flag
	for (;;) {
		// push value of pinb5 into low bit of debounce
		debounce = (debounce << 1) | ((PINB & (1 << PB4)) >> PB4);

		// FIXME: timeout should return the exact delay requested - the code below would return a very small delay on timeout as TCNT will have wrapped (by definition)
		if ((value == 0 && debounce == 0) || 	// low needed and found
		    (value != 0 && debounce == 0xFF) ||	// high needed and found
		    (TIFR & (1 << TOV0))) {		// timer timed out
			// return how long it took
			if (TCCR0B == (1 << CS02)) {
				return 256 - TCNT0;
			} else {
				return (256 - TCNT0) * 4;
			}
		}
	}
}

void wait(int value) {
	for (;;) {
		// push value of pinb5 into low bit of debounce
		debounce = (debounce << 1) | ((PINB & (1 << PB4)) >> PB4);

		// if the debounced pin matches the value requested
		if ((value == 0 && debounce == 0) || (value != 0 && debounce == 0xFF)) {
			// return how long it took
			// FIXME: check this formula
			return;
		}
	}
}

int main(void)
{
	DDRB = (1 << PB3) | (1 << PB2) | (1 << PB1) | (1 << PB0);;
	sei();                  // enable interrupts
	for (;;) {
		wait_or_expire(0, 1000);
		PORTB ^= 1 << PB2; 
		/*
		expire(40);
		PORTB ^= 1 << PB2; 
		expire(40);
		PORTB ^= 1 << PB2; 
		for (int i = 254; i < 270; i++) {
			expire(i);
			PORTB ^= 1 << PB2; 
		}
		*/
	}
	return 1;
}
