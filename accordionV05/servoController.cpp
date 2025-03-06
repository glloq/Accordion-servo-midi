#include "servoController.h"

ServoController::ServoController() {
    for (int i = 0; i < NUM_PCA_TOTAL; i++) {
        pca[i] = Adafruit_PWMServoDriver(PCA_TAB[i]);
    }
}

void ServoController::begin() {
    for (int i = 0; i < NUM_PCA_TOTAL; i++) {
        pca[i].begin();
        pca[i].setPWMFreq(50); // Fréquence adaptée aux servos
    }
}

void ServoController::setServoAngle(uint8_t pcaAddress, uint8_t channel, uint16_t angle) {
    int pwmValue = map(angle, 0, 180, 150, 600);
    for (int i = 0; i < NUM_PCA_TOTAL; i++) {
        if (PCA_TAB[i] == pcaAddress) {
            pca[i].setPWM(channel, 0, pwmValue);
            return;
        }
    }
}

void ServoController::enableServos(bool state) {
    for (int i = 0; i < NUM_PCA_TOTAL; i++) {
        digitalWrite(PCA_OE_PIN[i], state ? LOW : HIGH);
    }
}
