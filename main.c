// main.c

#include <stdint.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
 

void bitbang_spi(uint8_t b) {
	PORTB = 4;
	for (int i = 0; i < 8; i++, b = b << 1) {
		if (b & 0x80) {
			PORTB = 1;
			PORTB = 3;
		} else {
			PORTB = 0;
			PORTB = 2;
		}
	}
	PORTB = 4;
}

#define BUTTON_NONE 0x00000000
#define BUTTON_UP 0xfe01ef10 //0x10ef01fe
#define BUTTON_DOWN 0xff00ef10 //0x10ef00ff
#define BUTTON_SELECT 0xfb04ef10 //0x10ef04fb

void button_pressed(uint32_t button) {
	bitbang_spi(button >> 24);
	bitbang_spi(button >> 16);
	bitbang_spi(button >> 8);
	bitbang_spi(button >> 0);
}

void button_released(uint32_t button, uint8_t repeats) {
	bitbang_spi(button >> 24);
	bitbang_spi(button >> 16);
	bitbang_spi(button >> 8);
	bitbang_spi(button >> 0);
	bitbang_spi(repeats);
}

typedef struct state {
	uint8_t id;
	uint8_t value;
	void (*entry)(void);
	uint8_t early_next;
	uint8_t early_time;
	uint8_t short_next;
	uint8_t short_time;
	uint8_t long_next;
	uint8_t long_time;
	uint8_t late_next;
	uint8_t overflow_next;
} state_t;

//static uint64_t ticks;

typedef struct packet {
	state_t *state;
//	uint64_t state_started;
	uint8_t bits;
	uint32_t button;
	uint32_t button_to_release;
	uint8_t repeats;
} packet_t;

#define NULL	((void *)0)

#define LOW	0
#define HIGH	1

#define INIT	0
#define F_LOW	1
#define F_HIGH	2
#define REPEAT	3
#define PRESS	4
#define PREBIT	5
#define BIT0	6
#define BIT1	7
#define IDLE0	8
#define IDLE1	9
#define IDLE2	10
#define ERROR	11

static packet_t packet;

void packet_start(void) {
	packet.bits = 0;
	packet.button = BUTTON_NONE;
} 

void packet_repeat(void) {
	bitbang_spi(120);
	if (packet.repeats < 255) packet.repeats++;
	bitbang_spi(packet.repeats);
} 

void packet_release(void) {
	if (packet.button_to_release) {
		button_released(packet.button_to_release, packet.repeats);
		packet.button_to_release = BUTTON_NONE;
		packet.repeats = 0;
	}
}

void packet_shift(uint8_t value);

void packet_shift_zero(void) {
	packet_shift(0);
}

void packet_shift_one(void) {
	packet_shift(1);
}

void packet_release_and_start(void) {
	packet_release();
	packet_start();
}

void packet_error(void) {
}

static state_t states[] = {
	//			ENTRY				EARLY		SHORT		LONG		LATE	OVERFLOW
	{ INIT,		LOW,	NULL,				F_LOW,	255,	ERROR,	0,	ERROR,	0,	ERROR,	INIT },
	{ F_LOW,	HIGH,	packet_start,			ERROR,	18,	F_HIGH,	72,	ERROR,	255,	ERROR,	ERROR },
	{ F_HIGH,	LOW,	NULL,				ERROR,	6,	REPEAT,	13,	PRESS,	26,	ERROR,	ERROR },
	{ REPEAT,	HIGH,	packet_repeat,			ERROR,	0,	IDLE0,	4,	ERROR,	255,	ERROR,	ERROR },

	{ PRESS,	HIGH,	packet_release,			ERROR,	0,	PREBIT,	4,	ERROR,	255,	ERROR,	ERROR },
	{ PREBIT,	LOW,	NULL,				ERROR,	0,	BIT0,	4,	BIT1,	12,	ERROR,	ERROR },
	{ BIT0,		HIGH,   packet_shift_zero,		ERROR,	0,	PREBIT,	4,	ERROR,	255,	ERROR,	ERROR },
	{ BIT1,		HIGH,   packet_shift_one,		ERROR,	0,	PREBIT,	4,	ERROR,	255,	ERROR,	ERROR },

	{ IDLE0,	LOW,	NULL,				ERROR,	80,	F_LOW,	240,	ERROR,	255,	ERROR,	IDLE1 },
	{ IDLE1,	LOW,	NULL,				ERROR,	70,	F_LOW,	210,	IDLE2,	255,	ERROR,	IDLE2 },
	{ IDLE2,	LOW,	packet_release_and_start,	F_LOW,	255,	ERROR,	0,	ERROR,	0,	ERROR,	IDLE2 },
	{ ERROR,	LOW,	packet_error,			F_LOW,	255,	ERROR,	0,	ERROR,	0,	ERROR,	INIT },
};

