#include "handController.h"

// Constructeur : initialise la main avec son mapping spécifique
HandController::HandController(ServoController &servoCtrl, const ServoConfig *mapping, byte firstNote, byte numNotes)
    : servoController(servoCtrl), mapping(mapping), firstNote(firstNote), numNotes(numNotes) {}

// Vérifie si la note fait partie de la plage gérée par cette main
bool HandController::canPlay(byte note) {
    return (note >= firstNote && note < firstNote + numNotes);
}

// Active la note en ouvrant la valve correspondante et renvoie le débit d'air
float HandController::noteOn(byte note, byte velocity) {
    int index = note - firstNote;
    const ServoConfig &config = mapping[index];
    servoController.setServoAngle(config.pcaAddress, config.channel, SERVO_OPEN_ANGLE);
    return config.airFlowMultiplier;
}

// Désactive la note en fermant la valve correspondante et renvoie le débit d'air
float HandController::noteOff(byte note) {
    int index = note - firstNote;
    const ServoConfig &config = mapping[index];
    servoController.setServoAngle(config.pcaAddress, config.channel, config.closedPosition);
    return config.airFlowMultiplier;
}
