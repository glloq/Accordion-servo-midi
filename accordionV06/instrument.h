#ifndef INSTRUMENT_H
#define INSTRUMENT_H

#include "handController.h"
#include "bellowController.h"
#include "servoController.h"

class Instrument {
public:
    Instrument();

    void noteOn(byte note, byte velocity, byte channel); // Gestion d'une note ON
    void noteOff(byte note, byte channel);               // Gestion d'une note OFF
    void setVolume(byte volume);                         // Mise à jour du volume MIDI
    void update();                                       // Met à jour l'état général de l'instrument

private:
    HandController leftHand;
    HandController rightHand;
    BellowController bellowController;
    ServoController servoController;

    float totalAirFlow;  // Somme des débits d'air
    byte activeNotes;    // Nombre de notes actives
    //byte currentVolume;  // Volume actuel MIDI (0-127)
    
    uint32_t lastActivityTime; // Temps de la dernière note jouée
    void manageInactivity(); // Fonction pour gérer l'inactivité

    void managePCA(); // Gère l'activation/désactivation des PCA
};

#endif