void packet_init(void) {
	packet.state = &states[INIT];
	packet.repeats = 0;
	//packet.state_started = ticks | TCNT0;
	packet_start();
} 

void packet_change_state(uint8_t state) {
	//cei();
	//TCNT0 = 0;
	packet.state = &states[state];
	/*
	uint8_t id = packet.state->id;
	for (int i = 0, b = id; i < 8; i++, b = b << 1) {
		if (b & 0x80) {
			PORTB = 1;
			PORTB = 3;
		} else {
			PORTB = 0;
			PORTB = 2;
		}
	}
	PORTB = 4;
	*/

	if (packet.state->entry) {
		packet.state->entry();
	}
	//sei();
	// FIXME: start clock
}

void packet_shift(uint8_t value) {
	// least significant bit first
	packet.button = (packet.button >> 1);
	if (value) packet.button |= 0x80000000;
	/*
	for (int i = 0, b = packet.bits; i < 8; i++, b = b << 1) {
		if (b & 0x80) { 
			PORTB = 1;
			PORTB = 3;
		} else {
			PORTB = 0;
			PORTB = 2;
		}
	}
	PORTB = 4;
	*/

	if (++packet.bits == 32) {
		button_pressed(packet.button);
		packet.button_to_release = packet.button;
		packet_change_state(IDLE0);
	}

}

ISR(PCINT0_vect)
{
	// FIXME: stop clock
	uint8_t delay = TCNT0;
	TCNT0 = 0;
	// output length of delay via bit-banged SPI for debug
	/*
	for (int i = 0, b = delay; i < 8; i++, b = b << 1) {
		if (b & 0x80) {
			PORTB = 1;
			PORTB = 3;
		} else {
			PORTB = 0;
			PORTB = 2;
		}
	}
	PORTB = 4;
	*/

	if (packet.state->value) { // looking for a +ve edge 
		if (!(PINB & (1 << PB4))) return;
	} else { // looking for a -ve edge
		if ((PINB & (1 << PB4))) return;
	}

	if (delay <= packet.state->early_time) {
		packet_change_state(packet.state->early_next);
		return;
	}
	if (delay <= packet.state->short_time) {
		packet_change_state(packet.state->short_next);
		return;
	}
	if (delay <= packet.state->long_time) {
		packet_change_state(packet.state->long_next);
		return;
	}
	packet_change_state(packet.state->late_next);
}

ISR(TIM0_OVF_vect)
{
	//PORTB ^= (1 << PB2);  // toggle pin PB2, on expiry of timer
	packet_change_state(packet.state->overflow_next);

	//ticks += 256;
	//uint64_t now = ticks + TCNT0;
	//delay = now - packet.state_start
	//if (delay >= packet.state->long_time) {
	//	packet_change_state(packet.state->late_next);
	//}
}

int main (void)
{
	// init outputs
	DDRB = (1 << PB3) | (1 << PB2) | (1 << PB1) | (1 << PB0);;

	// init counter
	TCCR0B = (1 << CS02); // clock freq is 256us.
	TIMSK = (1 << TOIE0);
	// FIXME: stop clock
	TCNT0 = 0;

	// init pin interrupt
	GIMSK |= (1 << PCIE);   // pin change interrupt enable
	PCMSK |= (1 << PCINT4); // pin change interrupt enabled for PCINT4

	//packet_init();
	sei();		  // enable interrupts
	// FIXME: start clock

	for (uint64_t i = 0;; ++i) {
	}
 
	return 1;
}
