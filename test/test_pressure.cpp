// =========================================================================================
// Tests du capteur de pression et de la regulation en boucle fermee.
//
// Compile avec -DPRESSURE_SENSOR_ENABLED=1 : la machine par defaut n'a pas de capteur, et
// sans cette definition toute la classe disparait a la compilation. C'est justement ce qui
// doit etre verifie — que le firmware fonctionne dans les deux cas — d'ou un binaire dedie
// plutot qu'un drapeau a l'execution.
//
// Le correcteur ne remplace pas le calcul en boucle ouverte : il produit un FACTEUR autour
// de 1.0. Les tests portent donc sur le signe et le bornage de ce facteur, pas sur une
// valeur de vitesse absolue.
// =========================================================================================
#include <cmath>
#include "harness.h"
#include "instrument.h"
#include "pressureRegulator.h"
#include "test_assert.h"

#define LOOP_US 40

// Comptes ADC correspondant a une pression donnee, d'apres l'etalonnage de config.h.
static int adcFor(float kpa) {
    return (int)(PRESSURE_ADC_AT_ZERO + kpa * PRESSURE_ADC_PER_KPA + 0.5f);
}

// Fait tourner le correcteur seul pendant `samples` periodes d'echantillonnage.
static void settle(PressureRegulator &reg, int samples) {
    for (int i = 0; i < samples; i++) {
        stubMicros += PRESSURE_SAMPLE_MS * 1000UL;
        reg.update();
    }
}

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
    printf("\n=== Scenario 1 : conversion ADC -> kPa et consigne ===\n");
    {
        stubResetMachine(0.0f, -1e9f, 1e9f);
        PressureRegulator reg;
        reg.begin();

        stubAnalogIn[PRESSURE_SENSOR_PIN] = adcFor(0.0f);
        settle(reg, 30);
        printf("     (mesure a offset nul : %.3f kPa)\n", reg.pressureKpa());
        check("le zero du capteur n'est pas le zero de l'ADC", fabs(reg.pressureKpa()) < 0.05f);

        stubAnalogIn[PRESSURE_SENSOR_PIN] = adcFor(3.0f);
        settle(reg, 60);
        printf("     (mesure a 3,0 kPa : %.3f kPa)\n", reg.pressureKpa());
        check("conversion correcte a 3 kPa", fabs(reg.pressureKpa() - 3.0f) < 0.15f);

        reg.setDemand(1.0f);
        check("demande 1.0 -> consigne nominale",
              fabs(reg.targetKpa() - PRESSURE_TARGET_KPA) < 0.001f);
        reg.setDemand(0.0f);
        check("demande nulle -> consigne nulle", reg.targetKpa() == 0.0f);

        // Une demande enorme ne doit jamais viser le plafond de securite, sinon le
        // correcteur passe son temps a declencher la coupure qu'il est cense eviter.
        reg.setDemand(100.0f);
        printf("     (consigne pour une demande de 100 : %.2f kPa, plafond %.2f)\n",
               reg.targetKpa(), (float)PRESSURE_MAX_KPA);
        check("la consigne reste sous le plafond de securite",
              reg.targetKpa() < PRESSURE_MAX_KPA);
    }

    printf("\n=== Scenario 2 : le correcteur va dans le bon sens ===\n");
    {
        stubResetMachine(0.0f, -1e9f, 1e9f);
        PressureRegulator reg;
        reg.begin();
        reg.setDemand(1.0f); // Consigne = PRESSURE_TARGET_KPA

        // Pression trop basse : il faut pousser plus fort.
        stubAnalogIn[PRESSURE_SENSOR_PIN] = adcFor(PRESSURE_TARGET_KPA * 0.4f);
        settle(reg, 100);
        float low = reg.scale();
        printf("     (pression a 40%% de la consigne -> facteur %.3f)\n", low);
        check("sous la consigne, la commande est augmentee", low > 1.0f);
        check("augmentation bornee", low <= PRESSURE_SCALE_MAX + 1e-4f);

        // Pression trop haute : il faut lever le pied.
        reg.reset();
        stubAnalogIn[PRESSURE_SENSOR_PIN] = adcFor(PRESSURE_TARGET_KPA * 1.6f);
        settle(reg, 100);
        float high = reg.scale();
        printf("     (pression a 160%% de la consigne -> facteur %.3f)\n", high);
        check("au-dessus de la consigne, la commande est reduite", high < 1.0f);
        check("reduction bornee", high >= PRESSURE_SCALE_MIN - 1e-4f);

        // A la consigne, le facteur doit rester proche de 1 : un correcteur qui derive a
        // l'equilibre ferait osciller la machine.
        reg.reset();
        stubAnalogIn[PRESSURE_SENSOR_PIN] = adcFor(PRESSURE_TARGET_KPA);
        settle(reg, 200);
        printf("     (pression a la consigne -> facteur %.3f)\n", reg.scale());
        check("a la consigne, pas de derive", fabs(reg.scale() - 1.0f) < 0.05f);
    }

    printf("\n=== Scenario 3 : l'integrateur ne s'emballe pas ===\n");
    {
        stubResetMachine(0.0f, -1e9f, 1e9f);
        PressureRegulator reg;
        reg.begin();
        reg.setDemand(1.0f);

        // Fuite totale : la pression ne monte jamais. Sans bornage, l'integrateur partirait
        // a l'infini et la premiere reprise de pression provoquerait un a-coup violent.
        stubAnalogIn[PRESSURE_SENSOR_PIN] = adcFor(0.0f);
        settle(reg, 4000);
        printf("     (apres 4000 echantillons a pression nulle : facteur %.3f)\n", reg.scale());
        check("facteur borne malgre une erreur permanente",
              reg.scale() <= PRESSURE_SCALE_MAX + 1e-4f);

        // Retour a la consigne : le facteur doit redescendre en un temps raisonnable.
        stubAnalogIn[PRESSURE_SENSOR_PIN] = adcFor(PRESSURE_TARGET_KPA);
        settle(reg, 1000);
        printf("     (apres retour a la consigne : facteur %.3f)\n", reg.scale());
        check("desaturation apres retour a la consigne", reg.scale() < PRESSURE_SCALE_MAX);

        // Fin de note : l'integrateur accumule doit etre purge, sinon la note suivante
        // demarre avec la correction d'une situation revolue.
        reg.setDemand(0.0f);
        settle(reg, 5);
        check("consigne nulle : correcteur relache", fabs(reg.scale() - 1.0f) < 1e-4f);
    }

    printf("\n=== Scenario 4 : surpression = defaut, machine en securite ===\n");
    {
        stubResetMachine(30.0f, 0.0f, BELLOW_MAX_POSITION + 5.0f);
        Instrument inst;
        inst.begin();
        stubAnalogIn[PRESSURE_SENSOR_PIN] = adcFor(PRESSURE_TARGET_KPA);
        runLoop(inst, 60000, 200); // Calibration complete
        check("instrument pret", inst.getState() == SYS_READY);

        inst.noteOn(60, 100, MIDI_CHANNEL_RIGHT);
        runLoop(inst, 200);
        check("pas de defaut a la pression nominale", inst.getState() == SYS_READY);

        // Sortie de secours bouchee, valve bloquee fermee, anche coincee : la pression part.
        stubAnalogIn[PRESSURE_SENSOR_PIN] = adcFor(PRESSURE_MAX_KPA * 1.5f);
        runLoop(inst, 500);
        check("defaut de surpression declare", inst.getState() == SYS_FAULT);
        check("cause = surpression", inst.getFault() == FAULT_OVERPRESSURE);
        check("moteur coupe", stubPinState[STEPPER_EN_PIN] == HIGH);
        check("aucune note ne passe en defaut",
              (inst.noteOn(72, 100, MIDI_CHANNEL_RIGHT), inst.getActiveNoteCount() == 0));
    }

    return testSummary("test_pressure");
}
