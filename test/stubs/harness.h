#ifndef HARNESS_H
#define HARNESS_H
#include "settings.h"
class FlexyStepper;
extern unsigned long stubClock;
extern int  stubPinState[32];
extern FlexyStepper *g_stepper;
extern float stubMinSwitchPos;
extern float stubMaxSwitchPos;
extern bool  stubForceBothEndstops;
#endif
