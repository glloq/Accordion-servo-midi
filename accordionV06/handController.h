#ifndef HANDCONTROLLER_H
#define HANDCONTROLLER_H

#include "settings.h"
#include "servoController.h"

// Classe générique pour contrôler les deux mains (gauche et droite)
class HandController {
public:
    // Constructeur générique pour gérer une main avec un mapping spécifique
    HandController(ServoController &servoCtrl, const ServoConfig *mapping, byte firstNote, byte numNotes);

    bool canPlay(byte note);      // Vérifie si une note peut être jouée
    float noteOn(byte note, byte velocity);  // Active une note et retourne le débit d'air a ajouter
    float noteOff(byte note);     // Désactive une note et retourne le débit d'air a retirer

private:
    ServoController &servoController; // Référence au contrôleur de servos
    const ServoConfig *mapping;       // Mapping des servos pour cette main
    byte firstNote;                    // Première note gérée
    byte numNotes;                      // Nombre total de notes
};

#endif
