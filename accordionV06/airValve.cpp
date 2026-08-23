#include "airValve.h"

AirValve::AirValve(ServoController &servoCtrl)
#if AIR_VALVE_TYPE == AIR_VALVE_SERVO
    : servoController(servoCtrl), valveOpen(false)
#else
    : valveOpen(false)
#endif
{
#if AIR_VALVE_TYPE != AIR_VALVE_SERVO
    (void)servoCtrl;
#endif
}

void AirValve::begin() {
#if AIR_VALVE_TYPE == AIR_VALVE_SOLENOID
    pinMode(VALVE_SOLENOID_PIN, OUTPUT);
#endif
    // Position de securite : a l'air libre tant que rien n'a demande de pression.
    open();
}

void AirValve::apply(bool wantOpen) {
#if AIR_VALVE_TYPE == AIR_VALVE_SERVO
    servoController.setServoAngle(VALVE_PCA_ADDRESS, VALVE_PCA_PIN,
                                  wantOpen ? VALVE_PCA_ANGLE_OPEN : VALVE_PCA_ANGLE_CLOSE);
#elif AIR_VALVE_TYPE == AIR_VALVE_SOLENOID
    bool level = wantOpen ? (VALVE_SOLENOID_OPEN_LEVEL != 0) : (VALVE_SOLENOID_OPEN_LEVEL == 0);
    digitalWrite(VALVE_SOLENOID_PIN, level ? HIGH : LOW);
#else
    // Pas de valve : la mise a l'air libre est obtenue en arretant la source d'air.
    (void)wantOpen;
#endif
}

// open() et close() commandent SANS condition sur l'etat memorise : la valve est l'organe
// de securite, et une commande de fermeture perdue (erreur I2C, PCA reinitialise) ne doit
// pas etre masquee par un cache. Le cout est d'une ecriture I2C par evenement de note, pas
// par tour de boucle.
void AirValve::open() {
    apply(true);
    valveOpen = true;
}

void AirValve::close() {
    apply(false);
    valveOpen = false;
}
