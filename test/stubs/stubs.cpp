#include <Arduino.h>
#include <Wire.h>
#include "harness.h"
#include <FlexyStepper.h>

unsigned long stubMicros = 0;
int stubPinState[32];

FlexyStepper *g_stepper = 0;
float stubPhysicalPos = 0.0f;
float stubMinSwitchPhys = -1e9f;
float stubMaxSwitchPhys =  1e9f;
bool  stubForceBothEndstops = false;

unsigned long stubStepCount = 0;
unsigned long stubIllegalPositionResets = 0;
bool stubInvertPhysicalDirection = false;
unsigned long stubI2cWrites = 0;
unsigned long stubI2cErrors = 0;
uint8_t stubMissingPcaMask = 0;

void stubResetMachine(float physicalStart, float minSwitch, float maxSwitch) {
    stubMicros = 1000000UL;      // Demarre a t = 1 s pour eviter les cas limites a 0
    stubPhysicalPos = physicalStart;
    stubMinSwitchPhys = minSwitch;
    stubMaxSwitchPhys = maxSwitch;
    stubForceBothEndstops = false;
    stubStepCount = 0;
    stubIllegalPositionResets = 0;
    stubInvertPhysicalDirection = false;
    stubI2cWrites = 0;
    stubI2cErrors = 0;
    stubMissingPcaMask = 0;
    for (int i = 0; i < 32; i++) stubPinState[i] = HIGH;
}

// ENABLE du driver : actif bas. Le firmware ecrit cette broche.
bool stubDriverEnabled() { return stubPinState[STEPPER_EN_PIN] == LOW; }

unsigned long micros() { return stubMicros; }
unsigned long millis() { return stubMicros / 1000UL; }
void delay(unsigned long ms) { stubMicros += ms * 1000UL; }

void pinMode(uint8_t, uint8_t) {}
void digitalWrite(uint8_t p, uint8_t v) { if (p < 32) stubPinState[p] = v; }

// Les fins de course sont DEDUITS de la position physique : ce sont des capteurs, pas des
// variables independantes. Ils restent donc coherents meme quand le compteur interne de la
// bibliotheque a derive (pas emis driver coupe, ou recalage de position).
int digitalRead(uint8_t p) {
    if (stubForceBothEndstops &&
        (p == LIMIT_SWITCH_MIN_PIN || p == LIMIT_SWITCH_MAX_PIN)) return LOW;
    if (p == LIMIT_SWITCH_MIN_PIN) return (stubPhysicalPos <= stubMinSwitchPhys) ? LOW : HIGH;
    if (p == LIMIT_SWITCH_MAX_PIN) return (stubPhysicalPos >= stubMaxSwitchPhys) ? LOW : HIGH;
    return HIGH;
}

long map(long x, long a, long b, long c, long d) { return (x - a) * (d - c) / (b - a) + c; }

SerialStub Serial;
SerialStub Serial1;
TwoWire Wire;
