#ifndef INSTRUMENT_H
#define INSTRUMENT_H

#include "handController.h"
#include "bellowController.h"
#include "servoController.h"

// Limite de notes simultanées (protection alimentation)
#define MAX_SIMULTANEOUS_NOTES 15

class Instrument {
public:
    Instrument();

    void begin();                                        // Initialise les contrôleurs
    void noteOn(byte note, byte velocity, byte channel); // Gestion d'une note ON
    void noteOff(byte note, byte channel);               // Gestion d'une note OFF
    void setVolume(byte volume);                         // Mise à jour du volume MIDI
    void allNotesOff();                                  // Désactive toutes les notes (MIDI Panic)
    void update();                                       // Met à jour l'état général de l'instrument

private:
    HandController leftHand;
    HandController rightHand;
    BellowController bellowController;
    ServoController servoController;

    float totalAirFlow;  // Somme des débits d'air
    byte activeNotes;    // Nombre de notes actives

    uint32_t lastActivityTime; // Temps de la dernière note jouée
    void manageInactivity();   // Fonction pour gérer l'inactivité
    void managePCA();          // Gère l'activation/désactivation des PCA
};

#endif
