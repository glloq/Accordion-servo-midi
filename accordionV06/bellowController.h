#ifndef BELLOW_CONTROLLER_H
#define BELLOW_CONTROLLER_H

#include <Arduino.h>
#include "settings.h"
#if AIR_SOURCE == AIR_SOURCE_BELLOW_STEPPER

#include <FlexyStepper.h>
#include "servoController.h"
#include "airValve.h"
#include "airFault.h"
#include "pressureRegulator.h"

// =========================================================================================
// SOURCE D'AIR : soufflet acoustique entraine par un moteur pas a pas.
// -----------------------------------------------------------------------------------------
// C'est la source la plus fidele a l'instrument d'origine : la pression est produite en
// poussant ET en tirant, avec inversion du sens avant les fins de course. C'est aussi la
// seule qui possede une course mesuree, donc une calibration.
//
// Elle implemente l'interface commune decrite dans airSource.h.
// =========================================================================================

// Etats du soufflet.
//
// Le homing se fait en DEUX passes (approche rapide, degagement, reapproche lente) :
// le premier contact ne sert qu'a localiser la butee. Comme l'arret d'urgence coupe le
// driver sans controler la deceleration, la surcourse du premier contact est inconnue ;
// seule la seconde approche, lente, fixe le zero.
enum BellowState {
    BELLOW_INIT,            // Avant begin()
    BELLOW_HOMING_FAST,     // Approche rapide de la butee basse
    BELLOW_HOMING_BACKOFF,  // Degagement de la butee
    BELLOW_HOMING_SLOW,     // Reapproche lente : fixe le zero
    BELLOW_STOPPING,        // Arret en cours : driver coupe, etat FlexyStepper en cours de purge
    BELLOW_READY,           // Pret a produire de la pression
    BELLOW_FAULT            // Defaut verrouille : moteur coupe, valve ouverte
};

class BellowController {
public:
    BellowController(ServoController &servoCtrl, AirValve &valve, PressureRegulator &regulator);

    void begin();  // Initialise le moteur pas a pas et lance le homing
    void update(); // Securite fins de course, machine a etats, service du generateur de pas

    // Demande d'air courante : somme des debits des notes actives, deja ponderee par la
    // velocite de chaque note et par l'attaque en cours (voir Instrument).
    void setAirDemand(float airDemand);
    void setVolume(byte volumeValue);         // CC7  (volume principal)
    void setExpression(byte expressionValue); // CC11 (expression)

    void startCalibration(); // (Re)lance la calibration, non bloquante
    void startHoming() { startCalibration(); } // Ancien nom
    void stopAndDisable();   // Arret + coupure du driver (mise au repos)

    void openValve();  // Ouvre la valve d'air (mise a l'air libre)
    void closeValve(); // Ferme la valve d'air

    bool isCalibrating() const;
    bool isHoming() const { return isCalibrating(); } // Ancien nom
    bool isReady() const { return state == BELLOW_READY; }
    bool hasFault() const { return state == BELLOW_FAULT; }
    AirFault getFault() const { return fault; }
    void setFault(AirFault code); // Passage en defaut (aussi utilise par Instrument)
    void clearFault();            // Acquitte un defaut et relance une calibration

    // Diagnostic / tests
    BellowState getState() const { return state; }
    // Position dans le repere de la MACHINE (0 = ferme), signe de DIR compris.
    float getPositionInMillimeters() {
        return stepper.getCurrentPositionInMillimeters() * STEPPER_DIR_SIGN;
    }
    float getMaxSpeed() const { return maxSpeed; }

private:
    AirValve &airValve;                // Valve generale de mise a l'air libre
    PressureRegulator &pressure;       // Correction en boucle fermee (neutre sans capteur)
    FlexyStepper stepper;              // Moteur pas a pas entrainant le soufflet

    BellowState state;
    BellowState stateBeforeStop; // Etat a reprendre apres une sequence d'arret
    AirFault fault;

    bool movingDirection; // true = ouverture, false = fermeture
    bool motorRunning;    // Une consigne de pression est active
    bool motorEnabled;    // Etat de la broche ENABLE du driver

    float airDemand;      // Derniere demande d'air connue
    byte volume;          // CC7  (0-127)
    byte expression;      // CC11 (0-127)
    float maxSpeed;       // Vitesse max reellement tenable (bornee par le debit de pas)

    uint32_t homingStartTime; // Debut de la calibration complete (toutes passes confondues)
    uint32_t stopStartTime;   // Debut de la sequence d'arret courante

    // Debounce des fins de course
    uint32_t lastEndstopMinTime;
    uint32_t lastEndstopMaxTime;
    bool rawEndstopMin, rawEndstopMax;       // Dernier niveau lu
    bool stableEndstopMin, stableEndstopMax; // Niveau stabilise (true = actif)

    static bool readEndstop(uint8_t pin);   // Lecture brute, niveau actif configure
    void updateEndstops();  // Debounce des deux fins de course
    // true si le mouvement en cours pousse VERS le contact indique. Un contact atteint
    // alors qu'on s'en eloigne n'est pas une urgence (cas du degagement de homing, ou du
    // repos sur la butee basse juste apres la calibration).
    bool drivingInto(bool minSwitch) const;

    void beginEmergencyStop(); // Coupe le driver immediatement, puis purge FlexyStepper
    void updateStopping();
    void onEndstopConfirmed(bool isMin);
    void resumeAfterStop();

    void startFastApproach();
    void startBackoff();
    void startSlowApproach();
    void updateHomingTravel(); // Surveille distance et duree des approches
    void updateBackoff();
    void finishHoming();

    void updateDirection(); // Inversion 30/70%, evaluee a chaque tour de boucle
    void applyDemand();     // Recalcule vitesse et cible depuis la demande courante
    void stopPressure();    // Stoppe le mouvement sans couper le driver
    void retarget();        // Envoie la cible correspondant a movingDirection
    void serviceStepper();  // Appels de service au generateur de pas
    void enableMotor();     // Active le driver
    void disableMotor();    // Coupe le driver
};

#endif // AIR_SOURCE_BELLOW_STEPPER
#endif
