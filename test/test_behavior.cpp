// =========================================================================================
// Tests de comportement du firmware, joues sur une machine simulee.
//
// Le stub FlexyStepper modelise un pas par appel, les rampes d'acceleration, la
// persistance de la direction et le fait que la position PHYSIQUE ne bouge que si le
// driver est alimente. Les fins de course sont deduits de la position physique.
//
// Chaque scenario correspond a un point d'audit.
// =========================================================================================
#include <cmath>
#include "harness.h"
#include <FlexyStepper.h>
#include "instrument.h"
#include "bellowController.h"
#include "test_assert.h"

// Periode de boucle simulee (us). Le firmware suppose une boucle rapide pour tenir la
// frequence de pas ; on prend une valeur plausible pour un ATmega32U4.
#define LOOP_US 40

static void runLoop(Instrument &inst, unsigned long durationMs, unsigned long loopUs = LOOP_US) {
    unsigned long end = stubMicros + durationMs * 1000UL;
    while (stubMicros < end) {
        unsigned long before = stubMicros;
        inst.update();
        // Le temps consomme par les stubs (I2C) compte ; on complete jusqu'a la periode.
        unsigned long spent = stubMicros - before;
        stubMicros += (spent < loopUs) ? (loopUs - spent) : 1;
    }
}

// Machine : soufflet a `offset` mm au-dessus du contact bas (place a 0), contact haut
// 5 mm au-dela de la course nominale.
static void setupMachine(float offset = 30.0f) {
    stubResetMachine(offset, 0.0f, BELLOW_MAX_POSITION + 5.0f);
}

static void bringUp(Instrument &inst) {
    inst.begin();
    runLoop(inst, 60000, 200); // Homing complet (deux passes), pas de temps grossier
}

