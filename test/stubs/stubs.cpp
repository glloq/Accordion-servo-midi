#include <Arduino.h>
#include <Wire.h>
#include "harness.h"
#include <FlexyStepper.h>

unsigned long stubClock = 0;
int  stubPinState[32];
FlexyStepper *g_stepper = 0;
float stubMinSwitchPos = -1e9f;
float stubMaxSwitchPos =  1e9f;
bool stubForceBothEndstops = false;

unsigned long millis() { return stubClock; }
void delay(unsigned long ms) { stubClock += ms; }
void pinMode(uint8_t, uint8_t) {}
void digitalWrite(uint8_t p, uint8_t v) { if (p < 32) stubPinState[p] = v; }

int digitalRead(uint8_t p) {
  if (stubForceBothEndstops) {
    if (p == LIMIT_SWITCH_MIN_PIN || p == LIMIT_SWITCH_MAX_PIN) return LOW;
  }
  if (!g_stepper) return HIGH;
  if (p == LIMIT_SWITCH_MIN_PIN) return (g_stepper->pos <= stubMinSwitchPos) ? LOW : HIGH;
  if (p == LIMIT_SWITCH_MAX_PIN) return (g_stepper->pos >= stubMaxSwitchPos) ? LOW : HIGH;
  return HIGH;
}
long map(long x, long a, long b, long c, long d) { return (x-a)*(d-c)/(b-a)+c; }
SerialStub Serial;
SerialStub Serial1;
TwoWire Wire;
