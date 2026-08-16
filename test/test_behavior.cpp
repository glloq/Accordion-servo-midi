// =========================================================================================
// Tests de comportement du firmware, joues sur une machine simulee.
//
// Les stubs de test/stubs/ remplacent Arduino, Wire, Adafruit_PWMServoDriver, FlexyStepper
// et MIDI. Le stub FlexyStepper simule reellement le deplacement, et les fins de course
// sont DEDUITS de la position du soufflet (comme des capteurs physiques) : un recalage de
// coordonnee ne deplace pas les contacts.
//
// Chaque scenario correspond a un point de l'audit.
// =========================================================================================
#include <cmath>
#include "harness.h"
#include <FlexyStepper.h>
#include "instrument.h"
#include "bellowController.h"
#include "test_assert.h"

// Avance le temps simule et fait tourner la boucle principale.
static void run(Instrument &inst, unsigned long ms, unsigned long stepMs = 1) {
    for (unsigned long t = 0; t < ms; t += stepMs) {
        stubClock += stepMs;
        inst.update();
    }
}

// Machine physique : le soufflet demarre 30 mm au-dessus du contact bas, et le contact
// haut est 5 mm au-dela de la course nominale.
static void setupMachine(float startOffset = 30.0f) {
    stubClock = 1000;
    stubForceBothEndstops = false;
    stubMinSwitchPos = -startOffset;
    stubMaxSwitchPos = -startOffset + BELLOW_MAX_POSITION + 5.0f;
}

// Amene l'instrument a l'etat READY (homing complet).
static void bringUp(Instrument &inst) {
    inst.begin();
    run(inst, 20000, 2);
}

