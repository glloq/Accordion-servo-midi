#include "bellowController.h"
#include <math.h>

// === CONSTRUCTEUR ===
BellowController::BellowController(ServoController &servoCtrl)
    : servoController(servoCtrl), state(BELLOW_INIT), fault(FAULT_NONE),
      valveOpen(false), movingDirection(true), motorRunning(false), motorEnabled(false),
      airDemand(0.0f), velocityFactor(1.0f), volume(100), expression(127),
      maxSpeed(STEPPER_MAX_SPEED), homingStartTime(0),
      lastEndstopMinTime(0), lastEndstopMaxTime(0),
      rawEndstopMin(false), rawEndstopMax(false),
      stableEndstopMin(false), stableEndstopMax(false) {}

// === INITIALISATION ===
void BellowController::begin() {
    stepper.connectToPins(STEPPER_STEP_PIN, STEPPER_DIR_PIN);
    stepper.setStepsPerMillimeter(STEPS_PER_MM);

    // Borne la vitesse maximale au debit de pas realiste du MCU.
    // Sans cela, STEPPER_MAX_SPEED * STEPS_PER_MM peut demander plus de 100 kHz de pas,
    // que FlexyStepper ne peut pas generer sur AVR : le moteur decroche silencieusement.
    maxSpeed = STEPPER_MAX_STEP_RATE_HZ / STEPS_PER_MM;
    if (maxSpeed > (float)STEPPER_MAX_SPEED) maxSpeed = (float)STEPPER_MAX_SPEED;
    if (maxSpeed < (float)STEPPER_MIN_SPEED) maxSpeed = (float)STEPPER_MIN_SPEED;

    stepper.setSpeedInMillimetersPerSecond(HOMING_SPEED);
    stepper.setAccelerationInMillimetersPerSecondPerSecond(STEPPER_MIN_ACCEL);

    pinMode(LIMIT_SWITCH_MIN_PIN, INPUT_PULLUP);
    pinMode(LIMIT_SWITCH_MAX_PIN, INPUT_PULLUP);
    pinMode(STEPPER_EN_PIN, OUTPUT);
    digitalWrite(STEPPER_EN_PIN, HIGH); // Driver coupe tant que begin() n'a pas fini
    motorEnabled = false;

    // Amorce le debounce avec l'etat reel des contacts : un fin de course deja enfonce
    // au demarrage est ainsi detecte immediatement, sans attendre une transition.
    rawEndstopMin = stableEndstopMin = (digitalRead(LIMIT_SWITCH_MIN_PIN) == LOW);
    rawEndstopMax = stableEndstopMax = (digitalRead(LIMIT_SWITCH_MAX_PIN) == LOW);
    lastEndstopMinTime = lastEndstopMaxTime = millis();

    startHoming();
}

// === BOUCLE PRINCIPALE ===
void BellowController::update() {
    updateEndstops();

    switch (state) {
        case BELLOW_HOMING:
            updateHoming();
            break;

        case BELLOW_READY:
            // L'inversion 30/70% doit etre evaluee en continu : la version precedente ne la
            // testait que dans updateSpeed(), donc uniquement sur evenement MIDI. Une note
            // tenue traversait le seuil sans jamais inverser et finissait sur le fin de course.
            updateDirection();
            checkEndStops();
            break;

        default:
            break;
    }

    if (state != BELLOW_FAULT) {
        stepper.processMovement();
    }
}

// === ACTIVATION / COUPURE DU DRIVER ===
void BellowController::enableMotor() {
    if (motorEnabled) return;
    digitalWrite(STEPPER_EN_PIN, LOW); // ENABLE actif bas
    motorEnabled = true;
}

void BellowController::disableMotor() {
    if (!motorEnabled) return;
    digitalWrite(STEPPER_EN_PIN, HIGH);
    motorEnabled = false;
}

