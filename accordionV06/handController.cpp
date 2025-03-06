#include "handController.h"

// === CONSTRUCTEUR ===
HandController::HandController(ServoController &servoCtrl, const ServoConfig *mapping, byte firstNote, byte numNotes)
    : servoController(servoCtrl), mapping(mapping), firstNote(firstNote), numNotes(numNotes) {}

// === VÉRIFICATION DE LA NOTE ===
bool HandController::canPlay(byte note) {
    return (note >= firstNote && note < firstNote + numNotes);
}

// === ACTIVATION D’UNE NOTE ===
float HandController::noteOn(byte note, byte velocity) {
    int index = note - firstNote;
    const ServoConfig &config = mapping[index];

    // 🔄 Ajustement de l’angle d’ouverture en fonction de `openDirection`
    uint16_t openAngle = config.openDirection ? (config.closedPosition - SERVO_OPEN_ANGLE) 
                                              : (config.closedPosition + SERVO_OPEN_ANGLE);

    servoController.setServoAngle(config.pcaAddress, config.channel, openAngle);
    return config.airFlowMultiplier;
}

// === DÉSACTIVATION D’UNE NOTE ===
float HandController::noteOff(byte note) {
    int index = note - firstNote;
    const ServoConfig &config = mapping[index];

    // 🔄 Fermeture à l’angle initial
    servoController.setServoAngle(config.pcaAddress, config.channel, config.closedPosition);
    return config.airFlowMultiplier;
}
