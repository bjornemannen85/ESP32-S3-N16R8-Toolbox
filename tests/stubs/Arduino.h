#pragma once
#include <cstdint>
#include <cstddef>
#define PROGMEM
#define pgm_read_byte(p) (*(const uint8_t *)(p))
#define pgm_read_word(p) (*(const uint16_t *)(p))
inline unsigned long testDelay=0;
inline void delay(unsigned long ms){testDelay+=ms;}

inline unsigned long millis(){return testDelay;}
