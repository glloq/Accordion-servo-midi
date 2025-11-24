#include "bellowController.h"
#include <Wire.h>

// === CONSTRUCTEUR ===
// Initialise les variables et stocke une référence vers ServoController
BellowController::BellowController(ServoController &servoCtrl)
    : servoController(servoCtrl), valveOpen(false), movingDirection(true),
      motorRunning(false), currentSpeed(0), volume(100), lastTotalAirFlow(0),
      calibState(CALIB_IDLE), lastEndstopMinTime(0), lastEndstopMaxTime(0),
      lastEndstopMinState(HIGH), lastEndstopMaxState(HIGH) {}

// === INITIALISATION ===
// Configure le moteur pas à pas et démarre le calibrage
void BellowController::begin() {
    stepper.connectToPins(STEPPER_STEP_PIN, STEPPER_DIR_PIN);
    stepper.setStepsPerMillimeter(STEP_PER_MM);
    stepper.setSpeedInMillimetersPerSecond(STEPPER_MIN_SPEED);
    stepper.setAccelerationInMillimetersPerSecondPerSecond(STEPPER_MIN_ACCEL);

    pinMode(LIMIT_SWITCH_MIN_PIN, INPUT_PULLUP);
    pinMode(LIMIT_SWITCH_MAX_PIN, INPUT_PULLUP);
    pinMode(STEPPER_EN_PIN, OUTPUT);
    digitalWrite(STEPPER_EN_PIN, LOW); // Active le moteur

    startCalibration(); // Démarre le homing (non-bloquant)
}

// === MISE À JOUR ===
// Met à jour la position du moteur et vérifie les fins de course
void BellowController::update() {
    // Gère la calibration si en cours
    if (calibState != CALIB_IDLE) {
        updateCalibration();
    }

    stepper.processMovement();
    checkEndStops();
}

// === DÉMARRAGE CALIBRAGE (NON-BLOQUANT) ===
void BellowController::startCalibration() {
    openValve();  // Ouvre la vanne à vide
    digitalWrite(STEPPER_EN_PIN, LOW);
    stepper.setSpeedInMillimetersPerSecond(STEPPER_MIN_SPEED);

    // Configure le mouvement continu vers le min
    stepper.setTargetPositionInMillimeters(-BELLOW_MAX_POSITION * 2);
    calibState = CALIB_MOVING;
}

// === MISE À JOUR CALIBRAGE ===
void BellowController::updateCalibration() {
    if (calibState == CALIB_MOVING) {
        // Vérifie si le fin de course min est atteint
        if (digitalRead(LIMIT_SWITCH_MIN_PIN) == LOW) {
            stepper.setCurrentPositionInMillimeters(0);
            stepper.setTargetPositionInMillimeters(0);  // Arrête le mouvement
            movingDirection = true;
            calibState = CALIB_IDLE;
        }
    }
}

// === MISE À JOUR DE LA VITESSE ===
// Ajuste la vitesse du soufflet en fonction du débit d'air des notes actives
void BellowController::updateSpeed(float totalAirFlow) {
    // Sauvegarde pour updateVolume()
    lastTotalAirFlow = totalAirFlow;

    // Ne pas bouger pendant la calibration
    if (calibState != CALIB_IDLE) return;

    // Si aucune note active (airFlow = 0), arrêter le moteur
    if (totalAirFlow <= 0.0f) {
        if (motorRunning) {
            // Arrête le moteur à la position actuelle
            float currentPos = stepper.getCurrentPositionInMillimeters();
            stepper.setTargetPositionInMillimeters(currentPos);
            motorRunning = false;
        }
        return;
    }

    // Calcul de la vitesse en fonction du débit d'air et du volume MIDI
    int16_t newSpeed = NORMAL_SPEED * totalAirFlow * (volume / 127.0f);
    newSpeed = constrain(newSpeed, STEPPER_MIN_SPEED, STEPPER_MAX_SPEED);
    currentSpeed = newSpeed;

    // Vérification des seuils pour inverser la direction avant les FDC
    float currentPosition = stepper.getCurrentPositionInMillimeters();
    float normalizedPosition = (currentPosition - BELLOW_MIN_POSITION) / (BELLOW_MAX_POSITION - BELLOW_MIN_POSITION);

    if (normalizedPosition >= BELLOW_REVERSE_THRESHOLD_OPEN) {
        movingDirection = false; // On commence à refermer le soufflet
    }
    if (normalizedPosition <= BELLOW_REVERSE_THRESHOLD_CLOSE) {
        movingDirection = true; // On commence à rouvrir le soufflet
    }

    // Définit la cible en fonction de la direction
    float target = movingDirection ? BELLOW_MAX_POSITION : BELLOW_MIN_POSITION;
    stepper.setSpeedInMillimetersPerSecond(currentSpeed);
    stepper.setTargetPositionInMillimeters(target);
    motorRunning = true;
}

// === MISE À JOUR DU VOLUME MIDI ===
// Permet de moduler l'intensité du soufflet en fonction du volume MIDI reçu
void BellowController::updateVolume(byte volumeValue) {
    volume = volumeValue;
    // Utilise le dernier débit d'air connu au lieu de 1.0
    updateSpeed(lastTotalAirFlow);
}

// === VÉRIFICATION DES FINS DE COURSE (AVEC DEBOUNCE) ===
// Stoppe le moteur si une extrémité est atteinte
void BellowController::checkEndStops() {
    uint32_t now = millis();
    bool currentMinState = digitalRead(LIMIT_SWITCH_MIN_PIN);
    bool currentMaxState = digitalRead(LIMIT_SWITCH_MAX_PIN);

    // Debounce fin de course MIN
    if (currentMinState != lastEndstopMinState) {
        lastEndstopMinTime = now;
        lastEndstopMinState = currentMinState;
    }
    if (currentMinState == LOW && (now - lastEndstopMinTime) > ENDSTOP_DEBOUNCE_MS) {
        stepper.setCurrentPositionInMillimeters(BELLOW_MIN_POSITION);
        movingDirection = true;
    }

    // Debounce fin de course MAX
    if (currentMaxState != lastEndstopMaxState) {
        lastEndstopMaxTime = now;
        lastEndstopMaxState = currentMaxState;
    }
    if (currentMaxState == LOW && (now - lastEndstopMaxTime) > ENDSTOP_DEBOUNCE_MS) {
        stepper.setCurrentPositionInMillimeters(BELLOW_MAX_POSITION);
        movingDirection = false;
    }
}

// === ARRÊT PROGRESSIF ===
// Arrête le moteur et désactive le driver (économie d'énergie)
void BellowController::stopWithDecay() {
    float currentPos = stepper.getCurrentPositionInMillimeters();
    stepper.setTargetPositionInMillimeters(currentPos);  // Arrête le mouvement
    motorRunning = false;
    digitalWrite(STEPPER_EN_PIN, HIGH); // Désactive le driver moteur
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
