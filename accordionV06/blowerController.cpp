#include "blowerController.h"
#if AIR_SOURCE == AIR_SOURCE_BLOWER_PWM || AIR_SOURCE == AIR_SOURCE_BLOWER_ESC || \
    AIR_SOURCE == AIR_SOURCE_PUMP_ONOFF

#include <math.h>

// Demande d'air correspondant a la pleine commande, selon la source compilee.
#if AIR_SOURCE == AIR_SOURCE_BLOWER_PWM
  #define BLOWER_FULL_SCALE ((float)BLOWER_DEMAND_FULL_SCALE)
#elif AIR_SOURCE == AIR_SOURCE_BLOWER_ESC
  #define BLOWER_FULL_SCALE ((float)ESC_DEMAND_FULL_SCALE)
#else
  #define BLOWER_FULL_SCALE ((float)PUMP_DEMAND_FULL_SCALE)
#endif

BlowerController::BlowerController(ServoController &servoCtrl, AirValve &valve,
                                   PressureRegulator &regulator)
    : servoController(servoCtrl), airValve(valve), pressure(regulator),
      state(BLOWER_INIT), fault(FAULT_NONE),
      airDemand(0.0f), volume(100), expression(127),
      command(0.0f), running(false), outputActive(false),
      phaseStart(0), cycleStart(0), runStart(0), restUntil(0) {
    (void)servoCtrl; // Utilise seulement par la variante ESC
}

void BlowerController::begin() {
#if AIR_SOURCE == AIR_SOURCE_BLOWER_PWM
    pinMode(BLOWER_PWM_PIN, OUTPUT);
#elif AIR_SOURCE == AIR_SOURCE_PUMP_ONOFF
    pinMode(PUMP_PIN, OUTPUT);
    // Niveau inactif ecrit EXPLICITEMENT. Apres pinMode(OUTPUT) une broche vaut LOW, ce qui
    // met la pompe en MARCHE si PUMP_ACTIVE_LEVEL vaut 0 (etage de puissance inverseur) :
    // se fier a l'etat par defaut de la broche ferait demarrer la pompe des la mise sous
    // tension, avant meme que le firmware n'ait decide quoi que ce soit.
    digitalWrite(PUMP_PIN, PUMP_ACTIVE_LEVEL ? LOW : HIGH);
    outputActive = false;
#endif
    pressure.begin();
    startCalibration();
}

// "Calibration" d'une source unidirectionnelle : il n'y a pas de position a trouver, mais
// il y a une mise en route. Un ESC non arme ignore toute consigne — c'est une securite du
// protocole, pas un detail — et il faut lui envoyer l'impulsion minimale pendant plusieurs
// secondes avant qu'il n'accepte les gaz.
void BlowerController::startCalibration() {
    airValve.open();       // Position de securite : rien ne doit sonner pendant la mise en route
    pressure.reset();
    fault = FAULT_NONE;
    running = false;
    command = 0.0f;
    outputActive = false;
    restUntil = 0;
    runStart = 0;
    cycleStart = millis();
    phaseStart = millis();

#if AIR_SOURCE == AIR_SOURCE_BLOWER_ESC
    servoController.setPulseMicroseconds(ESC_PCA_ADDRESS, ESC_PCA_PIN, ESC_PULSE_MIN_US);
    state = BLOWER_STARTING; // L'armement dure ESC_ARM_MS
#else
    applyCommand();
    state = BLOWER_READY;    // Rien a armer : la machine est utilisable immediatement
#endif
}

// === DEMANDE -> COMMANDE NORMALISEE ===
float BlowerController::normalizedDemand() const {
    if (airDemand <= 0.0f || volume == 0 || expression == 0) return 0.0f;

    float scale = (volume / 127.0f) * (expression / 127.0f);
    // Correction de pression : vaut exactement 1.0 sans capteur, la commande reste alors
    // strictement proportionnelle a la demande d'air.
    float value = (airDemand * scale * pressure.scale()) / BLOWER_FULL_SCALE;

    if (value <= 0.0f) return 0.0f;
    if (value > 1.0f) return 1.0f; // Saturation : c'est la limite de polyphonie utile
    return value;
}

