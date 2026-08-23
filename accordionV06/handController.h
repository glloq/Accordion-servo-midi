#ifndef HANDCONTROLLER_H
#define HANDCONTROLLER_H

#include "settings.h"
#include "servoController.h"

// Comparaison d'anciennete resistante au rebouclage du compteur d'activation.
// `a` est plus ancien que `b` si la difference signee est negative : cela reste vrai
// apres le passage de 65535 a 0, contrairement a un simple `a < b`.
inline bool seqIsOlder(uint16_t a, uint16_t b) {
    return (int16_t)(a - b) < 0;
}

// Classe generique pour controler les deux mains (gauche et droite).
//
// La velocite n'est volontairement pas traitee ici : une valve d'anche est ouverte ou
// fermee, il n'y a pas de nuance possible cote actionneur. La dynamique est produite par le
// debit d'air (voir la source d'air).
//
// Le type d'actionneur (servo ou electroaimant) est choisi dans config.h et resolu a la
// compilation : la classe expose la meme interface dans les deux cas.
class HandController {
public:
    // Constructeur generique pour gerer une main avec un mapping specifique.
    // `numNotes` peut valoir 0 : la main est alors inexistante et tout devient inoperant,
    // ce qui permet un instrument melodie seule ou basses seules sans code conditionnel.
    HandController(ServoController &servoCtrl, const NoteConfig *mapping, byte numNotes);

    // Entretien periodique. Sans objet pour des servos ; pour des electroaimants, c'est ici
    // que le courant d'appel retombe au courant de maintien.
    void update();

    bool canPlay(byte note) const;      // La note existe-t-elle dans le mapping ?
    bool isNoteActive(byte note) const; // La note est-elle deja ouverte ?
    byte priorityOf(byte note) const;   // Priorite de voix de la note (0 si inconnue)

    // Ouvre la valve de la note. `seq` est un compteur global croissant servant a
    // departager les candidats au vol de voix (la plus ancienne part en premier).
    // `velocity` est memorisee par note : elle pondere la demande d'air de CETTE note.
    // Retourne le debit d'air a ajouter, ou 0 si la note est inconnue / deja active.
    float noteOn(byte note, byte velocity, uint16_t seq);

    // Ferme reellement la valve. Retourne le debit d'air a retirer, 0 si rien a faire.
    float noteOff(byte note);

    // === DEMANDE D'AIR ===
    // Somme des debits des notes ouvertes, chacune ponderee par SA propre velocite.
    // Recalculee a chaque evenement de note plutot qu'accumulee : pas de derive, et la
    // velocite d'une note n'affecte plus le debit des autres.
    float weightedAirFlow() const;
    float airFlowOfIndex(byte index) const;
    byte  velocityOfIndex(byte index) const;
    int8_t indexOf(byte note) const { return findNoteIndex(mapping, numNotes, note); }

    // Sustain : la note reste ouverte mais est marquee comme relachee par le clavier.
    void markSustained(byte note);
    // Ferme toutes les notes retenues uniquement par la pedale.
    // `releasedCount` recoit le nombre de notes fermees. Retourne le debit d'air retire.
    float releaseSustained(byte &releasedCount);

    void allNotesOff();    // Desactive toutes les notes (MIDI Panic)
    void closeAllServos(); // Ferme tous les actionneurs (position initiale, echelonnee)

    byte getActiveNoteCount() const { return activeCount; }
    byte getNoteCount() const { return numNotes; }

    // === VOL DE VOIX ===
    // Cherche la note active la moins prioritaire, puis la plus ancienne, dont la priorite
    // est STRICTEMENT inferieure a `maxPriority`. Retourne false si aucun candidat.
    bool findStealCandidate(byte maxPriority, byte &outIndex, byte &outPriority, uint16_t &outSeq) const;
    // Ferme la note d'index donne et retourne le debit d'air retire.
    float releaseIndex(byte index);

private:
    ServoController &servoController; // Reference au controleur d'actionneurs
    const NoteConfig *mapping;        // Mapping des actionneurs pour cette main
    byte numNotes;                    // Nombre total de notes
    byte activeCount;                 // Nombre de notes actives

    bool noteStates[MAX_NOTES_PER_HAND];     // true = valve ouverte
    bool sustainedNotes[MAX_NOTES_PER_HAND]; // true = maintenue par la pedale seule
    uint16_t noteSeq[MAX_NOTES_PER_HAND];    // Ordre d'activation (pour le vol de voix)
    byte noteVelocity[MAX_NOTES_PER_HAND];   // Velocite MIDI de chaque note ouverte

#if NOTE_ACTUATOR == ACTUATOR_SOLENOID
    // Renfort d'appel : un electroaimant maintenu a pleine puissance chauffe. Une seule
    // note est suivie a la fois, la derniere declenchee — comme le supplement de debit
    // d'attaque. Suivre une echeance par note couterait 4 octets de SRAM par note, soit
    // davantage que tout le reste de l'etat de la main.
    int8_t pullInIndex;
    uint32_t pullInSince;
    void endPullIn(); // Ramene la note en cours d'appel au courant de maintien
#endif

    // Applique l'etat d'une valve, quel que soit le type d'actionneur configure.
    void applyValve(const NoteConfig &config, bool open);
    void closeIndex(byte index); // Ferme la valve et remet les etats a zero
};

#endif
