#ifndef WIRE_H_STUB
#define WIRE_H_STUB
#include <stdint.h>
// Simule la sonde de presence I2C : un PCA marque absent ne repond pas (NACK).
struct TwoWire {
  void begin() {}
  void setClock(unsigned long) {}
  void beginTransmission(uint8_t address) { pending = address; }
  uint8_t endTransmission();
  uint8_t pending;
};
extern TwoWire Wire;
#endif
