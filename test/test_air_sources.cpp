// =========================================================================================
// Tests des sources d'air autres que le soufflet pas a pas.
//
// Ce fichier est compile UNE FOIS PAR SOURCE (voir le Makefile) : la source testee est
// choisie par -DAIR_SOURCE=..., exactement comme le firmware embarque. Il n'y a donc jamais
// deux pilotes dans le meme binaire, et le test verifie la configuration reellement livree.
//
// Ce qui est verifie pour toutes :
//   - l'instrument devient READY et refuse les notes avant,
//   - une note produit reellement une commande, et zero note l'annule,
//   - CC7 = 0 coupe la pression,
//   - un defaut coupe la puissance et ouvre la valve.
//
// Puis les proprietes specifiques : oscillation du soufflet a servo, a-coup de demarrage de
// la turbine, armement de l'ESC, modulation et protection thermique de la pompe.
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

// Aucune de ces sources n'a de fin de course : on place les contacts hors d'atteinte pour
// que le stub ne les declenche jamais.
static void setupMachine() {
    stubResetMachine(0.0f, -1e9f, 1e9f);
}

static void bringUp(Instrument &inst) {
    inst.begin();
    runLoop(inst, 6000, 200); // Couvre l'armement d'un ESC (2,5 s) avec de la marge
}

// -----------------------------------------------------------------------------------------
#if AIR_SOURCE == AIR_SOURCE_BELLOW_SERVO
// -----------------------------------------------------------------------------------------
static const char *SUITE = "test_air_source_servo_bellow";

static void runSpecific() {
    printf("\n=== Soufflet a servo : pas de calibration, oscillation, arret ===\n");
    setupMachine();
    Instrument inst;
    inst.begin();
    // Un servo connait sa position : rien a chercher. L'instrument est utilisable des le
    // premier tour de boucle, contrairement au soufflet pas a pas.
    runLoop(inst, 50);
    check("pret sans calibration", inst.getState() == SYS_READY);

    ServoBellowController &src = inst.getAirSource();
    check("soufflet ferme au repos", fabs(src.getOpening()) < 0.01f);
    check("immobile sans note", !src.isMoving());

    inst.noteOn(60, 100, MIDI_CHANNEL_RIGHT);
    check("mise en mouvement sur NoteOn", src.isMoving());

    // Balayage : le soufflet doit s'ouvrir, puis repartir dans l'autre sens avant la butee.
    // A la vitesse minimale (une note douce), une course complete prend une vingtaine de
    // secondes — comme le soufflet pas a pas a NORMAL_SPEED. La fenetre d'observation doit
    // donc couvrir plusieurs dizaines de secondes simulees.
    float maxOpening = 0.0f, minOpening = 1.0f;
    bool reversed = false;
    float previous = src.getOpening();
    for (int i = 0; i < 600; i++) {
        runLoop(inst, 100);
        float opening = src.getOpening();
        if (opening > maxOpening) maxOpening = opening;
        if (opening < minOpening) minOpening = opening;
        if (opening < previous - 1e-4f) reversed = true;
        previous = opening;
    }
    printf("     (ouverture parcourue : %.2f a %.2f)\n", minOpening, maxOpening);
    check("le soufflet s'ouvre reellement", maxOpening > 0.5f);
    check("inversion avant la butee d'angle", reversed && maxOpening <= 1.0f);
    check("ne depasse pas le seuil haut", maxOpening <= BELLOW_SERVO_REVERSE_OPEN + 0.05f);

    // CC7 = 0 doit reellement arreter le balayage.
    inst.setVolume(0);
    runLoop(inst, 100);
    float frozen = src.getAngle();
    runLoop(inst, 500);
    check("CC7 = 0 arrete le balayage", !src.isMoving() && fabs(src.getAngle() - frozen) < 0.01f);

    inst.setVolume(100);
    runLoop(inst, 200);
    check("le balayage repart quand CC7 remonte", src.isMoving());

    inst.getAirSource().setFault(FAULT_OVERPRESSURE);
    runLoop(inst, 50);
    check("defaut propage a l'instrument", inst.getState() == SYS_FAULT);
    check("cause conservee", inst.getFault() == FAULT_OVERPRESSURE);
}

// -----------------------------------------------------------------------------------------
#elif AIR_SOURCE == AIR_SOURCE_BLOWER_PWM
// -----------------------------------------------------------------------------------------
static const char *SUITE = "test_air_source_blower_pwm";

