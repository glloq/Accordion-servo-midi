// =========================================================================================
// Tests d'ETAT DU GENERATEUR DE PAS.
//
// Ces tests s'attaquent directement au defaut signale par l'audit : FlexyStepper conserve
// son propre etat de direction et de rampe, et setCurrentPosition() ne l'arrete pas.
//
// Ils commencent par verifier que le STUB reproduit bien ce comportement : un stub trop
// idealise validerait n'importe quoi. On demontre d'abord la surcourse sur l'ancien motif
// (cible ramenee sur la position courante a pleine vitesse), puis on verifie que le
// firmware ne l'utilise plus.
// =========================================================================================
#include <cmath>
#include "harness.h"
#include <FlexyStepper.h>
#include "instrument.h"
#include "bellowController.h"
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

int main() {
    // =====================================================================================
    printf("\n=== A. Fidelite du stub : l'ancien motif d'arret produit bien une surcourse ===\n");
    {
        stubResetMachine(100.0f, -1e9f, 1e9f);
        stubPinState[STEPPER_EN_PIN] = LOW; // driver alimente

        FlexyStepper s;
        s.setStepsPerMillimeter(STEPS_PER_MM);
        s.setSpeedInMillimetersPerSecond(10.0f);
        s.setAccelerationInMillimetersPerSecondPerSecond(5.0f);
        s.setCurrentPositionInMillimeters(0.0f);
        s.setTargetPositionInMillimeters(-100.0f);

        // Monte en vitesse
        for (int i = 0; i < 4000000 && s.getCurrentRateStepsPerSecond() < 10.0f * STEPS_PER_MM * 0.99f; i++) {
            s.processMovement();
            stubMicros += 5;
        }
        float rate = s.getCurrentRateStepsPerSecond();
        printf("     (vitesse atteinte : %.1f mm/s)\n", rate / STEPS_PER_MM);
        check("le stub accelere reellement (pas de vitesse instantanee)",
              rate > 9.0f * STEPS_PER_MM);

        // ANCIEN MOTIF : on ramene la cible sur la position courante, moteur lance.
        // On mesure l'excursion MAXIMALE, pas la position finale : la bibliotheque revient
        // ensuite sur la cible, mais la machine, elle, a bel et bien percute la butee.
        float atStop = s.getCurrentPositionInMillimeters();
        s.setTargetPositionInMillimeters(atStop);
        float overshoot = 0.0f;
        for (int i = 0; i < 4000000 && !s.motionComplete(); i++) {
            s.processMovement();
            stubMicros += 5;
            float excursion = fabs(s.getCurrentPositionInMillimeters() - atStop);
            if (excursion > overshoot) overshoot = excursion;
        }
        printf("     (surcourse du motif setTargetPosition(position courante) : %.2f mm)\n",
               overshoot);
        check("le stub reproduit la surcourse (sinon il masquerait le bug)",
              overshoot > 1.0f);
        // d = v^2 / 2a = 10^2 / 10 = 10 mm, l'ordre de grandeur annonce par l'audit.
        check("surcourse de l'ordre de v^2/2a (~10 mm)", overshoot > 5.0f && overshoot < 20.0f);
    }

    // =====================================================================================
    printf("\n=== B. Le stub modelise la coupure du driver ===\n");
    {
        stubResetMachine(50.0f, -1e9f, 1e9f);
        stubPinState[STEPPER_EN_PIN] = HIGH; // driver COUPE

        FlexyStepper s;
        s.setStepsPerMillimeter(STEPS_PER_MM);
        s.setSpeedInMillimetersPerSecond(10.0f);
        s.setAccelerationInMillimetersPerSecondPerSecond(5.0f);
        s.setCurrentPositionInMillimeters(0.0f);
        s.setTargetPositionInMillimeters(-50.0f);

        float physBefore = stubPhysicalPos;
        for (int i = 0; i < 200000; i++) { s.processMovement(); stubMicros += 5; }

        check("le compteur de la bibliotheque avance malgre le driver coupe",
              fabs(s.getCurrentPositionInMillimeters()) > 1.0f);
        check("la machine, elle, ne bouge pas", fabs(stubPhysicalPos - physBefore) < 0.001f);
    }

    // =====================================================================================
    printf("\n=== C. Le firmware coupe le driver DES le contact brut ===\n");
    {
        // Contact bas a 0, soufflet a 20 mm : approche rapide a 10 mm/s.
        stubResetMachine(20.0f, 0.0f, BELLOW_MAX_POSITION + 5.0f);
        Instrument inst;
        inst.begin();

        bool sawContact = false;
        float posAtCut = 0.0f;
        bool cutRecorded = false;
        float deepest = 1e9f;

        unsigned long end = stubMicros + 30000UL * 1000UL;
        while (stubMicros < end && inst.getState() == SYS_HOMING) {
            bool contact = (digitalRead(LIMIT_SWITCH_MIN_PIN) == LOW);
            inst.update();
            stubMicros += LOOP_US;

            if (contact) sawContact = true;
            // Premiere coupure du driver observee apres le contact
            if (sawContact && !cutRecorded && stubPinState[STEPPER_EN_PIN] == HIGH) {
                posAtCut = stubPhysicalPos;
                cutRecorded = true;
            }
            if (stubPhysicalPos < deepest) deepest = stubPhysicalPos;
        }

        check("le contact a bien ete rencontre", sawContact);
        check("le driver a ete coupe", cutRecorded);
        printf("     (position a la coupure : %.3f mm ; point le plus bas : %.3f mm)\n",
               posAtCut, deepest);
        // Sans coupure immediate, la rampe de freinage a 5 mm/s^2 depuis 10 mm/s
        // parcourrait ~10 mm sous le contact.
        check("coupure a moins de 0,2 mm sous le contact", fabs(posAtCut) < 0.2f);
        check("surcourse totale tres inferieure a la distance de freinage (10 mm)",
              -deepest < 1.0f);
    }

    // =====================================================================================
    printf("\n=== D. Recalage de position uniquement moteur arrete ===\n");
    {
        stubResetMachine(20.0f, 0.0f, BELLOW_MAX_POSITION + 5.0f);
        Instrument inst;
        inst.begin();

        // Le stub compte lui-meme les appels a setCurrentPosition() faits alors que le
        // moteur tourne encore : c'est la violation exacte du contrat de la bibliotheque,
        // detectee a la source plutot que devinee depuis les positions observees.
        unsigned long end = stubMicros + 60000UL * 1000UL;
        while (stubMicros < end && inst.getState() != SYS_READY && inst.getState() != SYS_FAULT) {
            inst.update();
            stubMicros += LOOP_US;
        }
        check("homing termine", inst.getState() == SYS_READY);
        printf("     (recalages illicites detectes : %lu)\n", stubIllegalPositionResets);
        check("aucun setCurrentPosition() pendant un mouvement",
              stubIllegalPositionResets == 0);
        printf("     (zero final, position physique : %.3f mm)\n", stubPhysicalPos);
        check("zero repetable etabli par la passe lente", fabs(stubPhysicalPos) < 0.3f);

        // Et pendant le jeu, fins de course compris.
        inst.noteOn(60, 110, MIDI_CHANNEL_RIGHT);
        unsigned long before = stubIllegalPositionResets;
        end = stubMicros + 60000UL * 1000UL;
        while (stubMicros < end) { inst.update(); stubMicros += LOOP_US; }
        check("aucun recalage illicite pendant 60 s de jeu",
              stubIllegalPositionResets == before);
    }

    printf("\n=== E. Fin de course atteint EN JEU : arret, recalage, inversion ===\n");
    {
        // Contact haut volontairement place tres bas (60 mm) pour etre atteint en jeu
        // malgre le seuil d'inversion a 70 % (140 mm).
        stubResetMachine(20.0f, 0.0f, 60.0f);
        Instrument inst;
        inst.begin();
        runLoop(inst, 60000, 200);
        check("instrument pret", inst.isReady());

        inst.noteOn(60, 100, MIDI_CHANNEL_RIGHT);

        float highest = -1e9f;
        bool wentBackDown = false;
        unsigned long end = stubMicros + 40000UL * 1000UL;
        while (stubMicros < end) {
            inst.update();
            stubMicros += LOOP_US;
            if (stubPhysicalPos > highest) highest = stubPhysicalPos;
            if (highest > 55.0f && stubPhysicalPos < highest - 5.0f) wentBackDown = true;
        }
        printf("     (point le plus haut : %.3f mm, contact haut a 60,0 mm)\n", highest);
        check("le soufflet ne force pas au-dela du contact haut", highest < 61.0f);
        check("il repart en sens inverse apres le contact", wentBackDown);
        check("pas de defaut declenche par un contact normal en jeu", inst.isReady());
    }

    // =====================================================================================
    printf("\n=== F. Cablage DIR inverse -> defaut de direction ===\n");
    {
        // Le firmware croit descendre vers la butee basse, mais la machine monte.
        // Sans detection, il pousserait le soufflet jusqu'a la butee haute a pleine
        // vitesse pendant tout le homing.
        stubResetMachine(20.0f, -50.0f, 40.0f);
        stubInvertPhysicalDirection = true;
        Instrument inst;
        inst.begin();
        unsigned long end = stubMicros + 60000UL * 1000UL;
        while (stubMicros < end && inst.getState() != SYS_FAULT && inst.getState() != SYS_READY) {
            inst.update();
            stubMicros += LOOP_US;
        }
        printf("     (position physique atteinte : %.2f mm, contact haut a 40,0 mm)\n",
               stubPhysicalPos);
        check("defaut declare", inst.getState() == SYS_FAULT);
        check("code = FAULT_HOMING_DIRECTION", inst.getFault() == FAULT_HOMING_DIRECTION);
        check("le soufflet n'est pas force au-dela du contact haut", stubPhysicalPos < 41.0f);
        check("driver coupe", stubPinState[STEPPER_EN_PIN] == HIGH);
        stubInvertPhysicalDirection = false;
    }

    return testSummary("test_stepper_stop");
}
