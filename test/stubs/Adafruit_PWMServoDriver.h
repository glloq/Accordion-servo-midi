#ifndef ADA_PWM_STUB
#define ADA_PWM_STUB
#include <stdint.h>
// setPWM() renvoie le code d'erreur I2C, comme la version 3.x de la bibliotheque.
// Chaque ecriture consomme du temps simule : c'est ce cout qui rogne la frequence de pas.
class Adafruit_PWMServoDriver {
public:
  Adafruit_PWMServoDriver() : addr(0) {}
  Adafruit_PWMServoDriver(uint8_t a) : addr(a) {}
  void begin() {}
  void setPWMFreq(float) {}
  uint8_t setPWM(uint8_t, uint16_t, uint16_t);
  uint8_t addr;
};
#endif
