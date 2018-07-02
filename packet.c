// packet.c

#include <stdint.h>
#include <stddef.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "packet.h"
#include "button.h"
#include "control.h" // FIXME: only needed for control_tick_or_repeat(), this should go via a button function
#include "util.h"
#include "pins.h"

static state_t states[] = {
	//			ENTRY				EARLY		SHORT		LONG		LATE	OVERFLOW
	{ INIT,		LOW,	packet_tick,			F_LOW,	255,	ERROR,	0,	ERROR,	0,	ERROR,	INIT },
	{ F_LOW,	HIGH,	packet_start,			ERROR,	18,	F_HIGH,	72,	ERROR,	255,	ERROR,	ERROR },
	{ F_HIGH,	LOW,	NULL,				ERROR,	6,	REPEAT,	13,	PRESS,	26,	ERROR,	ERROR },
	{ REPEAT,	HIGH,	packet_repeat,			ERROR,	0,	IDLE0,	4,	ERROR,	255,	ERROR,	ERROR },

	{ PRESS,	HIGH,	packet_release,			ERROR,	0,	PREBIT,	4,	ERROR,	255,	ERROR,	ERROR },
	{ PREBIT,	LOW,	NULL,				ERROR,	0,	BIT0,	4,	BIT1,	12,	ERROR,	ERROR },
	{ BIT0,		HIGH,   packet_shift_zero,		ERROR,	0,	PREBIT,	4,	ERROR,	255,	ERROR,	ERROR },
	{ BIT1,		HIGH,   packet_shift_one,		ERROR,	0,	PREBIT,	4,	ERROR,	255,	ERROR,	ERROR },

	{ IDLE0,	LOW,	NULL,				ERROR,	80,	F_LOW,	240,	ERROR,	255,	ERROR,	IDLE1 },
	{ IDLE1,	LOW,	NULL,				ERROR,	70,	F_LOW,	210,	IDLE2,	255,	ERROR,	IDLE2 },
	{ IDLE2,	LOW,	packet_release_start_and_tick,	F_LOW,	255,	ERROR,	0,	ERROR,	0,	ERROR,	IDLE2 },
	{ ERROR,	LOW,	packet_error,			F_LOW,	255,	ERROR,	0,	ERROR,	0,	ERROR,	INIT },
};

static packet_t packet;

void packet_tick(void) {
	control_tick_or_repeat(packet.ticks);
	if (packet.ticks < 255) packet.ticks++;
} 

void packet_start(void) {
	packet.bits = 0;
	packet.button = BUTTON_NONE;
} 

void packet_repeat(void) {
	button_repeat(packet.button);
	control_tick_or_repeat(packet.repeats);
	if (packet.repeats < 255) packet.repeats++;
} 

void packet_release(void) {
	if (packet.button_to_release) {
		button_released(packet.button_to_release, packet.repeats);
		packet.button_to_release = BUTTON_NONE;
		packet.repeats = 0;
		packet.ticks = 0;
	}
}

void packet_shift_zero(void) {
	packet_shift(0);
}

void packet_shift_one(void) {
	packet_shift(1);
}

void packet_release_start_and_tick(void) {
	packet_release();
	packet_start();
	packet_tick();
}

void packet_error(void) {
	packet_release(); 	// this avoids a bug where a press is read but
				// the release is missed because of a protocol error
}

void packet_init(void) {
	packet.state = &states[INIT];
	packet.repeats = 0;
	packet.ticks = 0;
	packet_start();
} 

void packet_change_state(uint8_t state) {
	packet.state = &states[state];
	if (packet.state->entry) {
		packet.state->entry();
	}
}

void packet_shift(uint8_t value) {
	// least significant bit first
	packet.button = (packet.button >> 1);
	if (value) packet.button |= 0x80000000;
	if (++packet.bits == 32) {
		button_pressed(packet.button);
		packet.button_to_release = packet.button;
		packet_change_state(IDLE0);
	}
}

ISR(PCINT0_vect)
{
	uint8_t delay = TCNT0;
	TCNT0 = 0;

	for (int i = 0; i < DEBOUNCE_READS; i++) {
		if (packet.state->value) { // looking for a consistent +ve edge 
			if (!(PINB & (1 << PB4))) return;
		} else { // looking for a -ve edge
			if ((PINB & (1 << PB4))) return;
		}
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
	packet_change_state(packet.state->overflow_next);
}

