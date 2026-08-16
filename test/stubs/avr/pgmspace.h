#ifndef PGMSPACE_STUB_H
#define PGMSPACE_STUB_H
// Stub permettant de compiler NATIVEMENT le chemin de code AVR (PROGMEM).
// En memoire plate, lire "depuis la flash" revient a une lecture normale : cela ne
// reproduit pas la separation des espaces d'adressage, mais cela verifie que le code
// utilise bien les accesseurs et qu'il compile et se comporte correctement.
#include <stdint.h>
#include <string.h>
#define PROGMEM
#define pgm_read_byte(addr)  (*(const uint8_t *)(addr))
#define pgm_read_word(addr)  (*(const uint16_t *)(addr))
#define pgm_read_float(addr) (*(const float *)(addr))
#define memcpy_P memcpy
#endif