// === TRADUCTION VERS L'ETAGE DE SORTIE ===
void BlowerController::applyCommand() {
#if AIR_SOURCE == AIR_SOURCE_BLOWER_PWM
    uint16_t duty;
    if (!running) {
        // Au repos, la turbine peut etre laissee en rotation lente : elle repart alors sans
        // latence, au prix d'un bruit de fond permanent.
        duty = BLOWER_DUTY_IDLE;
    } else if (state == BLOWER_STARTING) {
        // Un moteur a l'arret ne demarre pas a BLOWER_DUTY_MIN : il faut un a-coup.
        duty = BLOWER_SPINUP_DUTY;
    } else {
        duty = (uint16_t)(BLOWER_DUTY_MIN +
                          command * (float)(BLOWER_DUTY_MAX - BLOWER_DUTY_MIN) + 0.5f);
    }
    if (duty > BLOWER_DUTY_MAX) duty = BLOWER_DUTY_MAX;
  #if BLOWER_PWM_INVERT
    analogWrite(BLOWER_PWM_PIN, 255 - (int)duty);
  #else
    analogWrite(BLOWER_PWM_PIN, (int)duty);
  #endif
    outputActive = (duty > 0);

#elif AIR_SOURCE == AIR_SOURCE_BLOWER_ESC
    uint16_t pulse;
    if (state == BLOWER_STARTING) {
        pulse = ESC_PULSE_MIN_US; // Armement : impulsion minimale maintenue
    } else if (!running) {
        pulse = ESC_PULSE_IDLE_US;
    } else {
        pulse = (uint16_t)(ESC_PULSE_IDLE_US +
                           command * (float)(ESC_PULSE_MAX_US - ESC_PULSE_IDLE_US) + 0.5f);
    }
    servoController.setPulseMicroseconds(ESC_PCA_ADDRESS, ESC_PCA_PIN, pulse);
    outputActive = (pulse > ESC_PULSE_MIN_US);

#else // AIR_SOURCE_PUMP_ONOFF
    // La pompe n'a pas de commande continue : c'est updatePump() qui decide de l'etat de la
    // sortie a partir de `command`. Ici on ne fait qu'arreter immediatement si la demande
    // est tombee a zero.
    if (!running) setOutput(false);
#endif
}

#if AIR_SOURCE == AIR_SOURCE_PUMP_ONOFF
void BlowerController::setOutput(bool active) {
    if (outputActive == active) return;
    bool level = active ? (PUMP_ACTIVE_LEVEL != 0) : (PUMP_ACTIVE_LEVEL == 0);
    digitalWrite(PUMP_PIN, level ? HIGH : LOW);
    outputActive = active;
    if (active) runStart = millis();
}

// Regulation d'une pompe tout-ou-rien.
//
//  - avec capteur : hysteresis autour de la consigne. C'est la regulation naturelle d'un
//    systeme a reservoir : on remplit jusqu'au haut de la bande, on laisse redescendre.
//  - sans capteur : modulation lente du rapport cyclique sur PUMP_CYCLE_MS. Une pompe a
//    membrane ne suit pas un PWM rapide — ses clapets ont une inertie mecanique — d'ou une
//    periode de l'ordre de la centaine de millisecondes.
//
// Dans les deux cas, la marche continue est bornee : ces pompes chauffent et n'ont pas de
// service continu. Un depassement repete alors que la pression reste basse signale une
// fuite ou une pompe morte, pas un simple exces de demande : c'est FAULT_PUMP_OVERRUN.
void BlowerController::updatePump() {
    uint32_t now = millis();

    if (restUntil != 0) {
        if ((int32_t)(now - restUntil) < 0) { setOutput(false); return; }
        restUntil = 0;
        cycleStart = now;
    }

    if (!running || command <= 0.0f) {
        setOutput(false);
        runStart = 0;
        return;
    }

    // Protection thermique : marche continue trop longue.
    if (outputActive && runStart != 0 && (now - runStart) > PUMP_MAX_RUN_MS) {
        setOutput(false);
        restUntil = now + PUMP_REST_MS;
  #if PRESSURE_SENSOR_ENABLED
        // La pompe a tourne a fond sans jamais approcher la consigne : ce n'est plus un
        // exces de demande, c'est une panne (fuite, clapet, membrane percee).
        if (pressure.targetKpa() > 0.0f && pressure.pressureKpa() < (pressure.targetKpa() * 0.5f)) {
            setFault(FAULT_PUMP_OVERRUN);
        }
  #endif
        return;
    }

  #if PRESSURE_SENSOR_ENABLED
    // Hysteresis autour de la consigne.
    float target = pressure.targetKpa();
    if (target > 0.0f) {
        float measured = pressure.pressureKpa();
        if (measured < (target - PUMP_HYSTERESIS_KPA))      setOutput(true);
        else if (measured > (target + PUMP_HYSTERESIS_KPA)) setOutput(false);
        return;
    }
  #endif

    // Modulation lente du rapport cyclique.
    uint32_t dutyPercent = (uint32_t)(PUMP_MIN_DUTY_PERCENT +
                                      command * (float)(PUMP_MAX_DUTY_PERCENT - PUMP_MIN_DUTY_PERCENT) + 0.5f);
    uint32_t elapsed = now - cycleStart;
    if (elapsed >= PUMP_CYCLE_MS) {
        cycleStart = now;
        elapsed = 0;
    }
    setOutput(elapsed < ((PUMP_CYCLE_MS * dutyPercent) / 100UL));
}
#else
void BlowerController::setOutput(bool active) { outputActive = active; }
void BlowerController::updatePump() {}
#endif

