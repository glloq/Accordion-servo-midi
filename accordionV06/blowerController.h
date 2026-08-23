#ifndef BLOWER_CONTROLLER_H
#define BLOWER_CONTROLLER_H

#include <Arduino.h>
#include "settings.h"
#if AIR_SOURCE == AIR_SOURCE_BLOWER_PWM || AIR_SOURCE == AIR_SOURCE_BLOWER_ESC || \
    AIR_SOURCE == AIR_SOURCE_PUMP_ONOFF

#include "servoController.h"
#include "airValve.h"
#include "airFault.h"
#include "pressureRegulator.h"

// =========================================================================================
// SOURCES D'AIR UNIDIRECTIONNELLES : turbine PWM, turbine brushless sur ESC, pompe
// tout-ou-rien.
// -----------------------------------------------------------------------------------------
// Les trois partagent la meme structure — une commande scalaire deduite de la demande d'air,
// sans course ni inversion ni calibration de position — et ne different que par l'etage de
// sortie. Les regrouper evite de tripler la logique de demande, de repos, de surveillance de
// pression et de valve ; ce qui les separe tient dans applyCommand().
//
// Differences essentielles avec un soufflet :
//   - la pression est CONTINUE tant que la machine tourne : il n'y a pas de va-et-vient, donc
//     pas de seuils d'inversion ni de fins de course ;
//   - il n'y a rien a calibrer en position. Un ESC demande en revanche un ARMEMENT, et une
//     turbine a l'arret un a-coup de demarrage : ces deux phases sont signalees par
//     isCalibrating(), ce qui suffit a Instrument pour refuser les notes en attendant ;
//   - le sens (soufflage ou aspiration) est un choix de montage, declare par AIR_DIRECTION.
//     Il ne change que le signe attendu du capteur de pression.
//
// Sans capteur de pression, tout ceci reste en boucle ouverte : la demande d'air pilote
// directement le rapport cyclique. Avec capteur, PressureRegulator corrige cette commande.
// =========================================================================================

// Phases internes. Elles ne sont pas exposees telles quelles : Instrument ne connait que
// isReady() / isCalibrating() / hasFault().
enum BlowerState {
    BLOWER_INIT,     // Avant begin()
    BLOWER_STARTING, // Armement de l'ESC, ou a-coup de demarrage de la turbine
    BLOWER_READY,
    BLOWER_FAULT
};

class BlowerController {
public:
    BlowerController(ServoController &servoCtrl, AirValve &valve, PressureRegulator &regulator);

    void begin();
    void update();

    void setAirDemand(float airDemand);
    void setVolume(byte volumeValue);
    void setExpression(byte expressionValue);

    void startCalibration(); // Rejoue l'armement / la mise en route
    void stopAndDisable();

    void openValve();
    void closeValve();

    bool isCalibrating() const { return state == BLOWER_STARTING; }
    bool isReady() const { return state == BLOWER_READY; }
    bool hasFault() const { return state == BLOWER_FAULT; }
    AirFault getFault() const { return fault; }
    void setFault(AirFault code);
    void clearFault();

    // Diagnostic / tests
    BlowerState getState() const { return state; }
    // Commande normalisee demandee, 0 = arret, 1 = plein regime.
    float getCommand() const { return command; }
    // Etat reel de la sortie de puissance. Pour une pompe tout-ou-rien, il alterne au
    // rythme de la modulation lente alors que getCommand() reste stable.
    bool isOutputActive() const { return outputActive; }

private:
    ServoController &servoController;
    AirValve &airValve;
    PressureRegulator &pressure;

    BlowerState state;
    AirFault fault;

    float airDemand;
    byte volume;
    byte expression;

    float command;      // Commande normalisee 0..1 deduite de la demande
    bool running;       // Une demande d'air est active
    bool outputActive;  // Etat courant de l'etage de puissance

    uint32_t phaseStart;   // Debut de l'armement / de l'a-coup de demarrage
    uint32_t cycleStart;   // Debut du cycle de modulation (pompe)
    uint32_t runStart;     // Debut de la marche continue (protection thermique pompe)
    uint32_t restUntil;    // Fin du repos force (pompe)

    float normalizedDemand() const; // Demande -> commande 0..1, correction de pression incluse
    void applyCommand();            // Traduit `command` vers l'etage de sortie
    void setOutput(bool active);    // Pompe : commande tout-ou-rien
    void updatePump();              // Modulation lente et protection thermique
};

#endif // sources unidirectionnelles
#endif