static void runSpecific() {
    printf("\n=== Turbine PWM : a-coup de demarrage, duty proportionnel, arret ===\n");
    setupMachine();
    Instrument inst;
    bringUp(inst);
    check("pret sans calibration de position", inst.getState() == SYS_READY);
    check("turbine a l'arret sans note", stubAnalogOut[BLOWER_PWM_PIN] == BLOWER_DUTY_IDLE);

    // Un moteur a l'arret ne demarre pas au duty minimal : le firmware doit passer par un
    // a-coup avant de retomber sur la commande utile.
    inst.noteOn(60, 100, MIDI_CHANNEL_RIGHT);
    runLoop(inst, 10);
    printf("     (duty juste apres NoteOn : %d)\n", stubAnalogOut[BLOWER_PWM_PIN]);
    check("a-coup de demarrage applique", stubAnalogOut[BLOWER_PWM_PIN] == BLOWER_SPINUP_DUTY);

    runLoop(inst, BLOWER_SPINUP_MS + 50);
    int oneNote = stubAnalogOut[BLOWER_PWM_PIN];
    printf("     (duty regime etabli, une note : %d)\n", oneNote);
    check("retombee sur la commande utile", oneNote < BLOWER_SPINUP_DUTY && oneNote >= BLOWER_DUTY_MIN);

    // Plusieurs anches ouvertes consomment plus d'air : le duty doit monter.
    for (byte n = 61; n <= 66; n++) inst.noteOn(n, 100, MIDI_CHANNEL_RIGHT);
    runLoop(inst, 200);
    int manyNotes = stubAnalogOut[BLOWER_PWM_PIN];
    printf("     (duty a 7 notes : %d)\n", manyNotes);
    check("le duty croit avec le nombre d'anches", manyNotes > oneNote);
    check("duty borne au maximum", manyNotes <= BLOWER_DUTY_MAX);

    inst.setVolume(0);
    runLoop(inst, 100);
    check("CC7 = 0 coupe la turbine", stubAnalogOut[BLOWER_PWM_PIN] == BLOWER_DUTY_IDLE);
    inst.setVolume(100);

    inst.allNotesOff();
    runLoop(inst, 100);
    check("plus aucune note : retour au ralenti", stubAnalogOut[BLOWER_PWM_PIN] == BLOWER_DUTY_IDLE);

    inst.getAirSource().setFault(FAULT_OVERPRESSURE);
    runLoop(inst, 50);
    check("defaut : turbine coupee", stubAnalogOut[BLOWER_PWM_PIN] == 0);
    check("defaut propage a l'instrument", inst.getState() == SYS_FAULT);
}

// -----------------------------------------------------------------------------------------
#elif AIR_SOURCE == AIR_SOURCE_BLOWER_ESC
// -----------------------------------------------------------------------------------------
static const char *SUITE = "test_air_source_blower_esc";

static void runSpecific() {
    printf("\n=== Turbine brushless : armement obligatoire, puis gaz proportionnels ===\n");
    setupMachine();
    Instrument inst;
    inst.begin();

    // Un ESC non arme ignore toute consigne. Tant que la sequence n'est pas terminee,
    // l'instrument doit refuser les notes plutot que de faire semblant de jouer.
    runLoop(inst, 200);
    check("armement en cours", inst.getState() == SYS_HOMING);
    inst.noteOn(60, 100, MIDI_CHANNEL_RIGHT);
    check("NoteOn refusee pendant l'armement", inst.getActiveNoteCount() == 0);

    runLoop(inst, ESC_ARM_MS + 500, 200);
    check("pret apres l'armement", inst.getState() == SYS_READY);

    unsigned long before = stubI2cWrites;
    inst.noteOn(60, 100, MIDI_CHANNEL_RIGHT);
    runLoop(inst, 100);
    check("le canal ESC est bien commande", stubI2cWrites > before);
    float oneNote = inst.getAirSource().getCommand();
    printf("     (commande a une note : %.3f)\n", oneNote);
    check("commande non nulle a une note", oneNote > 0.0f);

    for (byte n = 61; n <= 66; n++) inst.noteOn(n, 100, MIDI_CHANNEL_RIGHT);
    runLoop(inst, 100);
    float manyNotes = inst.getAirSource().getCommand();
    printf("     (commande a 7 notes : %.3f)\n", manyNotes);
    check("les gaz croissent avec le nombre d'anches", manyNotes > oneNote);
    check("commande saturee a 1.0", manyNotes <= 1.0f);

    inst.allNotesOff();
    runLoop(inst, 100);
    check("plus aucune note : gaz coupes", inst.getAirSource().getCommand() == 0.0f);

    inst.getAirSource().setFault(FAULT_OVERPRESSURE);
    runLoop(inst, 50);
    check("defaut propage a l'instrument", inst.getState() == SYS_FAULT);
}

// -----------------------------------------------------------------------------------------
#elif AIR_SOURCE == AIR_SOURCE_PUMP_ONOFF
// -----------------------------------------------------------------------------------------
static const char *SUITE = "test_air_source_pump";

