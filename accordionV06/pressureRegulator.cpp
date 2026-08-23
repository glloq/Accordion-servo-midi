#include "pressureRegulator.h"
#include <math.h>

#if !PRESSURE_SENSOR_ENABLED

// --- Sans capteur : coquille vide, entierement eliminee a la compilation ------------------
PressureRegulator::PressureRegulator() {}
void PressureRegulator::begin() {}
void PressureRegulator::update() {}
void PressureRegulator::setDemand(float) {}
void PressureRegulator::reset() {}
float PressureRegulator::scale() const { return 1.0f; }
float PressureRegulator::pressureKpa() const { return 0.0f; }
float PressureRegulator::targetKpa() const { return 0.0f; }
bool PressureRegulator::overPressure() const { return false; }

#else

PressureRegulator::PressureRegulator()
    : filtered(0.0f), target(0.0f), integral(0.0f), lastError(0.0f), correction(1.0f),
      lastSampleTime(0), primed(false) {}

void PressureRegulator::begin() {
    lastSampleTime = millis();
    reset();
}

void PressureRegulator::reset() {
    integral = 0.0f;
    lastError = 0.0f;
    correction = 1.0f;
}

// Conversion ADC -> kPa. Le zero du capteur est un OFFSET mesure et non zero : les capteurs
// de type MPX5010 sortent 0,2 a 0,5 V a pression nulle.
float PressureRegulator::readKpa() const {
    int raw = analogRead(PRESSURE_SENSOR_PIN);
    float kpa = ((float)raw - (float)PRESSURE_ADC_AT_ZERO) / PRESSURE_ADC_PER_KPA;
#if AIR_DIRECTION == AIR_DIRECTION_DRAW
    // En aspiration, la grandeur utile est la depression : on la compte positive pour que
    // consigne, mesure et correcteur restent dans le meme sens que en soufflage.
    kpa = -kpa;
#endif
    return kpa;
}

void PressureRegulator::setDemand(float demand) {
    if (demand <= 0.0f) {
        target = 0.0f;
        return;
    }
    target = demand * PRESSURE_TARGET_KPA;
    // La consigne ne doit jamais viser le plafond de securite : sinon le correcteur passe
    // sa vie a declencher la coupure.
    float ceiling = PRESSURE_MAX_KPA * 0.9f;
    if (target > ceiling) target = ceiling;
}

void PressureRegulator::update() {
    uint32_t now = millis();
    uint32_t elapsed = now - lastSampleTime;
    if (elapsed < (uint32_t)PRESSURE_SAMPLE_MS) return;
    lastSampleTime = now;

    float measured = readKpa();
    if (!primed) {
        // Premier echantillon : on initialise le filtre dessus plutot que de le laisser
        // converger depuis zero, ce qui produirait une fausse erreur au demarrage.
        filtered = measured;
        primed = true;
    } else {
        filtered += PRESSURE_FILTER_ALPHA * (measured - filtered);
    }

#if PRESSURE_CONTROL == PRESSURE_CONTROL_OPEN_LOOP
    correction = 1.0f; // Capteur present pour la surveillance seule (surpression)
    return;
#else
    // Sans consigne, il n'y a rien a reguler : on relache l'integrateur pour ne pas
    // repartir avec un terme accumule a la note suivante.
    if (target <= 0.0f) {
        reset();
        return;
    }

    float dt = (float)elapsed / 1000.0f;
    float error = target - filtered;

    // Erreur RELATIVE a la consigne : le correcteur produit un facteur multiplicatif, ses
    // gains doivent donc etre independants du niveau de pression vise.
    float relError = error / target;

    integral += PRESSURE_KI * relError * dt;
    if (integral >  PRESSURE_INTEGRAL_LIMIT) integral =  PRESSURE_INTEGRAL_LIMIT;
    if (integral < -PRESSURE_INTEGRAL_LIMIT) integral = -PRESSURE_INTEGRAL_LIMIT;

    float output = PRESSURE_KP * relError + integral;

  #if PRESSURE_CONTROL == PRESSURE_CONTROL_PID
    if (dt > 0.0f) output += PRESSURE_KD * (relError - lastError) / dt;
  #endif
    lastError = relError;

    correction = 1.0f + output;
    if (correction < PRESSURE_SCALE_MIN) correction = PRESSURE_SCALE_MIN;
    if (correction > PRESSURE_SCALE_MAX) correction = PRESSURE_SCALE_MAX;
#endif
}

float PressureRegulator::scale() const {
#if PRESSURE_CONTROL == PRESSURE_CONTROL_OPEN_LOOP
    return 1.0f;
#else
    return correction;
#endif
}

float PressureRegulator::pressureKpa() const { return primed ? filtered : 0.0f; }
float PressureRegulator::targetKpa() const { return target; }

bool PressureRegulator::overPressure() const {
    return primed && (filtered > PRESSURE_MAX_KPA);
}

#endif // PRESSURE_SENSOR_ENABLED
