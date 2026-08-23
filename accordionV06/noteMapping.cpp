// Definition des tables de notes, a partir des X-macros de config.h.
//
// Elles sont definies ici, et non dans l'en-tete, pour deux raisons :
//  - une seule copie en FLASH, quel que soit le nombre de fichiers qui les utilisent ;
//  - sur AVR elles sont marquees PROGMEM (voir NOTE_TABLE_STORAGE) et ne consomment donc
//    aucune SRAM. L'acces se fait exclusivement via les accesseurs de noteMapping.h.

#include "noteMapping.h"
#include "staticAssert.h"

// Adresses I2C des PCA9685 (en SRAM : relues a l'execution, quelques octets).
const uint8_t PCA_TAB[NUM_PCA_TOTAL] = {PCA_ADDRESS_LIST};

// Developpe une ligne de X-macro en initialiseur de NoteConfig.
#define NOTE_ROW(note, addr, ch, flow, closed, dir, prio) {note, addr, ch, flow, closed, dir, prio},
// ... et en "1," pour compter les lignes sans dupliquer la liste.
#define NOTE_COUNT_ONE(note, addr, ch, flow, closed, dir, prio) 1,

const NoteConfig RIGHT_HAND_MAPPING[NOTE_TABLE_SIZE(NUM_NOTES_RIGHT)] NOTE_TABLE_STORAGE = {
#if NUM_NOTES_RIGHT > 0
    RIGHT_HAND_NOTE_LIST(NOTE_ROW)
#else
    {0, 0, 0, 0.0f, 0, true, 0} // Main droite absente : entree fantome, jamais consultee
#endif
};

const NoteConfig LEFT_HAND_MAPPING[NOTE_TABLE_SIZE(NUM_NOTES_LEFT)] NOTE_TABLE_STORAGE = {
#if NUM_NOTES_LEFT > 0
    LEFT_HAND_NOTE_LIST(NOTE_ROW)
#else
    {0, 0, 0, 0.0f, 0, true, 0} // Main gauche absente : entree fantome, jamais consultee
#endif
};

// =========================================================================================
// Coherence entre les compteurs declares et les listes reellement ecrites.
// Un NUM_NOTES_* faux ne provoque aucune erreur de compilation par lui-meme : il tronque
// silencieusement la table (notes injouables) ou la deborde (lecture hors tableau).
// =========================================================================================
// Ces tableaux ne servent qu'a leur `sizeof`. Ils sont malgre tout places en FLASH : sans
// cela, un tableau const sur AVR occuperait de la SRAM si l'editeur de liens ne l'eliminait
// pas, ce qui n'est pas garanti selon les options de compilation.
#if NUM_NOTES_RIGHT > 0
namespace { const uint8_t rightRowCount[] NOTE_TABLE_STORAGE = {RIGHT_HAND_NOTE_LIST(NOTE_COUNT_ONE)}; }
ACCORDION_STATIC_ASSERT(sizeof(rightRowCount) == NUM_NOTES_RIGHT,
                        NUM_NOTES_RIGHT_ne_correspond_pas_a_RIGHT_HAND_NOTE_LIST);
#endif

#if NUM_NOTES_LEFT > 0
namespace { const uint8_t leftRowCount[] NOTE_TABLE_STORAGE = {LEFT_HAND_NOTE_LIST(NOTE_COUNT_ONE)}; }
ACCORDION_STATIC_ASSERT(sizeof(leftRowCount) == NUM_NOTES_LEFT,
                        NUM_NOTES_LEFT_ne_correspond_pas_a_LEFT_HAND_NOTE_LIST);
#endif

namespace { const uint8_t pcaRowCount[] NOTE_TABLE_STORAGE = {PCA_ADDRESS_LIST}; }
ACCORDION_STATIC_ASSERT(sizeof(pcaRowCount) == NUM_PCA_TOTAL,
                        NUM_PCA_TOTAL_ne_correspond_pas_a_PCA_ADDRESS_LIST);
