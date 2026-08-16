#ifndef NOTE_MAPPING_H
#define NOTE_MAPPING_H

// =========================================================================================
// Table des notes <-> servomoteurs
// -----------------------------------------------------------------------------------------
// Ce fichier ne depend QUE de <stdint.h> : il ne doit jamais inclure <Arduino.h>.
// Cela permet de le compiler tel quel sur PC pour les tests unitaires natifs
// (voir test/test_note_mapping/).
//
// IMPORTANT : chaque entree porte explicitement son numero de note MIDI.
// L'ancienne version supposait des notes contigues (index = note - firstNote), ce qui
// etait faux pour la main gauche : la disposition Stradella n'est pas chromatique.
// =========================================================================================

#include <stdint.h>

// === PRIORITES DE VOIX ===
// Utilisees quand la limite de notes simultanees est atteinte : une note entrante ne peut
// voler la place que d'une note STRICTEMENT moins prioritaire.
#define NOTE_PRIORITY_CHORD  1 // accords main gauche (rangee aigue)
#define NOTE_PRIORITY_MELODY 2 // melodie main droite
#define NOTE_PRIORITY_BASS   3 // basses fondamentales main gauche (rangee grave)

// === ADRESSES I2C DES 4 PCA9685 ===
#define NUM_PCA_TOTAL 4
const uint8_t PCA_TAB[NUM_PCA_TOTAL] = {0x40, 0x41, 0x42, 0x43};

// === ANGLES DES SERVOS ===
// Les servos sont montes dans deux sens opposes selon les blocs mecaniques :
//  - montage normal  : repos a 130 deg, ouverture a 130 - 40 = 90 deg
//  - montage miroir  : repos a  50 deg, ouverture a  50 + 40 = 90 deg
#define SERVO_CLOSED_ANGLE        130 // Angle de fermeture (valve fermee), montage normal
#define SERVO_CLOSED_ANGLE_MIRROR 50  // Angle de fermeture (valve fermee), montage miroir
#define SERVO_OPEN_ANGLE          40  // Debattement applique pour ouvrir la valve

// === NOMBRE DE NOTES PAR MAIN ===
#define NUM_NOTES_RIGHT 34
#define NUM_NOTES_LEFT  24

// === VALVE GENERALE ===
// Declaree ici avec les notes : elle occupe un canal PCA au meme titre qu'une anche, et
// les tests verifient qu'aucune note ne lui rentre dedans.
#define VALVE_PCA_ADDRESS PCA_TAB[3]  // PCA dedie a la valve
#define VALVE_PCA_PIN 15              // Canal PCA => broche 16 du 4eme PCA
#define VALVE_PCA_ANGLE_OPEN 70
#define VALVE_PCA_ANGLE_CLOSE 120

// === STRUCTURE DE CONFIGURATION D'UNE NOTE ===
struct NoteConfig {
    uint8_t midiNote;        // Numero de note MIDI (explicite, pas d'hypothese de continuite)
    uint8_t pcaAddress;      // Adresse I2C du PCA9685
    uint8_t channel;         // Canal PCA (0-15)
    float   airFlowMultiplier; // Facteur de debit d'air (influence la vitesse du soufflet)
    uint8_t closedPosition;  // Position fermee (angle en degres)
    bool    openDirection;   // Sens d'ouverture (true = normal, false = inverse)
    uint8_t priority;        // Priorite de voix (voir NOTE_PRIORITY_*)
};

