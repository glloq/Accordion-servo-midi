#ifndef INSTRUMENT_H
#define INSTRUMENT_H

// L'ordre des includes suit l'ordre de construction des membres.
#include "servoController.h"
#include "bellowController.h"
#include "handController.h"

// Etats globaux de l'instrument.
// Tant que l'etat n'est pas SYS_READY, les NoteOn sont refusees : accepter des notes
// pendant le homing rendait le systeme incoherent (valve fermee, compteur de notes
// incremente, mais soufflet immobile car en calibration).
enum SystemState {
    SYS_BOOT,       // Avant begin()
    SYS_SERVO_INIT, // Fermeture initiale des servos
    SYS_HOMING,     // Recherche du zero du soufflet
    SYS_READY,      // Jeu autorise
    SYS_FAULT       // Defaut verrouille
};

class Instrument {
public:
    Instrument();

    void begin();                                        // Initialise les controleurs
    void noteOn(byte note, byte velocity, byte channel); // Gestion d'une note ON
    void noteOff(byte note, byte channel);               // Gestion d'une note OFF
    void setVolume(byte volume);                         // CC7
    void setExpression(byte expression);                 // CC11
    void setSustain(bool active);                        // CC64
    void allNotesOff();                                  // Desactive toutes les notes (MIDI Panic)
    void update();                                       // Met a jour l'etat general de l'instrument

    bool isReady() const { return state == SYS_READY; }
    SystemState getState() const { return state; }
    BellowFault getFault() const { return bellowController.getFault(); }
    byte getActiveNoteCount() const { return activeNotes; }

private:
    // ORDRE DE CONSTRUCTION : servoController doit exister avant les objets qui en
    // conservent une reference.
    ServoController servoController;
    BellowController bellowController;
    HandController leftHand;
    HandController rightHand;

    SystemState state;

    float totalAirFlow;  // Somme des debits d'air des notes actives
    byte activeNotes;    // Nombre de notes actives
    uint16_t noteSequence; // Compteur d'activation, pour departager les vols de voix

    byte lastVelocity;       // Velocite de la derniere NoteOn
    uint32_t lastAttackTime; // Debut de la phase d'attaque courante
    bool attackActive;       // Phase d'attaque en cours

    bool sustainActive;  // Pedale de sustain (CC64)
    bool bellowIdle;     // Mise au repos deja effectuee (evite de re-commander en boucle)
    uint32_t lastActivityTime; // Date de la derniere note relachee

    HandController *handForChannel(byte channel);
    float velocityFactor() const;   // Facteur de dynamique courant
    void refreshAirDemand();        // Pousse la demande d'air vers le soufflet
    void removeAir(float airFlow, byte noteCount);
    bool stealVoice(byte incomingPriority); // Libere une voix moins prioritaire
    void enterFault();

    void manageInactivity(); // Mise au repos du soufflet apres inactivite
    void managePCA();        // Coupe l'OE des PCA quand plus rien ne bouge
};

#endif
