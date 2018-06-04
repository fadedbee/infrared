// main.c

#include <stdint.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
 
// millisecond times in units of 256usec
#define MS_0_25	1
#define MS_1	4
#define MS_1_25	5
#define MS_3	12
#define MS_3_5	14
#define MS_5_5	22
#define MS_13_5	54
#define MS_20	80
#define MS_68	272


#define MAX_DEBOUNCE 0xFF
static uint8_t debounce = 0x55; // 0101... pattern

#define BUTTON_NONE 0x00000000
#define BUTTON_UP 0x10ef01fe
#define BUTTON_DOWN 0x10ef00ff
#define BUTTON_SELECT 0x10ef04fb


int error(int flashes) {
	// repeat the flash pattern three times
	for (int j = 0; j < 3; j++) { 
		int maxi = flashes * 2;
		for (int i = 0; i < maxi; i++) {
			PORTB ^= 1 << PB3; 
			_delay_ms(200);
		}
		_delay_ms(600);
	}
	return flashes;
}

int fast_error(int flashes) {
	int maxi = flashes * 2;
	for (int i = 0; i < maxi; i++) {
		PORTB ^= 1 << PB3; 
		if (i % 20 == 0) {
			_delay_ms(1);
		}
	}
	return flashes;
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
		error(10);
	}

	TIFR |= (1 << TOV0);	// reset timer flag
	for (;;) {
		// push value of pinb5 into low bit of debounce
		debounce = (debounce << 1) | ((PINB & (1 << PB4)) >> PB4);

		if (TIFR & (1 << TOV0)) return;	// timer timed out
	}
}

int wait_or_expire(int value, int delay_qms) {
	if (delay_qms <= 0) return 0;
	if (delay_qms < 256) {
		TCCR0B = (1 << CS02); // F_CPU/256, tick = 256us, quarter of a ms, qms
		TCNT0 = 256 - delay_qms;	// just count the remainder
	} else if (delay_qms < 1024) {
		TCCR0B = (1 << CS02) | (1 << CS00); // F_CPU/1024, tick = 1.024ms
		TCNT0 = 256 - (delay_qms / 4);	// just count the remainder
	} else {
		error(11);
	}

	TIFR |= (1 << TOV0);	// reset timer flag
	for (;;) {
		// push value of pinb5 into low bit of debounce
		debounce = (debounce << 1) | ((PINB & (1 << PB4)) >> PB4);
		PORTB ^= (1 << PB0);

		if (TIFR & (1 << TOV0)) return delay_qms;	// timer timed out

		if ((value == 0 && debounce == 0) || 		// low needed and found
		    (value != 0 && debounce == MAX_DEBOUNCE)) {	// high needed and found
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

		if ((value == 0 && debounce == 0) || (value != 0 && debounce == MAX_DEBOUNCE)) {
			// return how long it took
			// FIXME: check this formula
			return;
		}
	}
}

static uint32_t button = 0;
static uint8_t press_time;

int main_loop() {
	for (;;) {
		int delay;

		delay = wait_or_expire(1, MS_13_5);	// expect high in 9 ms (4.5 .. 13.5)
		//if (delay < MS_5_5) return error(2);
		if (delay >= MS_13_5) return error(3);

		delay = wait_or_expire(0, MS_3);	// expect low in 2.25 ms (1.2 .. 3)
		if (delay < MS_1_25) return error(4);
		if (delay >= MS_3) return error(5);
		if (delay > MS_3) {
			button = 0;
			for (int bit = 0; bit < 32; bit++) {
				delay = wait_or_expire(1, MS_1_25);	// expect high in 0.56 ms (0.25 .. 1.25)
				if (delay < MS_0_25) return error(6);	// too soon

				delay = wait_or_expire(0, MS_3_5);	// expect low in 0.56 ms or 1.68 ms
				if (delay < MS_0_25) return error(7);
				if (delay >= MS_3_5) return error(8);
				if (delay < MS_1) {		// a zero
					// nothing to do, bits are already cleared
				} else {			// a one
					button |= (1 << bit);
				}
			}

			switch (button) {
				case BUTTON_UP:
					PORTB ^= (1 << PB0);
					break;
				case BUTTON_DOWN:
					PORTB ^= (1 << PB1);
					break;
				case BUTTON_SELECT:
					PORTB ^= (1 << PB2);
					break;
				default:
					PORTB ^= 7;
			}

		} // else it was "repeat", between 12 and 30 ms
		press_time++;

		// expect high (stop bit)
		wait(1);

		delay = wait_or_expire(0, MS_68); 	// expect low in 43ms (20 .. 68)
		if (delay < MS_20) return error(9);
		if (delay < MS_68) return 0;		// next signal is following
		// else, the button has been released

		if (button == BUTTON_DOWN && press_time > 10) {
			//volume_zero();
		}
		//volume_stop();

		button = BUTTON_NONE;
		press_time = 0;
		//released()
	}
	return 1;
}

int main(void)
{

	DDRB = (1 << PB3) | (1 << PB2) | (1 << PB1) | (1 << PB0);;

	for (;;) {
		//wait(0); // for low
		//main_loop();
		wait(1); // for high
		int delay = wait_or_expire(0, 100);
		//if (delay < 1) PORTB ^= (1 << PB0); 
		if (delay < 100) {
			PORTB ^= (1 << PB1);
			fast_error(delay);
		}
		if (delay >= 100) PORTB ^= (1 << PB2);
	}
}
