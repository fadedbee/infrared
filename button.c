// button.c

#include <stdint.h>
#include "button.h"
#include "util.h"
#include "control.h"

void button_pressed(uint32_t button) {
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