void BlowerController::update() {
    if (state == BLOWER_FAULT) return;

    pressure.update();
    if (pressure.overPressure()) {
        setFault(FAULT_OVERPRESSURE);
        return;
    }

    if (state == BLOWER_STARTING) {
#if AIR_SOURCE == AIR_SOURCE_BLOWER_ESC
        if ((millis() - phaseStart) < (uint32_t)ESC_ARM_MS) {
            applyCommand(); // Maintient l'impulsion d'armement
            return;
        }
#elif AIR_SOURCE == AIR_SOURCE_BLOWER_PWM
        if ((millis() - phaseStart) < (uint32_t)BLOWER_SPINUP_MS) {
            applyCommand(); // Maintient l'a-coup de demarrage
            return;
        }
#endif
        state = BLOWER_READY;
        applyCommand();
        return;
    }

    if (state != BLOWER_READY) return;

    command = normalizedDemand();
    updatePump();
    applyCommand();
}

void BlowerController::setAirDemand(float demand) {
    airDemand = demand;
    pressure.setDemand(demand);

    bool wantRunning = (demand > 0.0f && volume > 0 && expression > 0);

#if AIR_SOURCE == AIR_SOURCE_BLOWER_PWM
    // Demarrage depuis l'arret complet : un a-coup est necessaire. Inutile si la turbine
    // tourne deja au ralenti.
    if (wantRunning && !running && state == BLOWER_READY && BLOWER_DUTY_IDLE == 0) {
        state = BLOWER_STARTING;
        phaseStart = millis();
    }
#endif

    running = wantRunning;
    if (!running) {
        command = 0.0f;
        pressure.reset();
    } else {
        command = normalizedDemand();
    }
    if (state == BLOWER_READY) {
        updatePump();
        applyCommand();
    }
}

void BlowerController::setVolume(byte volumeValue) {
    volume = volumeValue;
    setAirDemand(airDemand); // Re-evalue running : CC7 = 0 doit reellement couper
}

void BlowerController::setExpression(byte expressionValue) {
    expression = expressionValue;
    setAirDemand(airDemand);
}

void BlowerController::stopAndDisable() {
    running = false;
    command = 0.0f;
    pressure.reset();
#if AIR_SOURCE == AIR_SOURCE_PUMP_ONOFF
    setOutput(false);
#else
    applyCommand();
#endif
}

void BlowerController::openValve()  { airValve.open(); }
void BlowerController::closeValve() { airValve.close(); }

void BlowerController::setFault(AirFault code) {
    if (state == BLOWER_FAULT) return; // Premier defaut conserve
    fault = code;
    state = BLOWER_FAULT;

    running = false;
    command = 0.0f;
    pressure.reset();

    // Coupure de la puissance avant tout, puis mise a l'air libre.
#if AIR_SOURCE == AIR_SOURCE_BLOWER_PWM
  #if BLOWER_PWM_INVERT
    analogWrite(BLOWER_PWM_PIN, 255);
  #else
    analogWrite(BLOWER_PWM_PIN, 0);
  #endif
    outputActive = false;
#elif AIR_SOURCE == AIR_SOURCE_BLOWER_ESC
    servoController.setPulseMicroseconds(ESC_PCA_ADDRESS, ESC_PCA_PIN, ESC_PULSE_MIN_US);
    outputActive = false;
#else
    setOutput(false);
#endif
    airValve.open();

    #if DEBUG
    Serial.print(F("[DEBUG] Defaut source d'air: "));
    Serial.println((int)code);
    #endif
}

void BlowerController::clearFault() {
    if (state != BLOWER_FAULT) return;
    startCalibration();
}

#endif // sources unidirectionnelles
