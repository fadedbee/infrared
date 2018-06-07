// main.c

#include <stdint.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#define NULL	((void *)0)

void bitbang_spi(uint8_t b) {
	uint8_t tmp = PORTB;
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
	PORTB = tmp;
}

typedef struct control_state {
	uint8_t id;
	void (*entry)(void);
	uint8_t up_pressed;
	uint8_t up_released;
	uint8_t down_pressed;
	uint8_t down_released;
	uint8_t select_pressed;
	uint8_t select_released;
	uint8_t ticks;
	uint8_t next;
} control_state_t;

static control_state_t *control_state;

#define CLOCKWISE_PIN	PB2
#define WIDDERSHINS_PIN	PB1
#define SELECT_PIN	PB0
#define ERROR_PIN	PB3

void control_idle(void) {
	PORTB = 0;	
}

void control_volume_up(void) {
	PORTB = (1 << CLOCKWISE_PIN);	
}

void control_volume_down(void) {
	PORTB = (1 << WIDDERSHINS_PIN);	
}

void control_select(void) {
	PORTB = (1 << SELECT_PIN);	
}

void control_error(void) {
	PORTB = (1 << ERROR_PIN);	
}

#define ZERO	0
#define IDLE	1
#define VOLUP	2
#define VOLUPC	3
#define VOLDN	4
#define VOLDNC	5
#define SELECT	6
#define SELC	7
#define ERR	8

static control_state_t control_states[] = {
	//		ENTRY			UP_P	UP_R	DOWN_P	DOWN_R	SEL_P	SEL_R	TICKS	NEXT
	{ ZERO,		control_volume_down,	ZERO,	ZERO,	ZERO,	ZERO,	ZERO,	ZERO,	3*16,	IDLE	},
	{ IDLE,		control_idle,		VOLUP,	ERR,	VOLDN,	ERR,	SELECT,	ERR,	0,	ERR	},
	{ VOLUP,	control_volume_up,	ERR,	VOLUPC,	ERR,	ERR,	ERR,	ERR,	0,	ERR	},
	{ VOLUPC,	NULL,			ERR,	ERR,	VOLDN,	ERR,	SELECT,	ERR,	1*16,	IDLE	},
	{ VOLDN,	control_volume_down,	ERR,	ERR,	ERR,	VOLDNC,	ERR,	ERR,	4,	ZERO	},
	{ VOLDNC,	NULL,			ERR,	ERR,	VOLDN,	ERR,	SELECT,	ERR,	1*16,	IDLE	},
	{ SELECT,	control_select,		ERR,	ERR,	ERR,	ERR,	ERR,	SELC,	0,	ERR	},
	{ SELC,		NULL,			ERR,	ERR,	ERR,	ERR,	SELECT,	ERR,	1,	IDLE	},
	{ ERR,		control_error,		ERR,	ERR,	ERR,	ERR,	ERR,	ERR,	1,	IDLE	},
};

void control_change_state(uint8_t state) {
        control_state = &control_states[state];
        if (control_state->entry) {
                control_state->entry();
        }
}

void control_init(void) {
	control_change_state(ZERO);
}

void control_up_pressed(void) {
	control_change_state(control_state->up_pressed);
}

void control_up_released(void) {
	control_change_state(control_state->up_released);
}

void control_down_pressed(void) {
	control_change_state(control_state->down_pressed);
}

void control_down_released(void) {
	control_change_state(control_state->down_released);
}

void control_select_pressed(void) {
	control_change_state(control_state->select_pressed);
}

void control_select_released(void) {
	control_change_state(control_state->select_released);
}

void control_tick(uint8_t ticks) {
	if (control_state->ticks > 0 && ticks > control_state->ticks) {
		control_change_state(control_state->next);
	}
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
	if (button == BUTTON_UP) {
		control_up_pressed();
	}
	if (button == BUTTON_DOWN) {
		control_down_pressed();
	}
	if (button == BUTTON_SELECT) {
		control_select_pressed();
	}
	// ignore any other buttons
}

void button_released(uint32_t button, uint8_t repeats) {
	bitbang_spi(button >> 24);
	bitbang_spi(button >> 16);
	bitbang_spi(button >> 8);
	bitbang_spi(button >> 0);
	bitbang_spi(repeats);
	if (button == BUTTON_UP) {
		control_up_released();
	}
	if (button == BUTTON_DOWN) {
		control_down_released();
	}
	if (button == BUTTON_SELECT) {
		control_select_released();
	}
	// ignore any other buttons
}

void button_repeat(uint32_t button) {
	// nothing to do
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
	uint8_t ticks;
} packet_t;

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

void packet_tick(void) {
	bitbang_spi(120);
	bitbang_spi(packet.ticks);
	control_tick(packet.ticks);
	if (packet.ticks < 255) packet.ticks++;
} 

void packet_start(void) {
	packet.bits = 0;
	packet.button = BUTTON_NONE;
} 

void packet_repeat(void) {
	button_repeat(packet.button);
	control_tick(packet.repeats);
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

void packet_shift(uint8_t value);

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
}

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

void packet_init(void) {
	packet.state = &states[INIT];
	packet.repeats = 0;
	packet.ticks = 0;
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

	control_init();
	//packet_init();
	sei();		  // enable interrupts
	// FIXME: start clock

	for (uint64_t i = 0;; ++i) {
	}
 
	return 1;
}
