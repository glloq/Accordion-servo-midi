#ifndef MIDI_HANDLER_H
#define MIDI_HANDLER_H

#include <Arduino.h>
#include "instrument.h"
#include <MIDI.h>

class MidiHandler {
public:
    MidiHandler(Instrument& instr);

    void begin();  // Initialise MIDI
    void update(); // Vérifie les messages MIDI

private:
    Instrument& instrument;
    byte volume;
    
    void processMIDI(); // Interne, traite les messages MIDI
    void handleControlChange(byte control, byte value);
};

#endif
