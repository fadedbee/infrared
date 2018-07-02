// control.h

#ifndef __CONTROL_H
#define __CONTROL_H

#include <stdint.h>

typedef struct control_state {
	uint8_t id;
	void (*entry)(void);
	uint8_t up_pressed;
	uint8_t up_released;
	uint8_t down_pressed;
	uint8_t down_released;
	uint8_t select_pressed;
	uint8_t select_released;
	uint8_t ticks_or_repeats;
	uint8_t next;
} control_state_t;

void control_idle(void);
void control_volume_up(void);
void control_volume_down(void);
void control_select(void);
void control_error(void);
void control_change_state(uint8_t state);
void control_init(void);
void control_up_pressed(void);
void control_up_released(void);
void control_down_pressed(void);
void control_down_released(void);
void control_select_pressed(void);
void control_select_released(void);
void control_tick_or_repeat(uint8_t ticks_or_repeats);

#define ZERO	0
#define IDLE	1
#define VOLUP	2
#define VOLUPC	3
#define VOLDN	4
#define VOLDNC	5
#define SELECT	6
#define SELB	7
#define SELC	8
#define ERR	9

#endif