// === DEBOUNCE DES FINS DE COURSE ===
void BellowController::updateEndstops() {
    uint32_t now = millis();

    bool minLevel = (digitalRead(LIMIT_SWITCH_MIN_PIN) == LOW);
    if (minLevel != rawEndstopMin) {
        rawEndstopMin = minLevel;
        lastEndstopMinTime = now;
    } else if ((now - lastEndstopMinTime) > ENDSTOP_DEBOUNCE_MS) {
        stableEndstopMin = minLevel;
    }

    bool maxLevel = (digitalRead(LIMIT_SWITCH_MAX_PIN) == LOW);
    if (maxLevel != rawEndstopMax) {
        rawEndstopMax = maxLevel;
        lastEndstopMaxTime = now;
    } else if ((now - lastEndstopMaxTime) > ENDSTOP_DEBOUNCE_MS) {
        stableEndstopMax = maxLevel;
    }
}

// === DEMARRAGE DU HOMING (NON BLOQUANT) ===
void BellowController::startHoming() {
    openValve(); // Sans pression : le soufflet doit pouvoir se fermer librement
    enableMotor();

    stepper.setSpeedInMillimetersPerSecond(HOMING_SPEED);
    stepper.setAccelerationInMillimetersPerSecondPerSecond(STEPPER_MIN_ACCEL);
    stepper.setCurrentPositionInMillimeters(0);
    stepper.setTargetPositionInMillimeters(-HOMING_MAX_DISTANCE);

    motorRunning = false;
    homingStartTime = millis();
    fault = FAULT_NONE;
    state = BELLOW_HOMING;
}

// === PROGRESSION DU HOMING ===
void BellowController::updateHoming() {
    // Les deux fins de course ne peuvent pas etre atteints en meme temps : cablage ou
    // capteur defectueux. On refuse de continuer a pousser le soufflet.
    if (stableEndstopMin && stableEndstopMax) {
        setFault(FAULT_ENDSTOP_WIRING);
        return;
    }

    if (stableEndstopMin) {
        stepper.setCurrentPositionInMillimeters(BELLOW_MIN_POSITION);
        stepper.setTargetPositionInMillimeters(BELLOW_MIN_POSITION);
        movingDirection = true; // Le prochain mouvement sera une ouverture
        state = BELLOW_READY;

        // Restaure les parametres de jeu et reprend une eventuelle demande d'air recue
        // pendant le homing.
        stepper.setAccelerationInMillimetersPerSecondPerSecond(STEPPER_MIN_ACCEL);
        applyDemand();
        return;
    }

    if ((millis() - homingStartTime) > HOMING_TIMEOUT_MS) {
        setFault(FAULT_HOMING_TIMEOUT);
        return;
    }

    if (fabs(stepper.getCurrentPositionInMillimeters()) > HOMING_MAX_DISTANCE) {
        setFault(FAULT_HOMING_DISTANCE);
        return;
    }
}

// === INVERSION 30/70% ===
void BellowController::updateDirection() {
    if (!motorRunning) return;

    float position = stepper.getCurrentPositionInMillimeters();
    float normalized = (position - (float)BELLOW_MIN_POSITION) /
                       ((float)BELLOW_MAX_POSITION - (float)BELLOW_MIN_POSITION);

    if (movingDirection && normalized >= BELLOW_REVERSE_THRESHOLD_OPEN) {
        movingDirection = false; // On commence a refermer le soufflet
        retarget();
    } else if (!movingDirection && normalized <= BELLOW_REVERSE_THRESHOLD_CLOSE) {
        movingDirection = true; // On commence a rouvrir le soufflet
        retarget();
    }
}

// === FINS DE COURSE EN JEU ===
// Filet de securite si les seuils 30/70% ont ete franchis (course mal calibree, derive de
// pas). On recale la position ET on repart dans l'autre sens, au lieu de rester bloque.
void BellowController::checkEndStops() {
    if (stableEndstopMin && stableEndstopMax) {
        setFault(FAULT_ENDSTOP_WIRING);
        return;
    }

    if (stableEndstopMin) {
        stepper.setCurrentPositionInMillimeters(BELLOW_MIN_POSITION);
        if (!movingDirection) {
            movingDirection = true;
            retarget();
        }
    }

    if (stableEndstopMax) {
        stepper.setCurrentPositionInMillimeters(BELLOW_MAX_POSITION);
        if (movingDirection) {
            movingDirection = false;
            retarget();
        }
    }
}

