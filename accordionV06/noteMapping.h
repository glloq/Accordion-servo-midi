#ifndef NOTE_MAPPING_H
#define NOTE_MAPPING_H

// =========================================================================================
// Table des notes <-> actionneurs
// -----------------------------------------------------------------------------------------
// Le CONTENU des tables vit dans config.h, sous forme de X-macros generees par l'UI de
// configuration. Ce fichier ne definit que la structure d'une entree, les accesseurs, et
// les constantes que les X-macros utilisent.
//
// Ce fichier ne depend QUE de <stdint.h> (et de <avr/pgmspace.h> sur AVR) : il ne doit
// jamais inclure <Arduino.h>. Cela permet de le compiler tel quel sur PC pour les tests
// unitaires natifs (voir test/).
//
// IMPORTANT : chaque entree porte explicitement son numero de note MIDI. La disposition
// Stradella de la main gauche n'est pas chromatique : supposer des notes contigues
// (index = note - firstNote) rendait certaines notes injouables.
//
// STOCKAGE : sur AVR les deux tables vivent en FLASH (PROGMEM). Elles pesaient ~580 octets
// de SRAM sur les 2560 de l'ATmega32U4, ce qui etait intenable avec le transport USB MIDI
// actif. L'acces passe donc obligatoirement par les accesseurs ci-dessous.
// =========================================================================================

#include <stdint.h>
#include "config.h"

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
// NUM_PCA_TOTAL et les adresses viennent de config.h. La table est definie dans
// noteMapping.cpp, qui verifie a la compilation que le compte correspond.
extern const uint8_t PCA_TAB[NUM_PCA_TOTAL];

// Une main peut etre vide (instrument melodie seule, ou basses seules). Un tableau de
// taille nulle n'est pas valide en C++ : la table porte alors une entree fantome, jamais
// consultee puisque HandController recoit numNotes = 0.
#define NOTE_TABLE_SIZE(n) ((n) > 0 ? (n) : 1)

// === STRUCTURE DE CONFIGURATION D'UNE NOTE ===
struct NoteConfig {
    uint8_t midiNote;          // Numero de note MIDI (explicite, pas d'hypothese de continuite)
    uint8_t pcaAddress;        // Adresse I2C du PCA9685
    uint8_t channel;           // Canal PCA (0-15)
    float   airFlowMultiplier; // Facteur de debit d'air (influence la demande d'air)
    uint8_t closedPosition;    // Position fermee (angle en degres)
    bool    openDirection;     // Sens d'ouverture (true = normal, false = miroir)
    uint8_t priority;          // Priorite de voix (voir NOTE_PRIORITY_*)
};

extern const NoteConfig RIGHT_HAND_MAPPING[NOTE_TABLE_SIZE(NUM_NOTES_RIGHT)];
extern const NoteConfig LEFT_HAND_MAPPING[NOTE_TABLE_SIZE(NUM_NOTES_LEFT)];

// === VALVE GENERALE SUR CANAL PCA ===
// Declaree ici avec les notes : quand elle est portee par un PCA9685, elle occupe un canal
// au meme titre qu'une anche, et test_note_mapping verifie qu'aucune note ne lui rentre
// dedans. Meme chose pour le servo de soufflet et le canal ESC.
#if AIR_VALVE_TYPE == AIR_VALVE_SERVO
  #define VALVE_PCA_ADDRESS PCA_TAB[VALVE_PCA_INDEX]
#endif
#if AIR_SOURCE == AIR_SOURCE_BELLOW_SERVO
  #define BELLOW_SERVO_PCA_ADDRESS PCA_TAB[BELLOW_SERVO_PCA_INDEX]
#endif
#if AIR_SOURCE == AIR_SOURCE_BLOWER_ESC
  #define ESC_PCA_ADDRESS PCA_TAB[ESC_PCA_INDEX]
#endif

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
// Recherche lineaire sur quelques dizaines d'entrees : negligeable face au debit MIDI
// (~1 kmsg/s au maximum).
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
