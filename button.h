// button.h

#ifndef __BUTTON_H
#define __BUTTON_H

#include <stdint.h>

#define BUTTON_NONE 0x00000000
#define BUTTON_UP 0xfe01ef10 //0x10ef01fe
#define BUTTON_DOWN 0xff00ef10 //0x10ef00ff
#define BUTTON_SELECT 0xfb04ef10 //0x10ef04fb

void button_pressed(uint32_t button);
void button_released(uint32_t button, uint8_t repeats);
void button_repeat(uint32_t button);

#endif