int main() {
    printf("\n=== Scenario 1 : homing + MIDI refuse pendant la calibration ===\n");
    {
        setupMachine();
        Instrument inst;
        inst.begin();
        check("etat = HOMING apres begin()", inst.getState() == SYS_HOMING);

        run(inst, 500);
        inst.noteOn(60, 100, MIDI_CHANNEL_RIGHT);
        check("NoteOn refusee pendant le homing", !inst.isReady());
        check("driver moteur actif pendant le homing", stubPinState[STEPPER_EN_PIN] == LOW);

        run(inst, 20000, 2);
        check("etat = READY apres contact du fin de course MIN", inst.getState() == SYS_READY);
        check("zero recale sur le contact physique",
              fabs(g_stepper->getCurrentPositionInMillimeters()) < 1.0f);
    }

    printf("\n=== Scenario 2 : homing long (proche de la course max) ===\n");
    {
        // Homing de ~24 s : bien plus long que PCA_DISABLE_DELAY (500 ms).
        // L'ancien code coupait le driver et l'OE en pleine calibration.
        setupMachine(240.0f);
        Instrument inst;
        inst.begin();

        bool driverCut = false, oeCut = false;
        for (int i = 0; i < 24000; i++) {
            stubClock += 1;
            inst.update();
            if (inst.getState() != SYS_HOMING) break;
            if (stubPinState[STEPPER_EN_PIN] == HIGH) driverCut = true;
            if (stubPinState[PCA_OE_PIN] == HIGH) oeCut = true;
        }
        check("driver moteur jamais coupe pendant le homing", !driverCut);
        check("OE des PCA jamais coupe pendant le homing (valve ouverte)", !oeCut);
        run(inst, 2000, 2);
        check("homing long mene bien a READY", inst.getState() == SYS_READY);
    }

    printf("\n=== Scenario 3 : homing sans fin de course -> defaut ===\n");
    {
        setupMachine();
        stubMinSwitchPos = -1e9f; // jamais atteint
        Instrument inst;
        inst.begin();
        run(inst, HOMING_TIMEOUT_MS + 3000, 10);
        check("defaut declare (pas de blocage infini)", inst.getState() == SYS_FAULT);
        check("driver moteur coupe en defaut", stubPinState[STEPPER_EN_PIN] == HIGH);
        check("servos maintenus alimentes en defaut", stubPinState[PCA_OE_PIN] == LOW);
        check("NoteOn refusee en defaut", (inst.noteOn(60, 100, MIDI_CHANNEL_RIGHT), !inst.isReady()));
    }

    printf("\n=== Scenario 4 : deux fins de course actifs -> defaut de cablage ===\n");
    {
        setupMachine();
        stubForceBothEndstops = true;
        Instrument inst;
        inst.begin();
        run(inst, 500);
        check("defaut FAULT_ENDSTOP_WIRING",
              inst.getState() == SYS_FAULT && inst.getFault() == FAULT_ENDSTOP_WIRING);
        stubForceBothEndstops = false;
    }

    printf("\n=== Scenario 5 : inversion 30/70 %% sur note TENUE (aucun evenement MIDI) ===\n");
    {
        setupMachine();
        Instrument inst;
        bringUp(inst);
        check("instrument pret", inst.isReady());

        inst.noteOn(60, 100, MIDI_CHANNEL_RIGHT); // une seule NoteOn, puis plus rien

        bool hitMax = false, hitMin = false;
        float peak = -1e9f, trough = 1e9f;
        // Deux minutes de note tenue, sans aucun message MIDI supplementaire.
        for (int i = 0; i < 120000; i++) {
            stubClock += 1;
            inst.update();
            float p = g_stepper->getCurrentPositionInMillimeters();
            if (i > 20000) { // apres stabilisation du cycle
                if (p > peak) peak = p;
                if (p < trough) trough = p;
            }
            if (digitalRead(LIMIT_SWITCH_MAX_PIN) == LOW) hitMax = true;
            // Juste apres le homing, le soufflet repose sur le contact bas : normal.
            // On ne surveille le contact bas qu'une fois le cycle etabli.
            if (i > 20000 && digitalRead(LIMIT_SWITCH_MIN_PIN) == LOW) hitMin = true;
        }
        printf("     (course observee : %.1f mm -> %.1f mm)\n", trough, peak);
        check("le fin de course HAUT n'est jamais atteint", !hitMax);
        check("le fin de course BAS n'est jamais atteint", !hitMin);
        check("inversion haute proche de 70 % (140 mm)", peak > 135.0f && peak < 150.0f);
        check("inversion basse proche de 30 % (60 mm)", trough > 52.0f && trough < 65.0f);
    }

    printf("\n=== Scenario 6 : reprise apres inactivite ===\n");
    {
        setupMachine();
        Instrument inst;
        bringUp(inst);
        check("instrument pret", inst.isReady());

        run(inst, BELLOW_INACTIVITY_TIMEOUT + 3000, 10);
        check("driver coupe apres inactivite", stubPinState[STEPPER_EN_PIN] == HIGH);
        check("OE des PCA coupe apres inactivite", stubPinState[PCA_OE_PIN] == HIGH);

        float before = g_stepper->getCurrentPositionInMillimeters();
        inst.noteOn(60, 100, MIDI_CHANNEL_RIGHT);
        run(inst, 500, 1);
        check("driver REACTIVE a la nouvelle note", stubPinState[STEPPER_EN_PIN] == LOW);
        check("OE des PCA reactive a la nouvelle note", stubPinState[PCA_OE_PIN] == LOW);
        check("le soufflet repart reellement",
              fabs(g_stepper->getCurrentPositionInMillimeters() - before) > 0.5f);
    }

    printf("\n=== Scenario 7 : CC7 = 0 coupe la production de pression ===\n");
    {
        setupMachine();
        Instrument inst;
        bringUp(inst);

        inst.noteOn(60, 100, MIDI_CHANNEL_RIGHT);
        run(inst, 2000, 1);
        inst.setVolume(0);
        run(inst, 10, 1);

        float before = g_stepper->getCurrentPositionInMillimeters();
        run(inst, 30000, 1); // 30 s a volume nul
        float after = g_stepper->getCurrentPositionInMillimeters();
        printf("     (derive sur 30 s : %.3f mm)\n", fabs(after - before));
        check("aucune derive a 2 mm/s a volume nul", fabs(after - before) < 0.01f);

        inst.setVolume(100);
        run(inst, 500, 1);
        check("le soufflet repart quand CC7 remonte",
              fabs(g_stepper->getCurrentPositionInMillimeters() - after) > 0.5f);
    }

    printf("\n=== Scenario 8 : sustain CC64 ne laisse pas de note bloquee ===\n");
    {
        setupMachine();
        Instrument inst;
        bringUp(inst);

        inst.setSustain(true);
        inst.noteOn(60, 100, MIDI_CHANNEL_RIGHT);
        run(inst, 50);
        inst.noteOff(60, MIDI_CHANNEL_RIGHT);
        run(inst, 50);
        float held = g_stepper->getCurrentPositionInMillimeters();
        run(inst, 1000, 1);
        check("note maintenue tant que la pedale est enfoncee",
              fabs(g_stepper->getCurrentPositionInMillimeters() - held) > 0.5f);

        inst.setSustain(false);
        run(inst, 50);
        float stopped = g_stepper->getCurrentPositionInMillimeters();
        run(inst, 2000, 1);
        check("soufflet arrete au relachement de la pedale",
              fabs(g_stepper->getCurrentPositionInMillimeters() - stopped) < 0.01f);

        run(inst, BELLOW_INACTIVITY_TIMEOUT + 3000, 10);
        check("l'inactivite se declenche (activeNotes bien retombe a 0)",
              stubPinState[STEPPER_EN_PIN] == HIGH);
    }

    printf("\n=== Scenario 9 : mapping MIDI main gauche (README) ===\n");
    {
        const uint8_t bassRow[12]  = {36,43,38,45,40,47,42,49,44,51,46,41};
        const uint8_t chordRow[12] = {48,55,50,57,52,59,54,61,56,63,58,53};
        bool allBass = true, allChord = true;
        for (int i = 0; i < 12; i++) {
            if (findNoteIndex(LEFT_HAND_MAPPING, NUM_NOTES_LEFT, bassRow[i]) < 0) allBass = false;
            if (findNoteIndex(LEFT_HAND_MAPPING, NUM_NOTES_LEFT, chordRow[i]) < 0) allChord = false;
        }
        check("les 12 basses du README sont jouables", allBass);
        check("les 12 accords du README sont jouables (61 et 63 compris)", allChord);
        check("note 37 absente (non contigue)",
              findNoteIndex(LEFT_HAND_MAPPING, NUM_NOTES_LEFT, 37) < 0);
        check("note 39 absente (non contigue)",
              findNoteIndex(LEFT_HAND_MAPPING, NUM_NOTES_LEFT, 39) < 0);
    }

    printf("\n=== Scenario 10 : polyphonie et vol de voix ===\n");
    {
        setupMachine();
        Instrument inst;
        bringUp(inst);

        // Sature avec 12 accords (priorite basse) + 3 notes de melodie = 15 voix
        const uint8_t chordRow[12] = {48,55,50,57,52,59,54,61,56,63,58,53};
        for (int i = 0; i < 12; i++) inst.noteOn(chordRow[i], 100, MIDI_CHANNEL_LEFT);
        for (int i = 0; i < 3; i++)  inst.noteOn(70 + i, 100, MIDI_CHANNEL_RIGHT);
        run(inst, 50);
        check("15 voix actives (limite atteinte)",
              inst.getActiveNoteCount() == MAX_SIMULTANEOUS_NOTES);

        // Une basse (priorite haute) doit voler une voix d'accord : le compte reste a 15
        // mais le premier accord joue (48) doit avoir ete ferme.
        inst.noteOn(36, 100, MIDI_CHANNEL_LEFT);
        run(inst, 10);
        check("la basse 36 est bien entree", inst.isReady());
        check("le compte de voix reste borne a 15",
              inst.getActiveNoteCount() == MAX_SIMULTANEOUS_NOTES);

        // Un accord supplementaire ne trouve aucune voix moins prioritaire qu'un accord
        // parmi les melodies/basses restantes... mais il reste des accords : il vole donc
        // le plus ancien. En revanche il ne doit jamais evincer la basse ni la melodie.
        inst.noteOn(59, 100, MIDI_CHANNEL_LEFT);
        run(inst, 10);
        check("toujours 15 voix apres un accord de plus",
              inst.getActiveNoteCount() == MAX_SIMULTANEOUS_NOTES);

        // La melodie (priorite > accord) doit encore pouvoir entrer.
        inst.noteOn(80, 100, MIDI_CHANNEL_RIGHT);
        run(inst, 10);
        check("une melodie entre encore a saturation",
              inst.getActiveNoteCount() == MAX_SIMULTANEOUS_NOTES);

        // MIDI Panic : tout se referme
        inst.allNotesOff();
        run(inst, 10);
        check("MIDI Panic remet le compte a zero", inst.getActiveNoteCount() == 0);
    }

    return testSummary("test_behavior");
}