// === MAPPING DES NOTES POUR LA MAIN DROITE (melodie, 34 notes chromatiques) ===
// Fa#3 (54) a Re#6 (87), contigu et chromatique.
// Le airFlowMultiplier decroit avec la hauteur : les graves consomment plus d'air.
const NoteConfig RIGHT_HAND_MAPPING[NUM_NOTES_RIGHT] = {
    // Premier bloc de 18 servos (PCA 0x40 + 2 canaux sur 0x43)
    { 54, PCA_TAB[0],  0, 1.00f, SERVO_CLOSED_ANGLE, true, NOTE_PRIORITY_MELODY},
    { 55, PCA_TAB[0],  1, 0.98f, SERVO_CLOSED_ANGLE, true, NOTE_PRIORITY_MELODY},
    { 56, PCA_TAB[0],  2, 0.96f, SERVO_CLOSED_ANGLE, true, NOTE_PRIORITY_MELODY},
    { 57, PCA_TAB[0],  3, 0.94f, SERVO_CLOSED_ANGLE, true, NOTE_PRIORITY_MELODY},
    { 58, PCA_TAB[0],  4, 0.92f, SERVO_CLOSED_ANGLE, true, NOTE_PRIORITY_MELODY},
    { 59, PCA_TAB[0],  5, 0.90f, SERVO_CLOSED_ANGLE, true, NOTE_PRIORITY_MELODY},
    { 60, PCA_TAB[0],  6, 0.88f, SERVO_CLOSED_ANGLE, true, NOTE_PRIORITY_MELODY},
    { 61, PCA_TAB[0],  7, 0.86f, SERVO_CLOSED_ANGLE, true, NOTE_PRIORITY_MELODY},
    { 62, PCA_TAB[0],  8, 0.84f, SERVO_CLOSED_ANGLE, true, NOTE_PRIORITY_MELODY},
    { 63, PCA_TAB[0],  9, 0.82f, SERVO_CLOSED_ANGLE, true, NOTE_PRIORITY_MELODY},
    { 64, PCA_TAB[0], 10, 0.80f, SERVO_CLOSED_ANGLE, true, NOTE_PRIORITY_MELODY},
    { 65, PCA_TAB[0], 11, 0.78f, SERVO_CLOSED_ANGLE, true, NOTE_PRIORITY_MELODY},
    { 66, PCA_TAB[0], 12, 0.76f, SERVO_CLOSED_ANGLE, true, NOTE_PRIORITY_MELODY},
    { 67, PCA_TAB[0], 13, 0.74f, SERVO_CLOSED_ANGLE, true, NOTE_PRIORITY_MELODY},
    { 68, PCA_TAB[0], 14, 0.72f, SERVO_CLOSED_ANGLE, true, NOTE_PRIORITY_MELODY},
    { 69, PCA_TAB[0], 15, 0.70f, SERVO_CLOSED_ANGLE, true, NOTE_PRIORITY_MELODY},
    { 70, PCA_TAB[3], 13, 0.68f, SERVO_CLOSED_ANGLE, true, NOTE_PRIORITY_MELODY},
    { 71, PCA_TAB[3], 14, 0.66f, SERVO_CLOSED_ANGLE, true, NOTE_PRIORITY_MELODY},

    // Deuxieme bloc de 16 servos (PCA 0x41), montage miroir
    { 72, PCA_TAB[1],  0, 0.66f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_MELODY},
    { 73, PCA_TAB[1],  1, 0.66f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_MELODY},
    { 74, PCA_TAB[1],  2, 0.64f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_MELODY},
    { 75, PCA_TAB[1],  3, 0.62f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_MELODY},
    { 76, PCA_TAB[1],  4, 0.60f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_MELODY},
    { 77, PCA_TAB[1],  5, 0.58f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_MELODY},
    { 78, PCA_TAB[1],  6, 0.56f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_MELODY},
    { 79, PCA_TAB[1],  7, 0.54f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_MELODY},
    { 80, PCA_TAB[1],  8, 0.52f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_MELODY},
    { 81, PCA_TAB[1],  9, 0.50f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_MELODY},
    { 82, PCA_TAB[1], 10, 0.48f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_MELODY},
    { 83, PCA_TAB[1], 11, 0.46f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_MELODY},
    { 84, PCA_TAB[1], 12, 0.44f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_MELODY},
    { 85, PCA_TAB[1], 13, 0.42f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_MELODY},
    { 86, PCA_TAB[1], 14, 0.40f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_MELODY},
    { 87, PCA_TAB[1], 15, 0.38f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_MELODY}
};

