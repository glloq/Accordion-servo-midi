// Tests de la table des notes.
// noteMapping.h ne dependant pas d'Arduino, ce fichier se compile tel quel sur PC.
#include "noteMapping.h"
#include "test_assert.h"

// Le meme fichier est rejoue pour plusieurs sources d'air : chacune reserve des canaux PCA
// differents. Le nom de suite le rappelle dans la sortie.
#if AIR_SOURCE == AIR_SOURCE_BELLOW_SERVO
  #define SUITE_NAME "test_note_mapping (soufflet a servo)"
#elif AIR_SOURCE == AIR_SOURCE_BLOWER_ESC
  #define SUITE_NAME "test_note_mapping (turbine ESC)"
#else
  #define SUITE_NAME "test_note_mapping"
#endif

int main() {
    printf("\n=== Main gauche : disposition du README ===\n");
    {
        // Rangee basse (grave) et rangee aigue, dans l'ordre physique du README.
        const uint8_t bassRow[12]  = {36, 43, 38, 45, 40, 47, 42, 49, 44, 51, 46, 41};
        const uint8_t chordRow[12] = {48, 55, 50, 57, 52, 59, 54, 61, 56, 63, 58, 53};

        bool bassOrderOk = true, chordOrderOk = true;
        for (int i = 0; i < 12; i++) {
            if (LEFT_HAND_MAPPING[i].midiNote != bassRow[i]) bassOrderOk = false;
            if (LEFT_HAND_MAPPING[12 + i].midiNote != chordRow[i]) chordOrderOk = false;
        }
        check("rangee basse conforme au README", bassOrderOk);
        check("rangee aigue conforme au README", chordOrderOk);

        // Ces deux notes etaient injouables avec l'ancien mapping contigu
        // (FIRST_NOTE_LEFT 36 + 24 notes => 36..59).
        check("note 61 jouable", findNoteIndex(LEFT_HAND_MAPPING, NUM_NOTES_LEFT, 61) >= 0);
        check("note 63 jouable", findNoteIndex(LEFT_HAND_MAPPING, NUM_NOTES_LEFT, 63) >= 0);

        // Inversement, l'ancien mapping pretendait jouer ces notes qui n'existent pas.
        check("note 37 absente", findNoteIndex(LEFT_HAND_MAPPING, NUM_NOTES_LEFT, 37) < 0);
        check("note 39 absente", findNoteIndex(LEFT_HAND_MAPPING, NUM_NOTES_LEFT, 39) < 0);
        check("note 60 absente", findNoteIndex(LEFT_HAND_MAPPING, NUM_NOTES_LEFT, 60) < 0);

        bool prioOk = true;
        for (int i = 0; i < 12; i++) {
            if (LEFT_HAND_MAPPING[i].priority != NOTE_PRIORITY_BASS) prioOk = false;
            if (LEFT_HAND_MAPPING[12 + i].priority != NOTE_PRIORITY_CHORD) prioOk = false;
        }
        check("priorites basses > accords", prioOk);
    }

    printf("\n=== Main droite : 34 notes chromatiques ===\n");
    {
        bool contiguous = true;
        for (int i = 0; i < NUM_NOTES_RIGHT; i++) {
            if (RIGHT_HAND_MAPPING[i].midiNote != 54 + i) contiguous = false;
        }
        check("notes 54 a 87 contigues", contiguous);
        check("note 53 hors plage", findNoteIndex(RIGHT_HAND_MAPPING, NUM_NOTES_RIGHT, 53) < 0);
        check("note 88 hors plage", findNoteIndex(RIGHT_HAND_MAPPING, NUM_NOTES_RIGHT, 88) < 0);

        bool prioOk = true;
        for (int i = 0; i < NUM_NOTES_RIGHT; i++) {
            if (RIGHT_HAND_MAPPING[i].priority != NOTE_PRIORITY_MELODY) prioOk = false;
        }
        check("priorite melodie sur toute la main droite", prioOk);
    }

    printf("\n=== Coherence des tables ===\n");
    {
        // Pas de note MIDI dupliquee au sein d'une meme main
        bool leftUnique = true, rightUnique = true;
        for (int i = 0; i < NUM_NOTES_LEFT; i++)
            for (int j = i + 1; j < NUM_NOTES_LEFT; j++)
                if (LEFT_HAND_MAPPING[i].midiNote == LEFT_HAND_MAPPING[j].midiNote) leftUnique = false;
        for (int i = 0; i < NUM_NOTES_RIGHT; i++)
            for (int j = i + 1; j < NUM_NOTES_RIGHT; j++)
                if (RIGHT_HAND_MAPPING[i].midiNote == RIGHT_HAND_MAPPING[j].midiNote) rightUnique = false;
        check("aucune note dupliquee a gauche", leftUnique);
        check("aucune note dupliquee a droite", rightUnique);

        // Aucun canal PCA partage entre deux notes, ni avec la valve generale.
        // Un doublon ici ferait bouger deux anches pour une seule note.
        uint8_t used[NUM_PCA_TOTAL][16];
        for (int p = 0; p < NUM_PCA_TOTAL; p++)
            for (int c = 0; c < 16; c++) used[p][c] = 0;

        bool channelsUnique = true;
        const NoteConfig *tables[2] = {LEFT_HAND_MAPPING, RIGHT_HAND_MAPPING};
        const int counts[2] = {NUM_NOTES_LEFT, NUM_NOTES_RIGHT};

        for (int t = 0; t < 2; t++) {
            for (int i = 0; i < counts[t]; i++) {
                int pcaIndex = -1;
                for (int p = 0; p < NUM_PCA_TOTAL; p++)
                    if (PCA_TAB[p] == tables[t][i].pcaAddress) pcaIndex = p;
                if (pcaIndex < 0 || tables[t][i].channel > 15) { channelsUnique = false; continue; }
                if (used[pcaIndex][tables[t][i].channel]) channelsUnique = false;
                used[pcaIndex][tables[t][i].channel] = 1;
            }
        }
        check("aucun canal PCA partage entre deux notes", channelsUnique);

        // Tout organe pose sur un PCA occupe un canal au meme titre qu'une anche. Les
        // reserver n'a rien de theorique : les valeurs par defaut de la valve, du servo de
        // soufflet et de l'ESC tombaient sur des canaux deja pris par des notes.
#if AIR_VALVE_TYPE == AIR_VALVE_SERVO
        int valvePca = -1;
        for (int p = 0; p < NUM_PCA_TOTAL; p++)
            if (PCA_TAB[p] == VALVE_PCA_ADDRESS) valvePca = p;
        check("canal de la valve generale libre",
              valvePca >= 0 && !used[valvePca][VALVE_PCA_PIN]);
#endif

#if AIR_SOURCE == AIR_SOURCE_BELLOW_SERVO
        check("canal du servo de soufflet libre",
              BELLOW_SERVO_PCA_INDEX < NUM_PCA_TOTAL &&
              !used[BELLOW_SERVO_PCA_INDEX][BELLOW_SERVO_PCA_PIN]);
#endif

#if AIR_SOURCE == AIR_SOURCE_BLOWER_ESC
        check("canal de l'ESC libre",
              ESC_PCA_INDEX < NUM_PCA_TOTAL && !used[ESC_PCA_INDEX][ESC_PCA_PIN]);
#endif

        // Les angles d'ouverture doivent rester dans la plage servo utile.
        bool anglesOk = true;
        for (int t = 0; t < 2; t++) {
            for (int i = 0; i < counts[t]; i++) {
                uint16_t open = openAngleFor(tables[t][i]);
                if (open > 180) anglesOk = false;
                if (tables[t][i].closedPosition > 180) anglesOk = false;
                if (tables[t][i].airFlowMultiplier <= 0.0f) anglesOk = false;
            }
        }
        check("angles et debits dans des plages valides", anglesOk);

        // Les deux sens de montage doivent converger vers le meme angle ouvert.
        NoteConfig normal = {0, PCA_TAB[0], 0, 1.0f, SERVO_CLOSED_ANGLE, true, NOTE_PRIORITY_MELODY};
        NoteConfig mirror = {0, PCA_TAB[0], 0, 1.0f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_MELODY};
        check("montage normal et miroir ouvrent au meme angle",
              openAngleFor(normal) == openAngleFor(mirror));
    }

    return testSummary(SUITE_NAME);
}
