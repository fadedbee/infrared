// packet.h

#ifndef __PACKET_H
#define __PACKET_H

#include <stdint.h>

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

typedef struct packet {
	state_t *state;
	uint8_t bits;
	uint32_t button;
	uint32_t button_to_release;
	uint8_t repeats;
	uint8_t ticks;
} packet_t;

#define DEBOUNCE_READS 4

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

void packet_tick(void);
void packet_start(void);
void packet_repeat(void);
void packet_release(void);
void packet_shift(uint8_t value);
void packet_shift_zero(void);
void packet_shift_one(void);
void packet_release_start_and_tick(void);
void packet_error(void);
void packet_init(void);
void packet_change_state(uint8_t state);

#endif
