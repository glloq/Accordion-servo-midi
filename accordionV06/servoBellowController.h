#ifndef SERVO_BELLOW_CONTROLLER_H
#define SERVO_BELLOW_CONTROLLER_H

#include <Arduino.h>
#include "settings.h"
#if AIR_SOURCE == AIR_SOURCE_BELLOW_SERVO

#include "servoController.h"
#include "airValve.h"
#include "airFault.h"
#include "pressureRegulator.h"

// =========================================================================================
// SOURCE D'AIR : soufflet entraine par un servomoteur.
// -----------------------------------------------------------------------------------------
// Meme principe bidirectionnel que le soufflet pas a pas — la pression est produite en
// ouvrant ET en fermant, avec inversion avant les extremes — mais la position est commandee
// en ANGLE. Consequences directes :
//
//   - pas de fin de course : la butee est l'angle lui-meme, borne par la configuration ;
//   - pas de calibration : un servo connait sa position, l'instrument est pret des begin() ;
//   - la demande d'air fixe la VITESSE de balayage, pas la position.
//
// Le servo etant commande en position et non en vitesse, le balayage est produit en
// avancant la consigne d'angle d'un increment a chaque periode BELLOW_SERVO_UPDATE_MS.
// Envoyer une consigne a chaque tour de boucle saturerait le bus I2C pour rien : un servo
// analogique ne rafraichit sa position qu'a SERVO_PWM_FREQUENCY.
//
// Limites assumees : course et force bien inferieures a celles d'un moteur pas a pas sur
// vis. C'est le systeme des maquettes et des petits instruments.
// =========================================================================================
class ServoBellowController {
public:
    ServoBellowController(ServoController &servoCtrl, AirValve &valve,
                          PressureRegulator &regulator);

    void begin();
    void update();

    void setAirDemand(float airDemand);
    void setVolume(byte volumeValue);
    void setExpression(byte expressionValue);

    void startCalibration(); // Ramene le soufflet a l'angle ferme
    void stopAndDisable();

    void openValve();
    void closeValve();

    bool isCalibrating() const { return false; } // Rien a calibrer : l'angle est la position
    bool isReady() const { return ready && fault == FAULT_NONE; }
    bool hasFault() const { return fault != FAULT_NONE; }
    AirFault getFault() const { return fault; }
    void setFault(AirFault code);
    void clearFault();

    // Diagnostic / tests
    float getAngle() const { return angle; }
    bool isMoving() const { return running; }
    // Fraction de course ouverte, 0 = ferme, 1 = ouvert. Independante du sens de montage.
    float getOpening() const;

private:
    ServoController &servoController;
    AirValve &airValve;
    PressureRegulator &pressure;

    float angle;          // Consigne d'angle courante (degres)
    bool movingDirection; // true = vers l'angle ouvert
    bool running;         // Une demande d'air est active
    bool ready;
    AirFault fault;

    float airDemand;
    byte volume;
    byte expression;

    uint32_t lastUpdate;  // Derniere consigne envoyee au servo

    float sweepSpeed() const; // Vitesse de balayage (deg/s) pour la demande courante
    void writeAngle();
};

#endif // AIR_SOURCE_BELLOW_SERVO
#endif
