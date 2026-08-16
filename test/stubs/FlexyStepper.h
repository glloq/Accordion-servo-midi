#ifndef FLEXY_STUB
#define FLEXY_STUB
#include <stdint.h>
#include <Arduino.h>
// Stub simulant reellement le mouvement et la position physique de la machine.
// Les fins de course sont deduits de la position (voir stubs.cpp), comme sur la vraie
// machine : ce sont des capteurs physiques, pas des variables independantes.
class FlexyStepper;
extern FlexyStepper *g_stepper;
extern float stubMinSwitchPos; // Coordonnee moteur ou le contact MIN se ferme
extern float stubMaxSwitchPos; // Coordonnee moteur ou le contact MAX se ferme

class FlexyStepper {
public:
  FlexyStepper() : pos(0), target(0), speed(1), lastMs(0) { g_stepper = this; }
  void connectToPins(uint8_t, uint8_t) {}
  void setStepsPerMillimeter(float) {}
  void setSpeedInMillimetersPerSecond(float s) { speed = s; }
  void setAccelerationInMillimetersPerSecondPerSecond(float) {}
  void setTargetPositionInMillimeters(float t) { target = t; }
  // Recaler la coordonnee moteur ne deplace pas les capteurs physiques :
  // leurs coordonnees se decalent d'autant.
  void setCurrentPositionInMillimeters(float p) {
    float delta = p - pos;
    stubMinSwitchPos += delta;
    stubMaxSwitchPos += delta;
    pos = p;
  }
  float getCurrentPositionInMillimeters() { return pos; }
  bool processMovement() {
    unsigned long now = millis();
    float dt = (now - lastMs) / 1000.0f;
    lastMs = now;
    if (dt <= 0) return pos == target;
    float step = speed * dt;
    if (pos < target) { pos += step; if (pos > target) pos = target; }
    else if (pos > target) { pos -= step; if (pos < target) pos = target; }
    return pos == target;
  }
  bool motionComplete() { return pos == target; }
  float pos, target, speed;
  unsigned long lastMs;
};
#endif
