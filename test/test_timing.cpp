// =========================================================================================
// Tests de TIMING.
//
// FlexyStepper ne produit qu'un pas par appel a processMovement() : la frequence de pas
// est donc bornee par la periode de la boucle principale. Ces tests mesurent la frequence
// reellement atteinte, avec et sans trafic MIDI, en tenant compte du cout simule des
// ecritures I2C (STUB_I2C_WRITE_US) et de l'analyse des messages MIDI.
//
// Ils ne remplacent pas une mesure sur la machine reelle, mais ils detectent les
// regressions d'ordonnancement : c'est exactement ce que l'audit signalait comme absent.
// =========================================================================================
#include <cmath>
#include "harness.h"
#include <FlexyStepper.h>
#include "instrument.h"
#include "midiHandler.h"
#include <MIDI.h>
#include "test_assert.h"

// Le firmware declare ces instances ; on les reutilise via les stubs MIDI.
extern MidiInterfaceStub MIDI_DIN;

#define LOOP_US 40

int main() {
    printf("\n=== A. Frequence de pas atteinte, boucle au repos ===\n");
    float baselineRate = 0.0f;
    {
        stubResetMachine(30.0f, 0.0f, BELLOW_MAX_POSITION + 5.0f);
        Instrument inst;
        inst.begin();
        // Homing complet
        unsigned long end = stubMicros + 60000UL * 1000UL;
        while (stubMicros < end && inst.getState() != SYS_READY) {
            inst.update();
            stubMicros += LOOP_US;
        }
        check("instrument pret", inst.isReady());

        // Demande maximale : 15 notes, velocite maximale.
        for (int i = 0; i < 15; i++) inst.noteOn(60 + i, 127, MIDI_CHANNEL_RIGHT);

        // Laisse la rampe s'etablir
        end = stubMicros + 4000UL * 1000UL;
        while (stubMicros < end) { inst.update(); stubMicros += LOOP_US; }

        unsigned long startSteps = stubStepCount;
        unsigned long startUs = stubMicros;
        end = stubMicros + 2000UL * 1000UL;
        while (stubMicros < end) { inst.update(); stubMicros += LOOP_US; }

        float elapsed = (stubMicros - startUs) / 1000000.0f;
        baselineRate = (stubStepCount - startSteps) / elapsed;
        printf("     (periode de boucle %d us -> %.0f pas/s, soit %.1f mm/s)\n",
               LOOP_US, baselineRate, baselineRate / STEPS_PER_MM);

        // Plafond theorique impose par la boucle : STEPPER_SERVICE_CALLS pas par tour.
        float loopCeiling = (1000000.0f / LOOP_US) * STEPPER_SERVICE_CALLS;
        printf("     (plafond impose par la boucle : %.0f pas/s)\n", loopCeiling);
        check("la frequence reste sous le plafond de la boucle", baselineRate <= loopCeiling * 1.05f);
        check("la vitesse est bien bornee par STEPPER_MAX_STEP_RATE_HZ",
              baselineRate <= STEPPER_MAX_STEP_RATE_HZ * 1.05f);
        check("frequence de pas utile atteinte (> 3000 pas/s)", baselineRate > 3000.0f);
    }

    printf("\n=== B. Effet d'une rafale MIDI sur la generation de pas ===\n");
    {
        stubResetMachine(30.0f, 0.0f, BELLOW_MAX_POSITION + 5.0f);
        Instrument inst;
        MidiHandler midi(inst);
        inst.begin();
        midi.begin();

        unsigned long end = stubMicros + 60000UL * 1000UL;
        while (stubMicros < end && inst.getState() != SYS_READY) {
            midi.update();
            inst.update();
            stubMicros += LOOP_US;
        }
        check("instrument pret", inst.isReady());

        for (int i = 0; i < 15; i++) inst.noteOn(60 + i, 127, MIDI_CHANNEL_RIGHT);
        end = stubMicros + 4000UL * 1000UL;
        while (stubMicros < end) { midi.update(); inst.update(); stubMicros += LOOP_US; }

        // Rafale MIDI continue : on reinjecte des CC (pas de commande servo) pour isoler
        // le cout d'ordonnancement du MIDI lui-meme.
        unsigned long startSteps = stubStepCount;
        unsigned long startUs = stubMicros;
        end = stubMicros + 2000UL * 1000UL;
        while (stubMicros < end) {
            for (int k = 0; k < 4; k++) MIDI_DIN.push(midi::ControlChange, 1, 7, 100);
            midi.update();
            inst.update();
            stubMicros += LOOP_US;
        }
        float elapsed = (stubMicros - startUs) / 1000000.0f;
        float burstRate = (stubStepCount - startSteps) / elapsed;
        printf("     (sous rafale MIDI saturee : %.0f pas/s, soit %.0f %% du repos)\n",
               burstRate, 100.0f * burstRate / baselineRate);

        // Un seul message MIDI est traite par tour de boucle : le generateur de pas
        // reprend la main entre chaque message. La chute doit rester limitee.
        check("la generation de pas resiste a une rafale MIDI continue",
              burstRate > baselineRate * 0.70f);
    }

    printf("\n=== C. Cout d'un accord : ecritures I2C ===\n");
    {
        stubResetMachine(30.0f, 0.0f, BELLOW_MAX_POSITION + 5.0f);
        Instrument inst;
        inst.begin();
        unsigned long end = stubMicros + 60000UL * 1000UL;
        while (stubMicros < end && inst.getState() != SYS_READY) {
            inst.update();
            stubMicros += LOOP_US;
        }

        unsigned long before = stubI2cWrites;
        unsigned long tBefore = stubMicros;
        for (int i = 0; i < 10; i++) inst.noteOn(60 + i, 100, MIDI_CHANNEL_RIGHT);
        unsigned long writes = stubI2cWrites - before;
        unsigned long cost = stubMicros - tBefore;

        printf("     (accord de 10 notes : %lu ecritures I2C, %lu us)\n", writes, cost);
        // 10 notes + la fermeture de la valve generale a la premiere note.
        check("une ecriture I2C par note (plus la valve)", writes >= 10 && writes <= 12);
        check("cout total sous 2 ms", cost < 2000);
    }

    printf("\n=== D. Un seul message MIDI traite par tour de boucle ===\n");
    {
        stubResetMachine(30.0f, 0.0f, BELLOW_MAX_POSITION + 5.0f);
        Instrument inst;
        MidiHandler midi(inst);
        inst.begin();
        midi.begin();
        unsigned long end = stubMicros + 60000UL * 1000UL;
        while (stubMicros < end && inst.getState() != SYS_READY) {
            midi.update(); inst.update(); stubMicros += LOOP_US;
        }

        for (int i = 0; i < 8; i++) MIDI_DIN.push(midi::NoteOn, MIDI_CHANNEL_RIGHT, 60 + i, 100);
        midi.update(); // un seul appel
        check("un seul message consomme par appel a update()", inst.getActiveNoteCount() == 1);

        for (int i = 0; i < 7; i++) { midi.update(); inst.update(); }
        check("les suivants arrivent un par tour", inst.getActiveNoteCount() == 8);
    }

    return testSummary("test_timing");
}
