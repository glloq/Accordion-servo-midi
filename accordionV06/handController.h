#ifndef HANDCONTROLLER_H
#define HANDCONTROLLER_H

#include "settings.h"
#include "servoController.h"

// Nombre maximum de notes pour une main (main droite = 34)
#define MAX_NOTES_PER_HAND 34

// Classe générique pour contrôler les deux mains (gauche et droite)
class HandController {
public:
    // Constructeur générique pour gérer une main avec un mapping spécifique
    HandController(ServoController &servoCtrl, const ServoConfig *mapping, byte firstNote, byte numNotes);

    bool canPlay(byte note);                  // Vérifie si une note peut être jouée
    bool isNoteActive(byte note);             // Vérifie si une note est déjà active
    float noteOn(byte note, byte velocity);   // Active une note et retourne le débit d'air à ajouter
    float noteOff(byte note);                 // Désactive une note et retourne le débit d'air à retirer
    void allNotesOff();                       // Désactive toutes les notes (MIDI Panic)
    byte getActiveNoteCount() const { return activeCount; }
    void closeAllServos();                    // Ferme tous les servos (position initiale)

private:
    ServoController &servoController; // Référence au contrôleur de servos
    const ServoConfig *mapping;       // Mapping des servos pour cette main
    byte firstNote;                   // Première note gérée
    byte numNotes;                    // Nombre total de notes
    bool noteStates[MAX_NOTES_PER_HAND];  // État de chaque note (true = active)
    byte activeCount;                 // Nombre de notes actives
};

#endif
