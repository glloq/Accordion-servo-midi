// Definition des tables de notes.
//
// Elles sont definies ici, et non dans l'en-tete, pour deux raisons :
//  - une seule copie en FLASH, quel que soit le nombre de fichiers qui les utilisent ;
//  - sur AVR elles sont marquees PROGMEM (voir NOTE_TABLE_STORAGE) et ne consomment donc
//    aucune SRAM. L'acces se fait exclusivement via les accesseurs de noteMapping.h.

#include "noteMapping.h"

// Adresses I2C des 4 PCA9685 (en SRAM : relues en boucle a l'execution, 4 octets).
const uint8_t PCA_TAB[NUM_PCA_TOTAL] = {0x40, 0x41, 0x42, 0x43};

// === MAPPING DES NOTES POUR LA MAIN DROITE (melodie, 34 notes chromatiques) ===
// Fa#3 (54) a Re#6 (87), contigu et chromatique.
// Le airFlowMultiplier decroit avec la hauteur : les graves consomment plus d'air.
const NoteConfig RIGHT_HAND_MAPPING[NUM_NOTES_RIGHT] NOTE_TABLE_STORAGE = {
    // Premier bloc de 18 servos (PCA 0x40 + 2 canaux sur 0x43)
    { 54, 0x40,  0, 1.00f, SERVO_CLOSED_ANGLE, true, NOTE_PRIORITY_MELODY},
    { 55, 0x40,  1, 0.98f, SERVO_CLOSED_ANGLE, true, NOTE_PRIORITY_MELODY},
    { 56, 0x40,  2, 0.96f, SERVO_CLOSED_ANGLE, true, NOTE_PRIORITY_MELODY},
    { 57, 0x40,  3, 0.94f, SERVO_CLOSED_ANGLE, true, NOTE_PRIORITY_MELODY},
    { 58, 0x40,  4, 0.92f, SERVO_CLOSED_ANGLE, true, NOTE_PRIORITY_MELODY},
    { 59, 0x40,  5, 0.90f, SERVO_CLOSED_ANGLE, true, NOTE_PRIORITY_MELODY},
    { 60, 0x40,  6, 0.88f, SERVO_CLOSED_ANGLE, true, NOTE_PRIORITY_MELODY},
    { 61, 0x40,  7, 0.86f, SERVO_CLOSED_ANGLE, true, NOTE_PRIORITY_MELODY},
    { 62, 0x40,  8, 0.84f, SERVO_CLOSED_ANGLE, true, NOTE_PRIORITY_MELODY},
    { 63, 0x40,  9, 0.82f, SERVO_CLOSED_ANGLE, true, NOTE_PRIORITY_MELODY},
    { 64, 0x40, 10, 0.80f, SERVO_CLOSED_ANGLE, true, NOTE_PRIORITY_MELODY},
    { 65, 0x40, 11, 0.78f, SERVO_CLOSED_ANGLE, true, NOTE_PRIORITY_MELODY},
    { 66, 0x40, 12, 0.76f, SERVO_CLOSED_ANGLE, true, NOTE_PRIORITY_MELODY},
    { 67, 0x40, 13, 0.74f, SERVO_CLOSED_ANGLE, true, NOTE_PRIORITY_MELODY},
    { 68, 0x40, 14, 0.72f, SERVO_CLOSED_ANGLE, true, NOTE_PRIORITY_MELODY},
    { 69, 0x40, 15, 0.70f, SERVO_CLOSED_ANGLE, true, NOTE_PRIORITY_MELODY},
    { 70, 0x43, 13, 0.68f, SERVO_CLOSED_ANGLE, true, NOTE_PRIORITY_MELODY},
    { 71, 0x43, 14, 0.66f, SERVO_CLOSED_ANGLE, true, NOTE_PRIORITY_MELODY},

    // Deuxieme bloc de 16 servos (PCA 0x41), montage miroir
    { 72, 0x41,  0, 0.66f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_MELODY},
    { 73, 0x41,  1, 0.66f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_MELODY},
    { 74, 0x41,  2, 0.64f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_MELODY},
    { 75, 0x41,  3, 0.62f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_MELODY},
    { 76, 0x41,  4, 0.60f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_MELODY},
    { 77, 0x41,  5, 0.58f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_MELODY},
    { 78, 0x41,  6, 0.56f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_MELODY},
    { 79, 0x41,  7, 0.54f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_MELODY},
    { 80, 0x41,  8, 0.52f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_MELODY},
    { 81, 0x41,  9, 0.50f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_MELODY},
    { 82, 0x41, 10, 0.48f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_MELODY},
    { 83, 0x41, 11, 0.46f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_MELODY},
    { 84, 0x41, 12, 0.44f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_MELODY},
    { 85, 0x41, 13, 0.42f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_MELODY},
    { 86, 0x41, 14, 0.40f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_MELODY},
    { 87, 0x41, 15, 0.38f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_MELODY}
};

