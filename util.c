// util.c

#include <stdint.h>
#include <avr/io.h>
#include <util/delay.h>
#include "pins.h"

#ifdef DEBUG_SPI
void spi(uint8_t b) {
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
#endif

void panic(uint8_t header, uint8_t code) {
	for (int j = 0; j < 3; j++) {
		for (int i = 0; i < header; i++) {
			PORTB |= (1 << ERROR_PIN);
			_delay_ms(200);
			PORTB ^= (1 << ERROR_PIN);
			_delay_ms(300);
		}
		_delay_ms(500);
		for (int i = 0; i < code; i++) {
			PORTB |= (1 << ERROR_PIN);
			_delay_ms(200);
			PORTB ^= (1 << ERROR_PIN);
			_delay_ms(300);
		}
		_delay_ms(1000);
	}
}