// Mesure le rapport cyclique reellement produit sur la broche de la pompe.
static float measureDuty(Instrument &inst, unsigned long durationMs) {
    unsigned long end = stubMicros + durationMs * 1000UL;
    unsigned long onUs = 0, totalUs = 0;
    while (stubMicros < end) {
        unsigned long before = stubMicros;
        inst.update();
        unsigned long spent = stubMicros - before;
        unsigned long step = (spent < LOOP_US) ? LOOP_US : spent;
        stubMicros = before + step;
        bool on = (stubPinState[PUMP_PIN] == (PUMP_ACTIVE_LEVEL ? HIGH : LOW));
        if (on) onUs += step;
        totalUs += step;
    }
    return totalUs ? ((float)onUs / (float)totalUs) : 0.0f;
}

static void runSpecific() {
    printf("\n=== Pompe tout-ou-rien : modulation lente et protection thermique ===\n");
    setupMachine();
    Instrument inst;
    bringUp(inst);
    check("pret sans calibration de position", inst.getState() == SYS_READY);

    float idle = measureDuty(inst, 1000);
    check("pompe a l'arret sans note", idle < 0.01f);

    inst.noteOn(60, 100, MIDI_CHANNEL_RIGHT);
    float oneNote = measureDuty(inst, 2000);
    printf("     (rapport cyclique a une note : %.2f)\n", oneNote);
    check("la pompe module sans tourner en continu",
          oneNote > 0.05f && oneNote < 0.99f);

    for (byte n = 61; n <= 66; n++) inst.noteOn(n, 100, MIDI_CHANNEL_RIGHT);
    float manyNotes = measureDuty(inst, 2000);
    printf("     (rapport cyclique a 7 notes : %.2f)\n", manyNotes);
    check("le rapport cyclique croit avec le nombre d'anches", manyNotes > oneNote);

    // Protection thermique : au-dela de PUMP_MAX_RUN_MS de marche continue, la pompe doit
    // s'arreter meme si la demande reste maximale. Une pompe a membrane n'a pas de service
    // continu ; la laisser tourner la detruit.
    float longRun = measureDuty(inst, PUMP_MAX_RUN_MS + PUMP_REST_MS + 2000);
    printf("     (rapport cyclique moyen sur une longue tenue : %.2f)\n", longRun);
    check("repos force impose sur une longue tenue", longRun < 0.99f);

    inst.setVolume(0);
    float muted = measureDuty(inst, 1000);
    check("CC7 = 0 arrete la pompe", muted < 0.01f);
    inst.setVolume(100);

    inst.allNotesOff();
    float stopped = measureDuty(inst, 1000);
    check("plus aucune note : pompe arretee", stopped < 0.01f);

    inst.getAirSource().setFault(FAULT_OVERPRESSURE);
    runLoop(inst, 50);
    check("defaut : pompe coupee",
          stubPinState[PUMP_PIN] == (PUMP_ACTIVE_LEVEL ? LOW : HIGH));
    check("defaut propage a l'instrument", inst.getState() == SYS_FAULT);
}

#else
#error "test_air_sources ne couvre pas AIR_SOURCE_BELLOW_STEPPER : voir test_behavior."
#endif

// -----------------------------------------------------------------------------------------
// Proprietes communes a toutes les sources testees ici.
// -----------------------------------------------------------------------------------------
static void runCommon() {
    printf("\n=== Proprietes communes : valve generale et mise au repos ===\n");
    setupMachine();
    Instrument inst;
    bringUp(inst);

    unsigned long before = stubI2cWrites;
    inst.noteOn(60, 100, MIDI_CHANNEL_RIGHT);
    runLoop(inst, 50);
    check("la premiere note declenche des commandes", stubI2cWrites > before);
    check("note comptabilisee", inst.getActiveNoteCount() == 1);

    inst.noteOff(60, MIDI_CHANNEL_RIGHT);
    runLoop(inst, 50);
    check("note relachee", inst.getActiveNoteCount() == 0);

    // Mise au repos apres inactivite : la source doit s'arreter d'elle-meme.
    runLoop(inst, AIR_INACTIVITY_TIMEOUT + 2000, 500);
    check("pas de defaut apres une longue inactivite", inst.getState() == SYS_READY);

    // Le MIDI Panic reste accepte dans tous les etats.
    inst.noteOn(60, 100, MIDI_CHANNEL_RIGHT);
    runLoop(inst, 20);
    inst.allNotesOff();
    check("MIDI Panic referme tout", inst.getActiveNoteCount() == 0);
}

int main() {
    runSpecific();
    runCommon();
    return testSummary(SUITE);
}
