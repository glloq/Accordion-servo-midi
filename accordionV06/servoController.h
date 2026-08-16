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
    void enableServos(bool state); // Active/Desactive les sorties PWM des PCA (broche OE)

    bool isEnabled() const { return pcaEnabled; }

    // Date de la derniere commande servo envoyee. Permet a Instrument de ne couper l'OE
    // qu'une fois les servos arrives en position (la valve generale notamment).
    uint32_t getLastCommandTime() const { return lastCommandTime; }

private:
    Adafruit_PWMServoDriver pca[NUM_PCA_TOTAL];
    bool pcaEnabled;          // Etat actuel des PCA (true = active, false = desactive)
    uint32_t lastCommandTime; // millis() du dernier setServoAngle()
};

#endif
