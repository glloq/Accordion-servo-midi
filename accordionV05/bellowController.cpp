#include "bellowController.h"
#include <Wire.h>

// === CONSTRUCTEUR ===
// Initialise les variables et stocke une référence vers ServoController
BellowController::BellowController(ServoController &servoCtrl)
    : servoController(servoCtrl), valveOpen(false), movingDirection(true), currentSpeed(0), lastNoteTime(0), volume(127) {}

// === INITIALISATION ===
// Configure le moteur pas à pas et effectue le calibrage du soufflet
void BellowController::begin() {
    stepper.connectToPins(STEPPER_STEP_PIN, STEPPER_DIR_PIN);
    stepper.setStepsPerMillimeter(STEP_PER_MM);
    stepper.setSpeedInMillimetersPerSecond(STEPPER_MIN_SPEED);
    stepper.setAccelerationInMillimetersPerSecondPerSecond(STEPPER_MIN_ACCEL);
    
    pinMode(LIMIT_SWITCH_MIN_PIN, INPUT_PULLUP);
    pinMode(LIMIT_SWITCH_MAX_PIN, INPUT_PULLUP);
    pinMode(STEPPER_EN_PIN, OUTPUT);
    digitalWrite(STEPPER_EN_PIN, LOW); // Active le moteur

    calibrateZero(); // Effectue un homing au démarrage
}

// === MISE À JOUR ===
// Met à jour la position du moteur et vérifie les fins de course
void BellowController::update() {
    stepper.processMovement();
    checkEndStops();
}

// === CALIBRAGE HOMING ===
// Positionne le soufflet en butée pour définir une référence
void BellowController::calibrateZero() {
    digitalWrite(STEPPER_EN_PIN, LOW);
    stepper.setSpeedInMillimetersPerSecond(STEPPER_MIN_SPEED);
    
    // Déplacement jusqu'à atteindre le capteur de fin de course
    while (digitalRead(LIMIT_SWITCH_MIN_PIN) == HIGH) {
        stepper.moveRelativeInMillimeters(-1);
    }
    
    // Définition de la position de référence
    stepper.setCurrentPositionInMillimeters(0);
    movingDirection = true;
}

// === MISE À JOUR DE LA VITESSE ===
// Ajuste la vitesse du soufflet en fonction du débit d'air des notes actives
void BellowController::updateSpeed(float totalAirFlow) {
    // Calcul de la vitesse en fonction du débit d'air et du volume MIDI
    int16_t newSpeed = NORMAL_SPEED * totalAirFlow * (volume / 127.0);
    newSpeed = constrain(newSpeed, STEPPER_MIN_SPEED, STEPPER_MAX_SPEED);
    currentSpeed = newSpeed;

    // Gestion de l'ouverture et fermeture de la valve
    if (currentSpeed > 0 && !valveOpen) {
        openValve();
    } else if (currentSpeed == 0 && valveOpen) {
        closeValve();
    }

    stepper.setSpeedInMillimetersPerSecond(currentSpeed);
    stepper.processMovement();
    checkEndStops();
}

// === MISE À JOUR DU VOLUME MIDI ===
// Permet de moduler l'intensité du soufflet en fonction du volume MIDI reçu
void BellowController::updateVolume(byte volumeValue) {
    volume = volumeValue;
    updateSpeed(1.0); // Force une mise à jour de la vitesse
}

// === VÉRIFICATION DES FINS DE COURSE ===
// Stoppe le moteur si une extrémité est atteinte
void BellowController::checkEndStops() {
    if (digitalRead(LIMIT_SWITCH_MIN_PIN) == LOW) {
        stepper.setCurrentPositionInMillimeters(BELLOW_MIN_POSITION);
        movingDirection = true;
    }
    
    if (digitalRead(LIMIT_SWITCH_MAX_PIN) == LOW) {
        stepper.setCurrentPositionInMillimeters(BELLOW_MAX_POSITION);
        movingDirection = false;
    }
}

// === ARRÊT PROGRESSIF ===
// Réduit lentement la vitesse du soufflet avant de l'arrêter complètement
void BellowController::stopWithDecay() {
    stepper.setSpeedInMillimetersPerSecond(0);
    closeValve();
    digitalWrite(STEPPER_EN_PIN, HIGH); // Désactive le moteur après un temps d'inactivité
}

// === OUVERTURE DE LA VALVE ===
// Active la valve pour permettre un déplacement du soufflet sans notes actives
void BellowController::openValve() {
    servoController.setServoAngle(VALVE_PCA_ADDRESS, VALVE_PCA_PIN, VALVE_PCA_ANGLE_OPEN);
    valveOpen = true;
}

// === FERMETURE DE LA VALVE ===
// Ferme la valve lorsque des notes sont activées
void BellowController::closeValve() {
    servoController.setServoAngle(VALVE_PCA_ADDRESS, VALVE_PCA_PIN, VALVE_PCA_ANGLE_CLOSE);
    valveOpen = false;
}
