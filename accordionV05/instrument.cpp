#include "instrument.h"

// Correction : Passer les paramètres nécessaires à HandController et BellowController
Instrument::Instrument()
    : totalAirFlow(0), activeNotes(0), currentVolume(127),
      leftHand(servoController, LEFT_HAND_MAPPING, FIRST_NOTE_LEFT, NUM_NOTES_LEFT), 
      rightHand(servoController, RIGHT_HAND_MAPPING, FIRST_NOTE_RIGHT, NUM_NOTES_RIGHT),
      bellowController(servoController) {} // Correction : Passer `servoController` à `bellowController`

void Instrument::noteOn(byte note, byte velocity, byte channel) {
    float airFlow = 0.0f;

    if (channel == MIDI_CHANNEL_LEFT) {
        if (leftHand.canPlay(note)) {
            airFlow = leftHand.noteOn(note, velocity);
        }
    } else if (channel == MIDI_CHANNEL_RIGHT) {
        if (rightHand.canPlay(note)) {
            airFlow = rightHand.noteOn(note, velocity);
        }
    }

    if (airFlow > 0) {
        totalAirFlow += airFlow;
        activeNotes++;
        bellowController.updateSpeed(totalAirFlow);
    }
}

void Instrument::noteOff(byte note, byte channel) {
    float airFlow = 0.0f;

    if (channel == MIDI_CHANNEL_LEFT) {
        airFlow = leftHand.noteOff(note);
    } else if (channel == MIDI_CHANNEL_RIGHT) {
        airFlow = rightHand.noteOff(note);
    }

    if (airFlow > 0) {
        totalAirFlow -= airFlow;
        if (activeNotes > 0) activeNotes--;
        bellowController.updateSpeed(totalAirFlow);
    }
}

// === MISE À JOUR DU VOLUME MIDI ===
void Instrument::setVolume(byte volume) {
    currentVolume = volume;
    bellowController.updateSpeed(totalAirFlow);
}

// === MISE À JOUR DE L'INSTRUMENT ===
void Instrument::update() {
    bellowController.update();
}
