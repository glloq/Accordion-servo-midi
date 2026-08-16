#ifndef BELLOW_CONTROLLER_H
#define BELLOW_CONTROLLER_H

#include <Arduino.h>
#include "settings.h"
#include <FlexyStepper.h>
#include "servoController.h"

// Etats du soufflet
enum BellowState {
    BELLOW_INIT,   // Avant begin()
    BELLOW_HOMING, // Recherche du zero en cours
    BELLOW_READY,  // Pret a produire de la pression
    BELLOW_FAULT   // Defaut verrouille : moteur coupe, valve ouverte
};

// Codes de defaut
enum BellowFault {
    FAULT_NONE = 0,
    FAULT_HOMING_TIMEOUT,  // Le fin de course MIN n'a pas ete atteint a temps
    FAULT_HOMING_DISTANCE, // Course maximale depassee sans atteindre le fin de course
    FAULT_ENDSTOP_WIRING   // Les deux fins de course actifs simultanement
};

class BellowController {
public:
    // Constructeur prenant un ServoController en parametre
    BellowController(ServoController &servoCtrl);

    void begin();  // Initialise le moteur pas a pas et lance le homing
    void update(); // Machine a etats : homing, inversion 30/70%, fins de course, pas moteur

    // Demande d'air courante.
    //   airDemand      : somme des airFlowMultiplier des notes actives
    //   velocityFactor : facteur de dynamique issu de la velocite MIDI (voir Instrument)
    void setAirDemand(float airDemand, float velocityFactor);
    void setVolume(byte volumeValue);         // CC7  (volume principal)
    void setExpression(byte expressionValue); // CC11 (expression)

    void startHoming();     // (Re)lance la calibration, non bloquante
    void stopAndDisable();  // Arret + coupure du driver (mise au repos)

    void openValve();  // Ouvre la valve d'air
    void closeValve(); // Ferme la valve d'air

    bool isHoming() const { return state == BELLOW_HOMING; }
    bool isReady() const { return state == BELLOW_READY; }
    bool hasFault() const { return state == BELLOW_FAULT; }
    BellowFault getFault() const { return fault; }
    void clearFault(); // Acquitte un defaut et relance un homing

private:
    ServoController &servoController; // Reference vers le controleur des servos
    FlexyStepper stepper;             // Moteur pas a pas pour controler le soufflet

    BellowState state;
    BellowFault fault;

    bool valveOpen;       // Indique si la valve est ouverte
    bool movingDirection; // true = ouverture, false = fermeture
    bool motorRunning;    // Une consigne de pression est active
    bool motorEnabled;    // Etat de la broche ENABLE du driver (actif bas)

    float airDemand;      // Derniere demande d'air connue
    float velocityFactor; // Dernier facteur de dynamique connu
    byte volume;          // CC7  (0-127)
    byte expression;      // CC11 (0-127)
    float maxSpeed;       // Vitesse max reellement tenable (bornee par le debit de pas)

    uint32_t homingStartTime;

    // Debounce des fins de course
    uint32_t lastEndstopMinTime;
    uint32_t lastEndstopMaxTime;
    bool rawEndstopMin, rawEndstopMax;       // Dernier niveau lu
    bool stableEndstopMin, stableEndstopMax; // Niveau stabilise (true = actif)

    void updateEndstops();     // Debounce des deux fins de course
    void updateHoming();       // Progression du homing + detection de defauts
    void updateDirection();    // Inversion 30/70%, evaluee a chaque tour de boucle
    void checkEndStops();      // Recalage et inversion sur fin de course (etat READY)
    void applyDemand();        // Recalcule vitesse et cible depuis la demande courante
    void stopPressure();       // Stoppe le mouvement sans couper le driver
    void retarget();           // Envoie la cible correspondant a movingDirection
    void enableMotor();        // Active le driver (ENABLE bas)
    void disableMotor();       // Coupe le driver (ENABLE haut)
    void setFault(BellowFault code);
};

#endif
