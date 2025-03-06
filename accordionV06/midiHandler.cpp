#include "midiHandler.h"
#include <MIDI.h>

MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);

MidiHandler::MidiHandler(Instrument& instr)
    : instrument(instr), volume(127) {}

void MidiHandler::begin() {
    MIDI.begin(MIDI_CHANNEL_OMNI); // Écoute tous les canaux MIDI
}

void MidiHandler::update() {
    processMIDI();
}

// Traite les messages MIDI reçus
void MidiHandler::processMIDI() {
    if (!MIDI.read()) return;

    byte type = MIDI.getType();
    byte channel = MIDI.getChannel();
    byte note = MIDI.getData1();
    byte velocity = MIDI.getData2();

    if (type == midi::ControlChange) {
        handleControlChange(note, velocity);
        return;
    }

    if (type == midi::NoteOn && velocity > 0) {
        instrument.noteOn(note, velocity, channel);
    } else if (type == midi::NoteOff || velocity == 0) {
        instrument.noteOff(note, channel);
    }
}

// Gestion des changements de volume MIDI
void MidiHandler::handleControlChange(byte control, byte value) {
    if (control == 7) {
        volume = value;
        instrument.setVolume(volume);
    }
}
