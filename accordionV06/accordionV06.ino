#include <Arduino.h>
#include "instrument.h"
#include "midiHandler.h"

Instrument instrument;
MidiHandler midiHandler(instrument);

void setup() {
    Serial.begin(115200);
}

void loop() {
    midiHandler.update(); 
    instrument.update();  
}
