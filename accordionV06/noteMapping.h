#ifndef NOTE_MAPPING_H
#define NOTE_MAPPING_H

// =========================================================================================
// Table des notes <-> servomoteurs
// -----------------------------------------------------------------------------------------
// Ce fichier ne depend QUE de <stdint.h> (et de <avr/pgmspace.h> sur AVR) : il ne doit
// jamais inclure <Arduino.h>. Cela permet de le compiler tel quel sur PC pour les tests
// unitaires natifs (voir test/).
//
// IMPORTANT : chaque entree porte explicitement son numero de note MIDI.
// La disposition Stradella de la main gauche n'est pas chromatique : supposer des notes
// contigues (index = note - firstNote) rendait certaines notes injouables.
//
// STOCKAGE : sur AVR les deux tables vivent en FLASH (PROGMEM). Elles pesaient ~580 octets
// de SRAM sur les 2560 de l'ATmega32U4, ce qui etait intenable avec le transport USB MIDI
// actif. L'acces passe donc obligatoirement par les accesseurs ci-dessous.
// =========================================================================================

#include <stdint.h>

#if defined(ARDUINO_ARCH_AVR)
  #include <avr/pgmspace.h>
  #include <string.h>
  #define NOTE_TABLE_STORAGE PROGMEM
#else
  #define NOTE_TABLE_STORAGE
#endif

// === PRIORITES DE VOIX ===
// Utilisees quand la limite de notes simultanees est atteinte : une note entrante ne peut
// voler la place que d'une note STRICTEMENT moins prioritaire.
#define NOTE_PRIORITY_CHORD  1 // accords main gauche (rangee aigue)
#define NOTE_PRIORITY_MELODY 2 // melodie main droite
#define NOTE_PRIORITY_BASS   3 // basses fondamentales main gauche (rangee grave)

// === PCA9685 ===
#define NUM_PCA_TOTAL 4
extern const uint8_t PCA_TAB[NUM_PCA_TOTAL]; // Adresses I2C, definies dans noteMapping.cpp

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
#define VALVE_PCA_INDEX 3             // Index dans PCA_TAB
#define VALVE_PCA_ADDRESS PCA_TAB[VALVE_PCA_INDEX]
#define VALVE_PCA_PIN 15              // Canal PCA => broche 16 du 4eme PCA
#define VALVE_PCA_ANGLE_OPEN 70
#define VALVE_PCA_ANGLE_CLOSE 120

// === STRUCTURE DE CONFIGURATION D'UNE NOTE ===
struct NoteConfig {
    uint8_t midiNote;          // Numero de note MIDI (explicite, pas d'hypothese de continuite)
    uint8_t pcaAddress;        // Adresse I2C du PCA9685
    uint8_t channel;           // Canal PCA (0-15)
    float   airFlowMultiplier; // Facteur de debit d'air (influence la vitesse du soufflet)
    uint8_t closedPosition;    // Position fermee (angle en degres)
    bool    openDirection;     // Sens d'ouverture (true = normal, false = inverse)
    uint8_t priority;          // Priorite de voix (voir NOTE_PRIORITY_*)
};

extern const NoteConfig RIGHT_HAND_MAPPING[NUM_NOTES_RIGHT];
extern const NoteConfig LEFT_HAND_MAPPING[NUM_NOTES_LEFT];

// =========================================================================================
// Accesseurs. Sur AVR les tables sont en FLASH : tout acces direct `mapping[i].champ`
// lirait de la SRAM a la meme adresse numerique et renverrait n'importe quoi.
// =========================================================================================

inline uint8_t noteNumberAt(const NoteConfig *mapping, uint8_t index) {
#if defined(ARDUINO_ARCH_AVR)
    return pgm_read_byte(&(mapping[index].midiNote));
#else
    return mapping[index].midiNote;
#endif
}

inline uint8_t notePriorityAt(const NoteConfig *mapping, uint8_t index) {
#if defined(ARDUINO_ARCH_AVR)
    return pgm_read_byte(&(mapping[index].priority));
#else
    return mapping[index].priority;
#endif
}

inline float noteAirFlowAt(const NoteConfig *mapping, uint8_t index) {
#if defined(ARDUINO_ARCH_AVR)
    float value;
    memcpy_P(&value, &(mapping[index].airFlowMultiplier), sizeof(float));
    return value;
#else
    return mapping[index].airFlowMultiplier;
#endif
}

// Copie une entree complete depuis la table vers la RAM.
inline void loadNoteConfig(const NoteConfig *mapping, uint8_t index, NoteConfig &out) {
#if defined(ARDUINO_ARCH_AVR)
    memcpy_P(&out, &(mapping[index]), sizeof(NoteConfig));
#else
    out = mapping[index];
#endif
}

// === RECHERCHE D'UNE NOTE DANS UN MAPPING ===
// Retourne l'index de la note, ou -1 si elle n'est pas jouable par cette main.
// Recherche lineaire sur 34 entrees max : negligeable face au debit MIDI (~1 kmsg/s max).
inline int8_t findNoteIndex(const NoteConfig *mapping, uint8_t count, uint8_t midiNote) {
    for (uint8_t i = 0; i < count; i++) {
        if (noteNumberAt(mapping, i) == midiNote) return (int8_t)i;
    }
    return -1;
}

// Angle d'ouverture effectif d'une note, selon le sens de montage du servo.
inline uint16_t openAngleFor(const NoteConfig &config) {
    return config.openDirection ? (uint16_t)(config.closedPosition - SERVO_OPEN_ANGLE)
                                : (uint16_t)(config.closedPosition + SERVO_OPEN_ANGLE);
}

#endif
