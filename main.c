#include <stdint.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
 
int fast_error(int flashes) {
        for (int i = 0; i < flashes; i++) {
                PORTB ^= 1 << PB3;
                if (i % 10 == 0) {
                        _delay_ms(1);
                }
        }
        return flashes;
}

void pos_edge() {
	PORTB ^= (1 << PB2);
}

void neg_edge() {
	PORTB ^= (1 << PB1);
}

void expire() {
	PORTB ^= (1 << PB0);
}

ISR(PCINT0_vect)
{
	if (PORTB & (1 << PB4)) {
		pos_edge();
	} else {
		neg_edge();
	}
}


ISR(TIM0_OVF_vect)
{
	PORTB ^= (1 << PB2);  // toggle pin PB2, on expiry of timer
	expire();
}



int main (void)
{
	// init pin interrupt
	GIMSK |= (1 << PCIE);   // pin change interrupt enable
	PCMSK |= (1 << PCINT4); // pin change interrupt enabled for PCINT4

	// init counter
	TIMSK = (1 << TOIE0);
	TCCR0B = (1 << CS02); // F_CPU/256, tick = 256us, quarter of a ms, qms
	TCNT0 = 256 - 255;        // just count the remainder

	sei();                  // enable interrupts

	DDRB = (1 << PB3) | (1 << PB2) | (1 << PB1) | (1 << PB0);;
	for (uint64_t i = 0;; ++i) {
	}
 
	return 1;
}