int main() {
    printf("\n=== Scenario 1 : homing deux passes, MIDI refuse pendant la calibration ===\n");
    {
        setupMachine();
        Instrument inst;
        inst.begin();
        check("etat = HOMING apres begin()", inst.getState() == SYS_HOMING);

        runLoop(inst, 200);
        inst.noteOn(60, 100, MIDI_CHANNEL_RIGHT);
        check("NoteOn refusee pendant le homing", !inst.isReady());
        check("driver moteur actif pendant le homing", stubPinState[STEPPER_EN_PIN] == LOW);

        runLoop(inst, 60000, 200);
        check("etat = READY apres homing", inst.getState() == SYS_READY);
        printf("     (position physique finale : %.3f mm, contact bas a 0)\n", stubPhysicalPos);
        check("zero etabli au contact, a moins de 0,5 mm", fabs(stubPhysicalPos) < 0.5f);
        check("pas de defaut", inst.getInstrumentFault() == INST_FAULT_NONE);
    }

    printf("\n=== Scenario 2 : surcourse maitrisee sur fin de course ===\n");
    {
        // Approche rapide a 10 mm/s. On mesure de combien le soufflet depasse le contact
        // avant l'arret : c'est le point P0 de l'audit.
        setupMachine(40.0f);
        Instrument inst;
        inst.begin();

        float deepest = 0.0f;
        unsigned long end = stubMicros + 60000UL * 1000UL;
        while (stubMicros < end && inst.getState() == SYS_HOMING) {
            inst.update();
            stubMicros += LOOP_US;
            if (stubPhysicalPos < deepest) deepest = stubPhysicalPos;
        }
        printf("     (surcourse maximale sous le contact : %.3f mm)\n", -deepest);
        check("surcourse inferieure a 1 mm", -deepest < 1.0f);
        check("driver coupe des le contact (pas de rampe de freinage subie)",
              -deepest < 0.5f);
    }

    printf("\n=== Scenario 3 : homing sans contact -> FAULT_HOMING_DISTANCE ===\n");
    {
        stubResetMachine(30.0f, -1e9f, 1e9f); // aucun contact atteignable
        Instrument inst;
        inst.begin();
        runLoop(inst, 60000, 500);
        check("defaut declare (pas de blocage infini)", inst.getState() == SYS_FAULT);
        check("code = FAULT_HOMING_DISTANCE et non un timeout",
              inst.getFault() == FAULT_HOMING_DISTANCE);
        check("driver moteur coupe en defaut", stubPinState[STEPPER_EN_PIN] == HIGH);
        check("servos maintenus alimentes en defaut", stubPinState[PCA_OE_PIN] == LOW);
    }

    printf("\n=== Scenario 4 : deux fins de course actifs -> defaut de cablage ===\n");
    {
        setupMachine();
        stubForceBothEndstops = true;
        Instrument inst;
        inst.begin();
        runLoop(inst, 500);
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
        unsigned long settle = stubMicros + 20000UL * 1000UL;
        unsigned long end = stubMicros + 90000UL * 1000UL;
        while (stubMicros < end) {
            inst.update();
            stubMicros += LOOP_US;
            if (stubMicros > settle) {
                if (stubPhysicalPos > peak) peak = stubPhysicalPos;
                if (stubPhysicalPos < trough) trough = stubPhysicalPos;
                if (digitalRead(LIMIT_SWITCH_MAX_PIN) == LOW) hitMax = true;
                if (digitalRead(LIMIT_SWITCH_MIN_PIN) == LOW) hitMin = true;
            }
        }
        printf("     (course observee : %.1f mm -> %.1f mm)\n", trough, peak);
        check("le fin de course HAUT n'est jamais atteint", !hitMax);
        check("le fin de course BAS n'est jamais atteint", !hitMin);
        check("inversion haute proche de 70 % (140 mm)", peak > 133.0f && peak < 152.0f);
        check("inversion basse proche de 30 % (60 mm)", trough > 50.0f && trough < 67.0f);
    }

    printf("\n=== Scenario 6 : reprise apres inactivite ===\n");
    {
        setupMachine();
        Instrument inst;
        bringUp(inst);
        check("instrument pret", inst.isReady());

        runLoop(inst, AIR_INACTIVITY_TIMEOUT + 3000, 500);
        check("driver coupe apres inactivite", stubPinState[STEPPER_EN_PIN] == HIGH);
        check("OE des PCA coupe apres inactivite", stubPinState[PCA_OE_PIN] == HIGH);

        float before = stubPhysicalPos;
        inst.noteOn(60, 100, MIDI_CHANNEL_RIGHT);
        runLoop(inst, 800);
        check("driver REACTIVE a la nouvelle note", stubPinState[STEPPER_EN_PIN] == LOW);
        check("OE des PCA reactive a la nouvelle note", stubPinState[PCA_OE_PIN] == LOW);
        check("le soufflet repart reellement", fabs(stubPhysicalPos - before) > 0.5f);
    }

    printf("\n=== Scenario 7 : CC7 = 0 coupe la production de pression ===\n");
    {
        setupMachine();
        Instrument inst;
        bringUp(inst);

        inst.noteOn(60, 100, MIDI_CHANNEL_RIGHT);
        runLoop(inst, 3000);
        inst.setVolume(0);
        runLoop(inst, 2000); // laisse la deceleration se terminer

        float before = stubPhysicalPos;
        runLoop(inst, 20000, 200); // 20 s a volume nul
        printf("     (derive sur 20 s : %.4f mm)\n", fabs(stubPhysicalPos - before));
        check("aucune derive a 2 mm/s a volume nul", fabs(stubPhysicalPos - before) < 0.01f);

        inst.setVolume(100);
        runLoop(inst, 1000);
        check("le soufflet repart quand CC7 remonte", fabs(stubPhysicalPos - before) > 0.5f);
    }

    printf("\n=== Scenario 8 : sustain CC64 ne laisse pas de note bloquee ===\n");
    {
        setupMachine();
        Instrument inst;
        bringUp(inst);

        inst.setSustain(true);
        inst.noteOn(60, 100, MIDI_CHANNEL_RIGHT);
        runLoop(inst, 200);
        inst.noteOff(60, MIDI_CHANNEL_RIGHT);
        runLoop(inst, 200);
        check("note toujours ouverte pedale enfoncee", inst.getActiveNoteCount() == 1);

        inst.setSustain(false);
        runLoop(inst, 100);
        check("note fermee au relachement de la pedale", inst.getActiveNoteCount() == 0);

        runLoop(inst, AIR_INACTIVITY_TIMEOUT + 3000, 500);
        check("l'inactivite se declenche ensuite", stubPinState[STEPPER_EN_PIN] == HIGH);
    }

    printf("\n=== Scenario 9 : polyphonie, vol de voix et rejeu ===\n");
    {
        setupMachine();
        Instrument inst;
        bringUp(inst);

        const uint8_t chordRow[12] = {48,55,50,57,52,59,54,61,56,63,58,53};
        for (int i = 0; i < 12; i++) inst.noteOn(chordRow[i], 100, MIDI_CHANNEL_LEFT);
        for (int i = 0; i < 3; i++)  inst.noteOn(70 + i, 100, MIDI_CHANNEL_RIGHT);
        runLoop(inst, 50);
        check("15 voix actives (limite atteinte)",
              inst.getActiveNoteCount() == MAX_SIMULTANEOUS_NOTES);

        inst.noteOn(36, 100, MIDI_CHANNEL_LEFT); // basse : priorite superieure
        runLoop(inst, 20);
        check("la basse entre en volant un accord",
              inst.getActiveNoteCount() == MAX_SIMULTANEOUS_NOTES);

        // Rejeu d'une note deja ouverte : ne doit pas consommer de voix ni en voler une.
        byte before = inst.getActiveNoteCount();
        inst.noteOn(36, 120, MIDI_CHANNEL_LEFT);
        runLoop(inst, 20);
        check("rejeu d'une note ouverte : compte inchange",
              inst.getActiveNoteCount() == before);

        inst.allNotesOff();
        runLoop(inst, 20);
        check("MIDI Panic remet le compte a zero", inst.getActiveNoteCount() == 0);
    }

    printf("\n=== Scenario 10 : PCA9685 absent au demarrage ===\n");
    {
        setupMachine();
        stubMissingPcaMask = 0x08; // 0x43 absent : c'est celui de la valve generale
        Instrument inst;
        inst.begin();
        runLoop(inst, 500);
        check("demarrage refuse", inst.getState() == SYS_FAULT);
        check("cause = PCA manquant", inst.getInstrumentFault() == INST_FAULT_PCA_MISSING);
        check("masque des absents correct", inst.getMissingPcaMask() == 0x08);
        check("driver moteur coupe", stubPinState[STEPPER_EN_PIN] == HIGH);
        inst.noteOn(60, 100, MIDI_CHANNEL_RIGHT);
        check("aucune note acceptee", inst.getActiveNoteCount() == 0);
        stubMissingPcaMask = 0;
    }

    printf("\n=== Scenario 11 : perte du bus I2C en cours de jeu ===\n");
    {
        setupMachine();
        Instrument inst;
        bringUp(inst);
        check("instrument pret", inst.isReady());

        stubI2cErrors = 500; // toutes les ecritures suivantes echouent
        for (int i = 0; i < SERVO_I2C_ERROR_LIMIT + 4; i++) {
            inst.noteOn(60 + i, 100, MIDI_CHANNEL_RIGHT);
            runLoop(inst, 5);
        }
        check("passage en defaut sur erreurs I2C repetees", inst.getState() == SYS_FAULT);
        check("cause = bus PCA", inst.getInstrumentFault() == INST_FAULT_PCA_BUS);
        check("driver moteur coupe", stubPinState[STEPPER_EN_PIN] == HIGH);
        stubI2cErrors = 0;
    }

    printf("\n=== Scenario 12 : velocite appliquee par note ===\n");
    {
        setupMachine();
        Instrument inst;
        bringUp(inst);

        // Un accord fort, puis une note faible ajoutee : la demande d'air doit AUGMENTER.
        // L'ancien modele appliquait la derniere velocite a toutes les notes, ce qui
        // faisait chuter le debit de l'accord entier.
        for (int i = 0; i < 3; i++) inst.noteOn(60 + i, 127, MIDI_CHANNEL_RIGHT);
        runLoop(inst, 300); // laisse l'attaque retomber
        float strongPos = stubPhysicalPos;
        runLoop(inst, 1000);
        float strongSpeed = (stubPhysicalPos - strongPos) / 1.0f;

        inst.noteOn(65, 10, MIDI_CHANNEL_RIGHT); // note tres douce ajoutee
        runLoop(inst, 300);
        float mixedPos = stubPhysicalPos;
        runLoop(inst, 1000);
        float mixedSpeed = (stubPhysicalPos - mixedPos) / 1.0f;

        printf("     (vitesse accord fort : %.2f mm/s -> avec note douce : %.2f mm/s)\n",
               strongSpeed, mixedSpeed);
        check("ajouter une note douce augmente le debit", mixedSpeed > strongSpeed);
    }

    return testSummary("test_behavior");
}
