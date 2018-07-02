// control.c

#include <stdint.h>
#include <stddef.h>
#include <avr/io.h>
#include "control.h"
#include "util.h"
#include "pins.h"

/*
 * NOTE: ticks are 63ms when idle, e.g. ZERO and VOLUPC 
 * or 100ms when a button is held-down, e.g. VOLDN.
 *
 * This could be fixed, but it would add a lot of complexity.
 */
control_state_t control_states[] = {
	//		ENTRY			UP_P	UP_R	DOWN_P	DOWN_R	SEL_P	SEL_R	TICKS	NEXT
	{ ZERO,		control_volume_down,	ZERO,	ZERO,	ZERO,	ZERO,	ZERO,	ZERO,	48,	IDLE	},	// 48/16 = 4.0s
	{ IDLE,		control_idle,		VOLUP,	IDLE,	VOLDN,	IDLE,	SELECT,	IDLE,	0,	ERR	},
	{ VOLUP,	control_volume_up,	ERR,	VOLUPC,	ERR,	ERR,	ERR,	ERR,	0,	ERR	},
	{ VOLUPC,	NULL,			VOLUP,	ERR,	VOLDN,	ERR,	SELECT,	ERR,	8,	IDLE	},	//  8/16 = 0.5s

	{ VOLDN,	control_volume_down,	ERR,	ERR,	ERR,	VOLDNC,	ERR,	ERR,	15,	ZERO	},	// 15/10 = 1.5s
	{ VOLDNC,	NULL,			VOLUP,	ERR,	VOLDN,	ERR,	SELECT,	ERR,	8,	IDLE	},	//  8/16 = 0.5s
	{ SELECT,	control_select,		ERR,	ERR,	ERR,	ERR,	ERR,	SELC,	1,	SELB	},
	{ SELB,		control_idle,		ERR,	ERR,	ERR,	ERR,	ERR,	IDLE,	0,	ERR	},

	{ SELC,		NULL,			VOLUP,	ERR,	VOLDN,	ERR,	SELECT,	ERR,	1,	IDLE	},
	{ ERR,		control_error,		ERR,	ERR,	ERR,	ERR,	ERR,	ERR,	1,	IDLE	},
};

control_state_t *control_state;

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
	panic(2, control_state->id);
}

void control_change_state(uint8_t state) {
	spi(control_state->id);
        control_state = &control_states[state];
	spi(control_state->id);
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

void control_tick_or_repeat(uint8_t ticks_or_repeats) {
	if (control_state->ticks_or_repeats > 0 && ticks_or_repeats > control_state->ticks_or_repeats) {
		control_change_state(control_state->next);
	}
}



