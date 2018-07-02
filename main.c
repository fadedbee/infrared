// main.c

#include <stdint.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include "control.h"
#include "packet.h"

int main(void)
{
	// init outputs
	DDRB = (1 << PB3) | (1 << PB2) | (1 << PB1) | (1 << PB0);;

	// init counter
	TCCR0B = (1 << CS02); // clock freq is 256us.
	TIMSK = (1 << TOIE0);
	TCNT0 = 0;

	// init pin interrupt
	GIMSK |= (1 << PCIE);   // pin change interrupt enable
	PCMSK |= (1 << PCINT4); // pin change interrupt enabled for PCINT4

	control_init();
	packet_init();
	sei();		  // enable interrupts

	for (;;) {
	}
 
	return 1;
}
