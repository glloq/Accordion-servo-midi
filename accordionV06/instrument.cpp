#include "instrument.h"

Instrument::Instrument()
    : totalAirFlow(0), activeNotes(0), lastActivityTime(millis()),
      leftHand(servoController, LEFT_HAND_MAPPING, FIRST_NOTE_LEFT, NUM_NOTES_LEFT),
      rightHand(servoController, RIGHT_HAND_MAPPING, FIRST_NOTE_RIGHT, NUM_NOTES_RIGHT),
      bellowController(servoController) {}

// === INITIALISATION ===
void Instrument::begin() {
    servoController.begin();   // Initialise les PCA9685
    bellowController.begin();  // Initialise le moteur pas à pas et calibre
}

// === ACTIVATION D'UNE NOTE ===
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

    // Ne traiter que si une note a réellement été activée
    if (airFlow > 0) {
        // Fermer la valve si c'est la première note (créer la pression)
        if (activeNotes == 0) {
            bellowController.closeValve();
        }

        totalAirFlow += airFlow;
        activeNotes++;
        bellowController.updateSpeed(totalAirFlow);
    }
}

// === DÉSACTIVATION D'UNE NOTE ===
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

        if (activeNotes == 0) {
            lastActivityTime = millis(); // 🔄 Enregistrement du dernier NoteOff
        }
    }
}

// === MET À JOUR L'INSTRUMENT ===
void Instrument::update() {
    bellowController.update();
    managePCA();        // Gère l'activation/désactivation des PCA
    manageInactivity(); // Gère l'inactivité du soufflet
}
// === GESTION DE LA DÉSACTIVATION DES PCA ===
void Instrument::managePCA() {
    if (activeNotes == 0 && millis() - lastActivityTime > PCA_DISABLE_DELAY) {
        servoController.enableServos(false);
    }
}

// === GESTION DU VOLUME MIDI ===
void Instrument::setVolume(byte volume) {
    //currentVolume = volume;
    bellowController.updateVolume(volume); // 🔹 Ajuste la vitesse du soufflet en fonction du volume
}

// === GESTION DE L'INACTIVITÉ DU SOUFFLET ===
void Instrument::manageInactivity() {
    if (activeNotes == 0 && millis() - lastActivityTime > BELLOW_INACTIVITY_TIMEOUT) {
        bellowController.openValve(); // 🔄 ouvre la valve après 1 min d'inactivité

        bellowController.stopWithDecay(); // 🔄 Désactive le moteur du soufflet
    }
}
