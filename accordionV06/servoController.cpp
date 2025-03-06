#include "servoController.h"

ServoController::ServoController() : pcaEnabled(true) { // Initialisé à "activé"
    for (int i = 0; i < NUM_PCA_TOTAL; i++) {
        pca[i] = Adafruit_PWMServoDriver(PCA_TAB[i]);
    }
}

void ServoController::begin() {
    for (int i = 0; i < NUM_PCA_TOTAL; i++) {
        pca[i].begin();
        pca[i].setPWMFreq(SERVO_PWM_FREQUENCY); // Fréquence adaptée aux servos
    }
    enableServos(true); // Active les PCA au démarrage
}

// Définit l'angle d'un servo, en activant les PCA si nécessaire
void ServoController::setServoAngle(uint8_t pcaAddress, uint8_t channel, uint16_t angle) {
    if (!pcaEnabled) {
        enableServos(true); // Réactive les PCA si désactivés
    }

    int pwmValue = map(angle, SERVO_MIN_ANGLE, SERVO_MAX_ANGLE, SERVO_MIN_PWM, SERVO_MAX_PWM);
    for (int i = 0; i < NUM_PCA_TOTAL; i++) {
        if (PCA_TAB[i] == pcaAddress) {
            pca[i].setPWM(channel, 0, pwmValue);
            return;
        }
    }
}

// Active/Désactive tous les PCA et met à jour l’état `pcaEnabled`
void ServoController::enableServos(bool state) {
    if (pcaEnabled == state) return; // Évite les changements inutiles

    digitalWrite(PCA_OE_PIN, state ? LOW : HIGH); // Contrôle le pin d'activation
    pcaEnabled = state; // Met à jour l'état interne
}
