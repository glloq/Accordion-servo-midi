#ifndef ADA_PWM_STUB
#define ADA_PWM_STUB
#include <stdint.h>
class Adafruit_PWMServoDriver {
public:
  Adafruit_PWMServoDriver() {}
  Adafruit_PWMServoDriver(uint8_t) {}
  void begin() {}
  void setPWMFreq(float) {}
  void setPWM(uint8_t, uint16_t, uint16_t) {}
};
#endif
