#include <Arduino.h>
#include <Wire.h>
#include "instrument.h"
#include "midiHandler.h"

Instrument instrument;
MidiHandler midiHandler(instrument);

void setup() {
    #if DEBUG
    Serial.begin(115200);
    #endif

    Wire.begin();               // Initialise I2C pour les PCA9685 (SDA=D2, SCL=D3 sur Leonardo)
    Wire.setClock(I2C_CLOCK_HZ); // Reduire a 100000 si le bus est long ou bruite

    instrument.begin();     // Initialise servos et lance le homing du soufflet
    midiHandler.begin();    // Initialise les transports MIDI

    // Note : instrument.begin() ne bloque pas jusqu'a la fin du homing. Les NoteOn recues
    // avant la fin de la calibration sont refusees par Instrument (etat != SYS_READY),
    // seul le MIDI Panic reste actif.
}

void loop() {
    midiHandler.update();
    instrument.update();
}