// === CIBLE COURANTE ===
void BellowController::retarget() {
    if (!motorRunning) return;
    stepper.setTargetPositionInMillimeters(movingDirection ? (float)BELLOW_MAX_POSITION
                                                           : (float)BELLOW_MIN_POSITION);
}

// === DEMANDE D'AIR ===
void BellowController::setAirDemand(float demand, float velFactor) {
    airDemand = demand;
    velocityFactor = velFactor;
    applyDemand();
}

void BellowController::setVolume(byte volumeValue) {
    volume = volumeValue;
    applyDemand();
}

void BellowController::setExpression(byte expressionValue) {
    expression = expressionValue;
    applyDemand();
}

// === CALCUL DE LA VITESSE ET DE LA CIBLE ===
void BellowController::applyDemand() {
    // Pendant le homing ou en defaut, la demande est memorisee mais pas appliquee.
    if (state != BELLOW_READY) return;

    // CC7 = 0 (ou CC11 = 0) doit reellement couper la production de pression.
    // L'ancien code appliquait constrain(0, STEPPER_MIN_SPEED, ...) et laissait donc le
    // soufflet avancer a 2 mm/s a volume nul.
    if (airDemand <= 0.0f || volume == 0 || expression == 0) {
        stopPressure();
        return;
    }

    float scale = (volume / 127.0f) * (expression / 127.0f);
    float speed = (float)NORMAL_SPEED * airDemand * velocityFactor * scale;
    speed = constrain(speed, (float)STEPPER_MIN_SPEED, maxSpeed);

    // Acceleration proportionnelle a la vitesse demandee : attaque franche a fort debit,
    // demarrage doux a faible debit.
    float span = maxSpeed - (float)STEPPER_MIN_SPEED;
    float ratio = (span > 0.0f) ? ((speed - (float)STEPPER_MIN_SPEED) / span) : 0.0f;
    float accel = (float)STEPPER_MIN_ACCEL + ratio * ((float)STEPPER_MAX_ACCEL - (float)STEPPER_MIN_ACCEL);

    enableMotor(); // Indispensable : le driver a pu etre coupe par l'inactivite
    stepper.setSpeedInMillimetersPerSecond(speed);
    stepper.setAccelerationInMillimetersPerSecondPerSecond(accel);
    motorRunning = true;
    retarget();
}

// === ARRET DE LA PRODUCTION DE PRESSION (DRIVER TOUJOURS ALIMENTE) ===
void BellowController::stopPressure() {
    if (!motorRunning) return;
    stepper.setTargetPositionInMillimeters(stepper.getCurrentPositionInMillimeters());
    motorRunning = false;
}

// === MISE AU REPOS ===
void BellowController::stopAndDisable() {
    stopPressure();
    disableMotor();
}

// === DEFAUT VERROUILLE ===
void BellowController::setFault(BellowFault code) {
    fault = code;
    state = BELLOW_FAULT;

    stepper.setTargetPositionInMillimeters(stepper.getCurrentPositionInMillimeters());
    motorRunning = false;
    disableMotor();
    openValve(); // Libere la pression residuelle

    #if DEBUG
    Serial.print(F("[DEBUG] Defaut soufflet: "));
    Serial.println((int)code);
    #endif
}

void BellowController::clearFault() {
    if (state != BELLOW_FAULT) return;
    fault = FAULT_NONE;
    startHoming();
}

// === OUVERTURE DE LA VALVE ===
void BellowController::openValve() {
    servoController.setServoAngle(VALVE_PCA_ADDRESS, VALVE_PCA_PIN, VALVE_PCA_ANGLE_OPEN);
    valveOpen = true;
}

// === FERMETURE DE LA VALVE ===
void BellowController::closeValve() {
    servoController.setServoAngle(VALVE_PCA_ADDRESS, VALVE_PCA_PIN, VALVE_PCA_ANGLE_CLOSE);
    valveOpen = false;
}
