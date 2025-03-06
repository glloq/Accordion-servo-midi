#ifndef INSTRUMENT_H
#define INSTRUMENT_H

#include "handController.h"
#include "bellowController.h"
#include "servoController.h"

class Instrument {
public:
    Instrument();

    void noteOn(byte note, byte velocity, byte channel);
    void noteOff(byte note, byte channel);
    void setVolume(byte volume);
    void update();

private:
    HandController leftHand;
    HandController rightHand;
    BellowController bellowController;
    ServoController servoController;

    float totalAirFlow;
    byte activeNotes;
    byte currentVolume;
};

#endif
