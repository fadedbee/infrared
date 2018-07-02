// util.h

#ifndef __UTIL_H
#define __UTIL_H

#include <stdint.h>

#ifdef DEBUG_SPI
void spi(uint8_t b);
#else
#define spi(x)
#endif
void panic(uint8_t header, uint8_t code);

#endif
