// =========================================================================================
// Tests des variantes de configuration structurantes autres que la source d'air.
//
// Compile plusieurs fois avec des definitions differentes (voir le Makefile) : routage MIDI
// par point de partage, routage fusionne, actionneurs a electroaimant, une broche OE par
// PCA. Ces options changent
// du code compile conditionnellement — les tester une seule fois dans la configuration par
// defaut reviendrait a ne pas les tester du tout.
// =========================================================================================
#include <cmath>
#include "harness.h"
#include "instrument.h"
#include "test_assert.h"

#define LOOP_US 40

static void runLoop(Instrument &inst, unsigned long durationMs, unsigned long loopUs = LOOP_US) {
    unsigned long end = stubMicros + durationMs * 1000UL;
    while (stubMicros < end) {
        unsigned long before = stubMicros;
        inst.update();
        unsigned long spent = stubMicros - before;
        stubMicros += (spent < loopUs) ? (loopUs - spent) : 1;
    }
}

static void bringUp(Instrument &inst) {
    stubResetMachine(30.0f, 0.0f, BELLOW_MAX_POSITION + 5.0f);
    inst.begin();
    runLoop(inst, 60000, 200);
}

// -----------------------------------------------------------------------------------------
#if MIDI_ROUTING == MIDI_ROUTING_SPLIT
// -----------------------------------------------------------------------------------------
static const char *SUITE = "test_variant_routing_split";

static void run() {
    printf("\n=== Routage par point de partage : un seul canal MIDI suffit ===\n");
    Instrument inst;
    bringUp(inst);
    check("instrument pret", inst.getState() == SYS_READY);

    // Tout arrive sur le canal 1, y compris des notes de main droite : c'est le cas d'un
    // fichier MIDI mono-canal, que le routage par canal rendait injouable.
    inst.noteOn(36, 100, 1);  // < MIDI_SPLIT_NOTE -> main gauche
    inst.noteOn(72, 100, 1);  // >= MIDI_SPLIT_NOTE -> main droite
    runLoop(inst, 50);
    check("les deux notes sont jouees depuis le meme canal", inst.getActiveNoteCount() == 2);

    // Le canal est ignore : la meme note sur un autre canal doit atteindre la meme main,
    // sinon un NoteOff sur un canal different laisserait l'anche ouverte.
    inst.noteOff(36, 10);
    inst.noteOff(72, 10);
    runLoop(inst, 50);
    check("NoteOff accepte quel que soit le canal", inst.getActiveNoteCount() == 0);

    // Une note qui tombe du bon cote du partage mais absente du mapping reste ignoree.
    inst.noteOn(30, 100, 1);
    runLoop(inst, 50);
    check("note hors mapping ignoree", inst.getActiveNoteCount() == 0);
}

// -----------------------------------------------------------------------------------------
#elif MIDI_ROUTING == MIDI_ROUTING_MERGE
// -----------------------------------------------------------------------------------------
static const char *SUITE = "test_variant_routing_merge";

static void run() {
    printf("\n=== Routage fusionne : la main droite est essayee en premier ===\n");
    Instrument inst;
    bringUp(inst);
    check("instrument pret", inst.getState() == SYS_READY);

    // La note 54 existe dans LES DEUX mappings (Fa#3 melodie, et accord Stradella). Le
    // routage doit donc etre deterministe, sinon un NoteOff pourrait viser l'autre main et
    // laisser une anche ouverte indefiniment.
    inst.noteOn(54, 100, 5);
    runLoop(inst, 50);
    check("note ambigue jouee une seule fois", inst.getActiveNoteCount() == 1);
    inst.noteOff(54, 9); // Canal different a dessein
    runLoop(inst, 50);
    check("refermee malgre le changement de canal", inst.getActiveNoteCount() == 0);

    // Une note connue de la seule main gauche doit malgre tout etre jouee.
    inst.noteOn(36, 100, 7);
    runLoop(inst, 50);
    check("note exclusive a la main gauche jouee", inst.getActiveNoteCount() == 1);
    inst.noteOff(36, 7);

    inst.noteOn(30, 100, 7);
    runLoop(inst, 50);
    check("note inconnue des deux mains ignoree", inst.getActiveNoteCount() == 0);
}

// -----------------------------------------------------------------------------------------
#elif NOTE_ACTUATOR == ACTUATOR_SOLENOID
// -----------------------------------------------------------------------------------------
static const char *SUITE = "test_variant_solenoid";

