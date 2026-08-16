#ifndef HANDCONTROLLER_H
#define HANDCONTROLLER_H

#include "settings.h"
#include "servoController.h"

// Nombre maximum de notes pour une main (main droite = 34)
#define MAX_NOTES_PER_HAND 34

// Classe generique pour controler les deux mains (gauche et droite).
//
// La velocite n'est volontairement pas traitee ici : une valve d'anche est ouverte ou
// fermee, il n'y a pas de nuance possible cote servo. La dynamique est produite par le
// debit d'air (voir BellowController).
class HandController {
public:
    // Constructeur generique pour gerer une main avec un mapping specifique
    HandController(ServoController &servoCtrl, const NoteConfig *mapping, byte numNotes);

    bool canPlay(byte note) const;      // La note existe-t-elle dans le mapping ?
    bool isNoteActive(byte note) const; // La note est-elle deja ouverte ?
    byte priorityOf(byte note) const;   // Priorite de voix de la note (0 si inconnue)

    // Ouvre la valve de la note. `seq` est un compteur global croissant servant a
    // departager les candidats au vol de voix (la plus ancienne part en premier).
    // Retourne le debit d'air a ajouter, ou 0 si la note est inconnue / deja active.
    float noteOn(byte note, uint16_t seq);

    // Ferme reellement la valve. Retourne le debit d'air a retirer, 0 si rien a faire.
    float noteOff(byte note);

    // Sustain : la note reste ouverte mais est marquee comme relachee par le clavier.
    void markSustained(byte note);
    // Ferme toutes les notes retenues uniquement par la pedale.
    // `releasedCount` recoit le nombre de notes fermees. Retourne le debit d'air retire.
    float releaseSustained(byte &releasedCount);

    void allNotesOff();    // Desactive toutes les notes (MIDI Panic)
    void closeAllServos(); // Ferme tous les servos (position initiale, echelonnee)

    byte getActiveNoteCount() const { return activeCount; }

    // === VOL DE VOIX ===
    // Cherche la note active la moins prioritaire, puis la plus ancienne, dont la priorite
    // est STRICTEMENT inferieure a `maxPriority`. Retourne false si aucun candidat.
    bool findStealCandidate(byte maxPriority, byte &outIndex, byte &outPriority, uint16_t &outSeq) const;
    // Ferme la note d'index donne et retourne le debit d'air retire.
    float releaseIndex(byte index);

private:
    ServoController &servoController; // Reference au controleur de servos
    const NoteConfig *mapping;        // Mapping des servos pour cette main
    byte numNotes;                    // Nombre total de notes
    byte activeCount;                 // Nombre de notes actives

    bool noteStates[MAX_NOTES_PER_HAND];     // true = valve ouverte
    bool sustainedNotes[MAX_NOTES_PER_HAND]; // true = maintenue par la pedale seule
    uint16_t noteSeq[MAX_NOTES_PER_HAND];    // Ordre d'activation (pour le vol de voix)

    void closeIndex(byte index); // Ferme la valve et remet les etats a zero
};

#endif
