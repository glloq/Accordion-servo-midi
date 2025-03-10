#ifndef SERVOCONTROLLER_H
#define SERVOCONTROLLER_H

#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include "settings.h"

class ServoController {
public:
    ServoController();
    void begin();
    void setServoAngle(uint8_t pcaAddress, uint8_t channel, uint16_t angle);
    void enableServos(bool state); // Active/Désactive les PCA

private:
    Adafruit_PWMServoDriver pca[NUM_PCA_TOTAL];
    bool pcaEnabled; // État actuel des PCA (true = activé, false = désactivé)
};

#endif