static void run() {
    printf("\n=== Electroaimants : appel pleine puissance puis maintien reduit ===\n");
    Instrument inst;
    bringUp(inst);
    check("instrument pret", inst.getState() == SYS_READY);

    // L'interet du maintien reduit est de ne pas bruler la bobine : ce qui compte est
    // qu'une commande SUPPLEMENTAIRE parte apres SOLENOID_PULLIN_MS, sans nouvel evenement
    // MIDI. Sans elle, l'electroaimant resterait a pleine puissance tant que la note dure.
    unsigned long before = stubI2cWrites;
    inst.noteOn(60, 100, MIDI_CHANNEL_RIGHT);
    unsigned long afterOpen = stubI2cWrites;
    check("l'ouverture commande l'actionneur", afterOpen > before);

    runLoop(inst, SOLENOID_PULLIN_MS / 2);
    check("pas encore de retombee avant l'echeance", stubI2cWrites == afterOpen);

    runLoop(inst, SOLENOID_PULLIN_MS + 20);
    check("retombee au courant de maintien", stubI2cWrites > afterOpen);

    // Une fois retombee, plus rien ne doit etre reemis : une commande par tour de boucle
    // saturerait le bus et ferait chuter la frequence de pas.
    unsigned long afterHold = stubI2cWrites;
    runLoop(inst, 500);
    check("aucune commande repetee ensuite", stubI2cWrites == afterHold);

    inst.noteOff(60, MIDI_CHANNEL_RIGHT);
    runLoop(inst, 50);
    check("note refermee", inst.getActiveNoteCount() == 0);

    // Deux electroaimants a pleine puissance en meme temps doublent l'appel de courant :
    // la note precedente doit retomber immediatement quand une nouvelle arrive.
    inst.noteOn(61, 100, MIDI_CHANNEL_RIGHT);
    unsigned long beforeSecond = stubI2cWrites;
    inst.noteOn(62, 100, MIDI_CHANNEL_RIGHT);
    printf("     (commandes emises pour la 2e note : %lu)\n", stubI2cWrites - beforeSecond);
    check("la note precedente retombe des la suivante",
          (stubI2cWrites - beforeSecond) >= 2);

    inst.allNotesOff();
    runLoop(inst, 50);
    check("MIDI Panic referme tout", inst.getActiveNoteCount() == 0);
}

// -----------------------------------------------------------------------------------------
#elif PCA_OE_MODE == PCA_OE_PER_PCA
// -----------------------------------------------------------------------------------------
static const char *SUITE = "test_variant_oe_par_pca";

static const uint8_t OE_PINS[NUM_PCA_TOTAL] = {PCA_OE_PIN_LIST};

static void run() {
    printf("\n=== OE par PCA : couper les anches sans couper la valve ===\n");
    Instrument inst;
    bringUp(inst);
    check("instrument pret", inst.getState() == SYS_READY);

    // Au repos, les bancs d'anches sont coupes : les servos cessent de forcer et de
    // grincer. C'est tout l'interet du cablage separe — avec un OE partage, la valve
    // generale etait coupee en meme temps et retombait de sa position.
    runLoop(inst, PCA_DISABLE_DELAY + 500);
    bool anchesCut = true;
    for (int i = 0; i < NUM_PCA_TOTAL; i++) {
        if (i == VALVE_PCA_INDEX) continue;
        if (stubPinState[OE_PINS[i]] != HIGH) anchesCut = false;
    }
    check("bancs d'anches coupes au repos", anchesCut);
    check("banc de la valve generale toujours alimente",
          stubPinState[OE_PINS[VALVE_PCA_INDEX]] == LOW);

    // Une commande envoyee a un banc coupe doit le reactiver : sinon elle partirait dans le
    // vide et l'anche resterait fermee sans que rien ne le signale.
    // La note 60 est sur le premier PCA, la note 72 sur le deuxieme.
    inst.noteOn(60, 100, MIDI_CHANNEL_RIGHT);
    runLoop(inst, 50);
    check("le banc de la note jouee se reactive",
          stubPinState[OE_PINS[0]] == LOW && inst.getActiveNoteCount() == 1);
    check("les bancs sans note active restent coupes", stubPinState[OE_PINS[1]] == HIGH);

    inst.noteOn(72, 100, MIDI_CHANNEL_RIGHT);
    runLoop(inst, 50);
    check("un second banc se reactive a son tour",
          stubPinState[OE_PINS[1]] == LOW && inst.getActiveNoteCount() == 2);

    inst.allNotesOff();
    runLoop(inst, 50);
    check("MIDI Panic referme tout", inst.getActiveNoteCount() == 0);
}

#else
#error "test_variants doit etre compile avec une variante explicite (routage, actionneur, OE)."
#endif

int main() {
    run();
    return testSummary(SUITE);
}
