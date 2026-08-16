#ifndef HARNESS_H
#define HARNESS_H

// Etat de la machine simulee, partage entre les stubs et les tests.
#include "settings.h"

class FlexyStepper;

// Horloge simulee, en MICROsecondes. millis() en derive.
extern unsigned long stubMicros;

// Etat des broches ecrites par le firmware (index = numero de broche).
extern int stubPinState[32];

// Machine simulee
extern FlexyStepper *g_stepper;
extern float stubPhysicalPos;    // Position physique reelle du soufflet (mm)
extern float stubMinSwitchPhys;  // Position physique du contact bas (mm)
extern float stubMaxSwitchPhys;  // Position physique du contact haut (mm)
extern bool  stubForceBothEndstops; // Force les deux contacts (test de cablage)

// Instrumentation
extern unsigned long stubStepCount;   // Nombre total de pas emis
extern unsigned long stubIllegalPositionResets; // setCurrentPosition() appele en mouvement
extern bool stubInvertPhysicalDirection;        // Simule un cablage DIR inverse
extern unsigned long stubI2cWrites;   // Nombre d'ecritures I2C
extern unsigned long stubI2cErrors;   // Erreurs I2C a injecter (decremente a chaque ecriture)
extern uint8_t stubMissingPcaMask;    // PCA a simuler absents du bus (bit 0 = 0x40)

// Cout simule d'une ecriture I2C vers un PCA9685 (5 octets a 400 kHz + overhead).
#define STUB_I2C_WRITE_US 110

void stubResetMachine(float physicalStart, float minSwitch, float maxSwitch);

#endif
