#ifndef SERVOCONTROLLER_H
#define SERVOCONTROLLER_H

#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include "settings.h"

class ServoController {
public:
    ServoController();

    // Sonde les 4 PCA9685 sur le bus I2C avant de les initialiser.
    // Retourne false si au moins un composant ne repond pas : un PCA absent signifie des
    // anches qui ne s'ouvriront jamais, ou pire une valve generale inoperante alors que le
    // soufflet, lui, fonctionne.
    bool begin();

    void setServoAngle(uint8_t pcaAddress, uint8_t channel, uint16_t angle);
    void enableServos(bool state); // Active/Desactive les sorties PWM des PCA (broche OE)

    bool isEnabled() const { return pcaEnabled; }

    // Date de la derniere commande servo envoyee. Permet a Instrument de ne couper l'OE
    // qu'une fois les servos arrives en position (la valve generale notamment).
    uint32_t getLastCommandTime() const { return lastCommandTime; }

    // Diagnostic
    uint8_t getMissingMask() const { return missingMask; }  // 1 bit par PCA absent
    uint8_t getConsecutiveErrors() const { return consecutiveErrors; }
    bool hasBusFailure() const { return consecutiveErrors >= SERVO_I2C_ERROR_LIMIT; }

private:
    Adafruit_PWMServoDriver pca[NUM_PCA_TOTAL];
    bool pcaEnabled;           // Etat actuel des PCA (true = active, false = desactive)
    uint32_t lastCommandTime;  // millis() du dernier setServoAngle()
    uint8_t missingMask;       // Bits des PCA n'ayant pas repondu au demarrage
    uint8_t consecutiveErrors; // Erreurs I2C consecutives en fonctionnement

    static bool probe(uint8_t address); // Presence d'un composant a cette adresse I2C
};

#endif