// === MAPPING DES NOTES POUR LA MAIN GAUCHE (basses et accords, 24 notes) ===
// Les numeros MIDI suivent la disposition physique decrite dans le README :
//   Rangee basse (grave) : 36 43 38 45 40 47 42 49 44 51 46 41
//   Rangee aigue         : 48 55 50 57 52 59 54 61 56 63 58 53
// Ces notes ne sont PAS contigues : la recherche se fait par findNoteIndex().
const NoteConfig LEFT_HAND_MAPPING[NUM_NOTES_LEFT] NOTE_TABLE_STORAGE = {
    // Rangee basse (grave) : 12 servos sur le PCA 0x42, montage normal
    { 36, 0x42,  0, 1.0f, SERVO_CLOSED_ANGLE, true, NOTE_PRIORITY_BASS},
    { 43, 0x42,  1, 1.0f, SERVO_CLOSED_ANGLE, true, NOTE_PRIORITY_BASS},
    { 38, 0x42,  2, 1.0f, SERVO_CLOSED_ANGLE, true, NOTE_PRIORITY_BASS},
    { 45, 0x42,  3, 1.0f, SERVO_CLOSED_ANGLE, true, NOTE_PRIORITY_BASS},
    { 40, 0x42,  4, 1.0f, SERVO_CLOSED_ANGLE, true, NOTE_PRIORITY_BASS},
    { 47, 0x42,  5, 1.0f, SERVO_CLOSED_ANGLE, true, NOTE_PRIORITY_BASS},
    { 42, 0x42,  6, 1.0f, SERVO_CLOSED_ANGLE, true, NOTE_PRIORITY_BASS},
    { 49, 0x42,  7, 1.0f, SERVO_CLOSED_ANGLE, true, NOTE_PRIORITY_BASS},
    { 44, 0x42,  8, 1.0f, SERVO_CLOSED_ANGLE, true, NOTE_PRIORITY_BASS},
    { 51, 0x42,  9, 1.0f, SERVO_CLOSED_ANGLE, true, NOTE_PRIORITY_BASS},
    { 46, 0x42, 10, 1.0f, SERVO_CLOSED_ANGLE, true, NOTE_PRIORITY_BASS},
    { 41, 0x42, 11, 1.0f, SERVO_CLOSED_ANGLE, true, NOTE_PRIORITY_BASS},

    // Rangee aigue (accords) : 12 servos, montage miroir
    { 48, 0x42, 12, 1.0f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_CHORD},
    { 55, 0x42, 13, 1.0f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_CHORD},
    { 50, 0x42, 14, 1.0f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_CHORD},
    { 57, 0x42, 15, 1.0f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_CHORD},
    { 52, 0x43,  1, 1.0f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_CHORD},
    { 59, 0x43,  2, 1.0f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_CHORD},
    { 54, 0x43,  3, 1.0f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_CHORD},
    { 61, 0x43,  4, 1.0f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_CHORD},
    { 56, 0x43,  5, 1.0f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_CHORD},
    { 63, 0x43,  6, 1.0f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_CHORD},
    { 58, 0x43,  7, 1.0f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_CHORD},
    { 53, 0x43,  8, 1.0f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_CHORD}
};
