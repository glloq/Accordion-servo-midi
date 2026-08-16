#ifndef BELLOW_CONTROLLER_H
#define BELLOW_CONTROLLER_H

#include <Arduino.h>
#include "settings.h"
#include <FlexyStepper.h>
#include "servoController.h"

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

// Codes de defaut
enum BellowFault {
    FAULT_NONE = 0,
    FAULT_HOMING_TIMEOUT,   // Butee basse non atteinte dans le temps imparti
    FAULT_HOMING_DISTANCE,  // Course maximale parcourue sans rencontrer la butee
    FAULT_ENDSTOP_WIRING,   // Les deux fins de course actifs simultanement
    FAULT_HOMING_DIRECTION, // Butee HAUTE atteinte alors qu'on descend (DIR inverse ?)
    FAULT_ENDSTOP_STUCK,    // Contact toujours actif apres degagement
    FAULT_STOP_TIMEOUT      // La sequence d'arret ne se termine pas
};

class BellowController {
public:
    // Constructeur prenant un ServoController en parametre
    BellowController(ServoController &servoCtrl);

    void begin();  // Initialise le moteur pas a pas et lance le homing
    void update(); // Securite fins de course, machine a etats, service du generateur de pas

    // Demande d'air courante : somme des debits des notes actives, deja ponderee par la
    // velocite de chaque note et par l'attaque en cours (voir Instrument).
    void setAirDemand(float airDemand);
    void setVolume(byte volumeValue);         // CC7  (volume principal)
    void setExpression(byte expressionValue); // CC11 (expression)

    void startHoming();     // (Re)lance la calibration, non bloquante
    void stopAndDisable();  // Arret + coupure du driver (mise au repos)

    void openValve();  // Ouvre la valve d'air
    void closeValve(); // Ferme la valve d'air

    bool isHoming() const;
    bool isReady() const { return state == BELLOW_READY; }
    bool hasFault() const { return state == BELLOW_FAULT; }
    BellowFault getFault() const { return fault; }
    void setFault(BellowFault code); // Passage en defaut (aussi utilise par Instrument)
    void clearFault();               // Acquitte un defaut et relance un homing

    // Diagnostic / tests
    BellowState getState() const { return state; }
    float getPositionInMillimeters() { return stepper.getCurrentPositionInMillimeters(); }
    float getMaxSpeed() const { return maxSpeed; }

private:
    ServoController &servoController; // Reference vers le controleur des servos
    FlexyStepper stepper;             // Moteur pas a pas pour controler le soufflet

    BellowState state;
    BellowState stateBeforeStop; // Etat a reprendre apres une sequence d'arret
    BellowFault fault;

    bool valveOpen;       // Indique si la valve est ouverte
    bool movingDirection; // true = ouverture, false = fermeture
    bool motorRunning;    // Une consigne de pression est active
    bool motorEnabled;    // Etat de la broche ENABLE du driver (actif bas)

    float airDemand;      // Derniere demande d'air connue
    byte volume;          // CC7  (0-127)
    byte expression;      // CC11 (0-127)
    float maxSpeed;       // Vitesse max reellement tenable (bornee par le debit de pas)

    uint32_t homingStartTime; // Debut du homing complet (toutes passes confondues)
    uint32_t stopStartTime;   // Debut de la sequence d'arret courante

    // Debounce des fins de course
    uint32_t lastEndstopMinTime;
    uint32_t lastEndstopMaxTime;
    bool rawEndstopMin, rawEndstopMax;       // Dernier niveau lu
    bool stableEndstopMin, stableEndstopMax; // Niveau stabilise (true = actif)

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
    void enableMotor();     // Active le driver (ENABLE bas)
    void disableMotor();    // Coupe le driver (ENABLE haut)
};

#endif
