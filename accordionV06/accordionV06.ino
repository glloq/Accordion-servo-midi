#include <Arduino.h>
#include <Wire.h>
#include "instrument.h"
#include "midiHandler.h"

Instrument instrument;
MidiHandler midiHandler(instrument);

void setup() {
    Serial.begin(115200);
    Wire.begin();           // Initialise I2C pour les PCA9685
    instrument.begin();     // Initialise servos et soufflet
    midiHandler.begin();    // Initialise MIDI sur Serial1
}

void loop() {
    midiHandler.update();
    instrument.update();
}
