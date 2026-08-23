#ifndef SERVOCONTROLLER_H
#define SERVOCONTROLLER_H

#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include "settings.h"

// Acces unique aux PCA9685. Tout ce qui est branche sur un PCA passe par ici : valves
// d'anches (servo ou electroaimant), valve generale, servo de soufflet, canal ESC.
//
// Le compteur d'erreurs I2C est global et non par composant : ce qui compte est de detecter
// qu'un bus muet laisse des anches ouvertes, pas de savoir lequel des PCA a laché.
class ServoController {
public:
    ServoController();

    // Sonde les PCA9685 sur le bus I2C avant de les initialiser.
    // Retourne false si au moins un composant ne repond pas : un PCA absent signifie des
    // anches qui ne s'ouvriront jamais, ou pire une valve generale inoperante alors que la
    // source d'air, elle, fonctionne.
    bool begin();

    // === COMMANDES ===
    void setServoAngle(uint8_t pcaAddress, uint8_t channel, uint16_t angle);
    // Impulsion en microsecondes : ce que demande un ESC, dont la plage utile (1000-2000 us)
    // ne correspond pas a la plage d'angles des servos.
    void setPulseMicroseconds(uint8_t pcaAddress, uint8_t channel, uint16_t pulseUs);
    // Rapport cyclique brut, 0-4095. Utilise pour les electroaimants : pleine puissance a
    // l'appel, courant de maintien reduit ensuite.
    void setRawDuty(uint8_t pcaAddress, uint8_t channel, uint16_t counts);

    // === SORTIES PWM (broche OE) ===
    // Couper l'OE rend l'instrument silencieux : les servos cessent de forcer, les
    // electroaimants retombent. Une commande envoyee ensuite reactive automatiquement le
    // banc concerne — sans quoi elle partirait dans le vide.
    void enableServos(bool state);                    // Tous les PCA
    // Un seul banc. N'a d'effet separe qu'en PCA_OE_PER_PCA ; avec un OE partage, couper un
    // banc reviendrait a tout couper, ce que la methode refuse de faire silencieusement.
    void enableBank(uint8_t pcaIndex, bool state);
    bool isBankEnabled(uint8_t pcaIndex) const;

    // Au moins une sortie est active.
    bool isEnabled() const { return bankMask != 0; }

    // Date de la derniere commande envoyee. Permet a Instrument de ne couper l'OE qu'une
    // fois les servos arrives en position (la valve generale notamment).
    uint32_t getLastCommandTime() const { return lastCommandTime; }

    // Diagnostic
    uint8_t getMissingMask() const { return missingMask; }  // 1 bit par PCA absent
    uint8_t getConsecutiveErrors() const { return consecutiveErrors; }
    bool hasBusFailure() const { return consecutiveErrors >= SERVO_I2C_ERROR_LIMIT; }

private:
    Adafruit_PWMServoDriver pca[NUM_PCA_TOTAL];
    // Un bit par PCA : 1 = sorties actives. Avec un OE partage les bits bougent ensemble ;
    // avec une broche par PCA ils sont independants, ce qui permet de couper les bancs
    // d'anches en gardant alimente celui qui porte la valve generale.
    uint16_t bankMask;
    uint32_t lastCommandTime;  // millis() de la derniere commande
    uint8_t missingMask;       // Bits des PCA n'ayant pas repondu au demarrage
    uint8_t consecutiveErrors; // Erreurs I2C consecutives en fonctionnement

    static bool probe(uint8_t address);          // Presence d'un composant a cette adresse
    static int8_t indexOfAddress(uint8_t address);
    static void writeOePin(uint8_t pin, bool enabled); // OE est actif bas
    // Ecriture effective : compte les erreurs et horodate. Tous les setters y aboutissent.
    void write(uint8_t pcaAddress, uint8_t channel, uint16_t onCounts, uint16_t offCounts);
};

// Conversion d'une impulsion en microsecondes vers les comptes du PCA9685.
// Le compteur fait 4096 pas par periode, et la periode vaut 1/SERVO_PWM_FREQUENCY.
inline uint16_t pulseUsToCounts(uint32_t pulseUs) {
    uint32_t counts = (pulseUs * 4096UL * (uint32_t)SERVO_PWM_FREQUENCY) / 1000000UL;
    if (counts > 4095UL) counts = 4095UL;
    return (uint16_t)counts;
}

#endif