// === MAPPING DES NOTES POUR LA MAIN GAUCHE (basses et accords, 24 notes) ===
// Les numeros MIDI suivent la disposition physique decrite dans le README :
//   Rangee basse (grave) : 36 43 38 45 40 47 42 49 44 51 46 41
//   Rangee aigue         : 48 55 50 57 52 59 54 61 56 63 58 53
// Ces notes ne sont PAS contigues : la recherche se fait par findNoteIndex().
const NoteConfig LEFT_HAND_MAPPING[NUM_NOTES_LEFT] = {
    // Rangee basse (grave) : 12 servos sur le PCA 0x42, montage normal
    { 36, PCA_TAB[2],  0, 1.0f, SERVO_CLOSED_ANGLE, true, NOTE_PRIORITY_BASS},
    { 43, PCA_TAB[2],  1, 1.0f, SERVO_CLOSED_ANGLE, true, NOTE_PRIORITY_BASS},
    { 38, PCA_TAB[2],  2, 1.0f, SERVO_CLOSED_ANGLE, true, NOTE_PRIORITY_BASS},
    { 45, PCA_TAB[2],  3, 1.0f, SERVO_CLOSED_ANGLE, true, NOTE_PRIORITY_BASS},
    { 40, PCA_TAB[2],  4, 1.0f, SERVO_CLOSED_ANGLE, true, NOTE_PRIORITY_BASS},
    { 47, PCA_TAB[2],  5, 1.0f, SERVO_CLOSED_ANGLE, true, NOTE_PRIORITY_BASS},
    { 42, PCA_TAB[2],  6, 1.0f, SERVO_CLOSED_ANGLE, true, NOTE_PRIORITY_BASS},
    { 49, PCA_TAB[2],  7, 1.0f, SERVO_CLOSED_ANGLE, true, NOTE_PRIORITY_BASS},
    { 44, PCA_TAB[2],  8, 1.0f, SERVO_CLOSED_ANGLE, true, NOTE_PRIORITY_BASS},
    { 51, PCA_TAB[2],  9, 1.0f, SERVO_CLOSED_ANGLE, true, NOTE_PRIORITY_BASS},
    { 46, PCA_TAB[2], 10, 1.0f, SERVO_CLOSED_ANGLE, true, NOTE_PRIORITY_BASS},
    { 41, PCA_TAB[2], 11, 1.0f, SERVO_CLOSED_ANGLE, true, NOTE_PRIORITY_BASS},

    // Rangee aigue (accords) : 12 servos, montage miroir
    { 48, PCA_TAB[2], 12, 1.0f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_CHORD},
    { 55, PCA_TAB[2], 13, 1.0f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_CHORD},
    { 50, PCA_TAB[2], 14, 1.0f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_CHORD},
    { 57, PCA_TAB[2], 15, 1.0f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_CHORD},
    { 52, PCA_TAB[3],  1, 1.0f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_CHORD},
    { 59, PCA_TAB[3],  2, 1.0f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_CHORD},
    { 54, PCA_TAB[3],  3, 1.0f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_CHORD},
    { 61, PCA_TAB[3],  4, 1.0f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_CHORD},
    { 56, PCA_TAB[3],  5, 1.0f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_CHORD},
    { 63, PCA_TAB[3],  6, 1.0f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_CHORD},
    { 58, PCA_TAB[3],  7, 1.0f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_CHORD},
    { 53, PCA_TAB[3],  8, 1.0f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_CHORD}
};

// === RECHERCHE D'UNE NOTE DANS UN MAPPING ===
// Retourne l'index de la note, ou -1 si elle n'est pas jouable par cette main.
// Recherche lineaire sur 34 entrees max : negligeable face au debit MIDI (~1 kmsg/s max).
inline int8_t findNoteIndex(const NoteConfig *mapping, uint8_t count, uint8_t midiNote) {
    for (uint8_t i = 0; i < count; i++) {
        if (mapping[i].midiNote == midiNote) return (int8_t)i;
    }
    return -1;
}

// Angle d'ouverture effectif d'une note, selon le sens de montage du servo.
inline uint16_t openAngleFor(const NoteConfig &config) {
    return config.openDirection ? (uint16_t)(config.closedPosition - SERVO_OPEN_ANGLE)
                                : (uint16_t)(config.closedPosition + SERVO_OPEN_ANGLE);
}

#endif
