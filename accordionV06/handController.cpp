#include "handController.h"

// === CONSTRUCTEUR ===
HandController::HandController(ServoController &servoCtrl, const ServoConfig *mapping, byte firstNote, byte numNotes)
    : servoController(servoCtrl), mapping(mapping), firstNote(firstNote), numNotes(numNotes), activeCount(0) {
    // Initialise tous les états à inactif
    for (byte i = 0; i < MAX_NOTES_PER_HAND; i++) {
        noteStates[i] = false;
    }
}

// === VÉRIFICATION DE LA NOTE ===
bool HandController::canPlay(byte note) {
    return (note >= firstNote && note < firstNote + numNotes);
}

// === VÉRIFIE SI UNE NOTE EST ACTIVE ===
bool HandController::isNoteActive(byte note) {
    if (!canPlay(note)) return false;
    return noteStates[note - firstNote];
}

// === ACTIVATION D'UNE NOTE ===
float HandController::noteOn(byte note, byte velocity) {
    // Vérification de sécurité des bornes
    if (!canPlay(note)) return 0.0f;

    int index = note - firstNote;

    // Ignore si la note est déjà active (évite double comptage)
    if (noteStates[index]) return 0.0f;

    const ServoConfig &config = mapping[index];

    // Ajustement de l'angle d'ouverture en fonction de `openDirection`
    uint16_t openAngle = config.openDirection ? (config.closedPosition - SERVO_OPEN_ANGLE)
                                              : (config.closedPosition + SERVO_OPEN_ANGLE);

    servoController.setServoAngle(config.pcaAddress, config.channel, openAngle);

    // Marque la note comme active
    noteStates[index] = true;
    activeCount++;

    return config.airFlowMultiplier;
}

// === DÉSACTIVATION D'UNE NOTE ===
float HandController::noteOff(byte note) {
    // Vérification de sécurité des bornes
    if (!canPlay(note)) return 0.0f;

    int index = note - firstNote;

    // Ignore si la note n'est pas active
    if (!noteStates[index]) return 0.0f;

    const ServoConfig &config = mapping[index];

    // Fermeture à l'angle initial
    servoController.setServoAngle(config.pcaAddress, config.channel, config.closedPosition);

    // Marque la note comme inactive
    noteStates[index] = false;
    if (activeCount > 0) activeCount--;

    return config.airFlowMultiplier;
}

// === DÉSACTIVE TOUTES LES NOTES (MIDI PANIC) ===
void HandController::allNotesOff() {
    for (byte i = 0; i < numNotes; i++) {
        if (noteStates[i]) {
            const ServoConfig &config = mapping[i];
            servoController.setServoAngle(config.pcaAddress, config.channel, config.closedPosition);
            noteStates[i] = false;
        }
    }
    activeCount = 0;
}

// === FERME TOUS LES SERVOS (POSITION INITIALE) ===
void HandController::closeAllServos() {
    for (byte i = 0; i < numNotes; i++) {
        const ServoConfig &config = mapping[i];
        servoController.setServoAngle(config.pcaAddress, config.channel, config.closedPosition);
        noteStates[i] = false;
    }
    activeCount = 0;
}
