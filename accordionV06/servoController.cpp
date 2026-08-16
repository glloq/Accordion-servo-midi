#include "servoController.h"

ServoController::ServoController() : pcaEnabled(true), lastCommandTime(0) {
    for (int i = 0; i < NUM_PCA_TOTAL; i++) {
        pca[i] = Adafruit_PWMServoDriver(PCA_TAB[i]);
    }
}

void ServoController::begin() {
    // Configure le pin OE pour controler les sorties PWM des servos
    pinMode(PCA_OE_PIN, OUTPUT);
    digitalWrite(PCA_OE_PIN, HIGH);  // Sorties coupees pendant l'init
    pcaEnabled = false;

    for (int i = 0; i < NUM_PCA_TOTAL; i++) {
        pca[i].begin();
        pca[i].setPWMFreq(SERVO_PWM_FREQUENCY); // Frequence adaptee aux servos
    }

    lastCommandTime = millis();
    enableServos(true); // Active les PCA au demarrage
}

// Definit l'angle d'un servo, en reactivant les PCA si necessaire
void ServoController::setServoAngle(uint8_t pcaAddress, uint8_t channel, uint16_t angle) {
    if (!pcaEnabled) {
        enableServos(true); // Reactive les PCA s'ils avaient ete coupes
    }

    // Borne l'angle : une entree de mapping erronee ne doit pas produire un PWM hors plage.
    uint16_t safeAngle = constrain((int)angle, SERVO_MIN_ANGLE, SERVO_MAX_ANGLE);

    int pwmValue = map(safeAngle, SERVO_MIN_ANGLE, SERVO_MAX_ANGLE, SERVO_MIN_PWM, SERVO_MAX_PWM);
    for (int i = 0; i < NUM_PCA_TOTAL; i++) {
        if (PCA_TAB[i] == pcaAddress) {
            pca[i].setPWM(channel, 0, pwmValue);
            lastCommandTime = millis(); // Redemarre le compte a rebours de coupure de l'OE
            return;
        }
    }
}

// Active/Desactive tous les PCA et met a jour l'etat `pcaEnabled`
void ServoController::enableServos(bool state) {
    if (pcaEnabled == state) return; // Evite les changements inutiles

    digitalWrite(PCA_OE_PIN, state ? LOW : HIGH); // OE est actif bas
    pcaEnabled = state;
}
