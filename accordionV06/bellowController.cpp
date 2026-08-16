#include "bellowController.h"
#include <math.h>

// === CONSTRUCTEUR ===
BellowController::BellowController(ServoController &servoCtrl)
    : servoController(servoCtrl), state(BELLOW_INIT), stateBeforeStop(BELLOW_INIT),
      fault(FAULT_NONE),
      valveOpen(false), movingDirection(true), motorRunning(false), motorEnabled(false),
      airDemand(0.0f), volume(100), expression(127),
      maxSpeed(STEPPER_MAX_SPEED), homingStartTime(0), stopStartTime(0),
      lastEndstopMinTime(0), lastEndstopMaxTime(0),
      rawEndstopMin(false), rawEndstopMax(false),
      stableEndstopMin(false), stableEndstopMax(false) {}

bool BellowController::isHoming() const {
    return state == BELLOW_HOMING_FAST || state == BELLOW_HOMING_BACKOFF ||
           state == BELLOW_HOMING_SLOW ||
           (state == BELLOW_STOPPING &&
            (stateBeforeStop == BELLOW_HOMING_FAST || stateBeforeStop == BELLOW_HOMING_BACKOFF ||
             stateBeforeStop == BELLOW_HOMING_SLOW));
}

// === INITIALISATION ===
void BellowController::begin() {
    stepper.connectToPins(STEPPER_STEP_PIN, STEPPER_DIR_PIN);
    stepper.setStepsPerMillimeter(STEPS_PER_MM);

    // Borne la vitesse maximale au debit de pas realiste.
    // FlexyStepper ne produit qu'un pas par appel a processMovement() : demander
    // STEPPER_MAX_SPEED * STEPS_PER_MM pas/s sans verifier menerait a un decrochage
    // silencieux du moteur.
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
    // -------------------------------------------------------------------------------
    // 1. SECURITE FIN DE COURSE, sur lecture BRUTE.
    //    L'arret ne doit dependre ni du debounce (50 ms = 1,25 mm a pleine vitesse) ni de
    //    l'etat interne de FlexyStepper. Le debounce ne sert qu'a interpreter ensuite.
    // -------------------------------------------------------------------------------
    if (state != BELLOW_STOPPING && state != BELLOW_FAULT && state != BELLOW_INIT) {
        bool rawMin = (digitalRead(LIMIT_SWITCH_MIN_PIN) == LOW);
        bool rawMax = (digitalRead(LIMIT_SWITCH_MAX_PIN) == LOW);

        if ((rawMin && drivingInto(true)) || (rawMax && drivingInto(false))) {
            beginEmergencyStop();
        }
    }

    // 2. Debounce, pour l'interpretation (contact reel ou parasite ?)
    updateEndstops();

    // Les deux contacts actifs simultanement : impossible mecaniquement.
    if (state != BELLOW_FAULT && stableEndstopMin && stableEndstopMax) {
        setFault(FAULT_ENDSTOP_WIRING);
        return;
    }

    // 3. Machine a etats
    switch (state) {
        case BELLOW_HOMING_FAST:
        case BELLOW_HOMING_SLOW:
            updateHomingTravel();
            break;

        case BELLOW_HOMING_BACKOFF:
            updateBackoff();
            break;

        case BELLOW_STOPPING:
            updateStopping();
            break;

        case BELLOW_READY:
            // L'inversion 30/70% est evaluee en continu : une note tenue ne genere aucun
            // evenement MIDI et doit malgre tout faire osciller le soufflet.
            updateDirection();
            break;

        default:
            break;
    }

    // 4. Generateur de pas
    if (state != BELLOW_FAULT) {
        serviceStepper();
    }
}

// === SERVICE DU GENERATEUR DE PAS ===
// FlexyStepper ne produit qu'UN pas par appel. Plusieurs appels par tour de boucle
// permettent de rattraper le temps passe dans le MIDI et les ecritures I2C. Les appels
// superflus sont quasi gratuits : processMovement() est auto-limite par micros().
void BellowController::serviceStepper() {
    for (uint8_t i = 0; i < STEPPER_SERVICE_CALLS; i++) {
        if (stepper.processMovement()) break; // Mouvement termine : inutile d'insister
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

// === SENS DU MOUVEMENT PAR RAPPORT A UN CONTACT ===
bool BellowController::drivingInto(bool minSwitch) const {
    switch (state) {
        case BELLOW_HOMING_FAST:
        case BELLOW_HOMING_SLOW:
            // Toute butee rencontree pendant une approche doit couper le moteur, y compris
            // la butee HAUTE : si le cablage DIR est inverse, c'est elle que l'on percute.
            (void)minSwitch;
            return true;
        case BELLOW_HOMING_BACKOFF:
            return !minSwitch; // Le degagement s'eloigne du contact bas, attendu actif
        case BELLOW_READY:
            if (!motorRunning) return false; // A l'arret, reposer sur un contact est normal
            return minSwitch ? !movingDirection : movingDirection;
        default:
            return false;
    }
}

// =========================================================================================
// SEQUENCE D'ARRET
// -----------------------------------------------------------------------------------------
// FlexyStepper conserve son propre etat de direction et de rampe. Changer la position
// courante ou la cible pendant un mouvement ne l'arrete pas : la bibliotheque documente
// d'ailleurs que setCurrentPosition() ne doit etre appelee qu'a l'arret.
//
// D'ou la sequence :
//   contact brut -> coupure ENABLE (arret physique, immediat)
//                -> purge de l'etat cinematique, driver deja coupe (aucun mouvement)
//                -> attente de la confirmation par debounce
//                -> recalage de position, puis reprise ou defaut
// =========================================================================================
void BellowController::beginEmergencyStop() {
    // Action de securite : coupure du couple, avant tout autre traitement.
    disableMotor();

    // Purge de l'etat interne de FlexyStepper. Le driver est deja coupe : les pas emis ici
    // ne produisent aucun deplacement, ils ne servent qu'a ramener la bibliotheque a
    // directionOfMotion = 0, seul etat ou setCurrentPosition() est legitime.
    stepper.setAccelerationInMillimetersPerSecondPerSecond(STEPPER_EMERGENCY_DECEL);
    stepper.setTargetPositionToStop();

    stateBeforeStop = state;
    stopStartTime = millis();
    motorRunning = false;
    state = BELLOW_STOPPING;

    #if DEBUG
    Serial.println(F("[DEBUG] Arret d'urgence fin de course"));
    #endif
}

void BellowController::updateStopping() {
    // 1. Attendre que la bibliotheque soit reellement a l'arret.
    if (!stepper.motionComplete()) {
        if ((millis() - stopStartTime) > STEPPER_STOP_TIMEOUT_MS) {
            setFault(FAULT_STOP_TIMEOUT);
        }
        return;
    }

    // 2. Laisser le debounce trancher sur la validite du contact.
    if ((millis() - stopStartTime) < (uint32_t)(ENDSTOP_DEBOUNCE_MS + 5)) return;

    // A partir d'ici FlexyStepper est a l'arret : setCurrentPosition() est sur.
    if (stableEndstopMin) { onEndstopConfirmed(true);  return; }
    if (stableEndstopMax) { onEndstopConfirmed(false); return; }

    // Aucun contact confirme : parasite electrique. On reprend ou on en etait.
    // La position peut avoir derive des quelques pas de la purge (driver coupe) ; l'ecart
    // est inferieur au dixieme de millimetre et sera efface au prochain contact reel.
    #if DEBUG
    Serial.println(F("[DEBUG] Fin de course parasite, reprise"));
    #endif
    resumeAfterStop();
}

void BellowController::onEndstopConfirmed(bool isMin) {
    if (!isMin) {
        // Butee HAUTE atteinte pendant le homing : on cherchait la butee BASSE.
        // Cablage DIR inverse, phases moteur permutees, ou fins de course intervertis.
        if (stateBeforeStop == BELLOW_HOMING_FAST || stateBeforeStop == BELLOW_HOMING_SLOW ||
            stateBeforeStop == BELLOW_HOMING_BACKOFF) {
            setFault(FAULT_HOMING_DIRECTION);
            return;
        }
        stepper.setCurrentPositionInMillimeters(BELLOW_MAX_POSITION);
        stepper.setTargetPositionInMillimeters(BELLOW_MAX_POSITION);
        movingDirection = false; // On repart en fermeture
        resumeAfterStop();
        return;
    }

    // --- Contact BAS confirme ---
    if (stateBeforeStop == BELLOW_HOMING_FAST) {
        // Premier contact : position encore inconnue (surcourse non maitrisee).
        // On se degage puis on reapproche lentement pour fixer un zero repetable.
        startBackoff();
        return;
    }

    if (stateBeforeStop == BELLOW_HOMING_SLOW) {
        stepper.setCurrentPositionInMillimeters(BELLOW_MIN_POSITION);
        stepper.setTargetPositionInMillimeters(BELLOW_MIN_POSITION);
        movingDirection = true;
        finishHoming();
        return;
    }

    // En jeu : recalage et inversion.
    stepper.setCurrentPositionInMillimeters(BELLOW_MIN_POSITION);
    stepper.setTargetPositionInMillimeters(BELLOW_MIN_POSITION);
    movingDirection = true;
    resumeAfterStop();
}

void BellowController::resumeAfterStop() {
    stepper.setAccelerationInMillimetersPerSecondPerSecond(STEPPER_MIN_ACCEL);

    switch (stateBeforeStop) {
        case BELLOW_HOMING_FAST:    startFastApproach(); return;
        case BELLOW_HOMING_BACKOFF: startBackoff();      return;
        case BELLOW_HOMING_SLOW:    startSlowApproach(); return;
        default: break;
    }

    state = BELLOW_READY;
    applyDemand(); // Reactive le moteur s'il reste une demande d'air
}

// =========================================================================================
// HOMING EN DEUX PASSES
// =========================================================================================
void BellowController::startHoming() {
    openValve(); // Sans pression : le soufflet doit pouvoir se fermer librement
    fault = FAULT_NONE;
    homingStartTime = millis();
    startFastApproach();
}

void BellowController::startFastApproach() {
    stepper.setSpeedInMillimetersPerSecond(HOMING_SPEED);
    stepper.setAccelerationInMillimetersPerSecondPerSecond(STEPPER_MIN_ACCEL);
    stepper.setCurrentPositionInMillimeters(0);
    stepper.setTargetPositionInMillimeters(-HOMING_MAX_DISTANCE);
    movingDirection = false; // Fermeture
    motorRunning = false;
    state = BELLOW_HOMING_FAST;
    enableMotor();
}

void BellowController::startBackoff() {
    // On ne connait pas encore le zero : on se contente de s'ecarter du contact.
    stepper.setSpeedInMillimetersPerSecond(HOMING_SLOW_SPEED);
    stepper.setAccelerationInMillimetersPerSecondPerSecond(STEPPER_MIN_ACCEL);
    stepper.setCurrentPositionInMillimeters(0);
    stepper.setTargetPositionInMillimeters(HOMING_BACKOFF_MM);
    movingDirection = true; // Ouverture
    motorRunning = false;
    state = BELLOW_HOMING_BACKOFF;
    enableMotor();
}

void BellowController::startSlowApproach() {
    stepper.setSpeedInMillimetersPerSecond(HOMING_SLOW_SPEED);
    stepper.setAccelerationInMillimetersPerSecondPerSecond(STEPPER_MIN_ACCEL);
    stepper.setCurrentPositionInMillimeters(0);
    // Course volontairement courte : on est deja a HOMING_BACKOFF_MM du contact.
    stepper.setTargetPositionInMillimeters(-(HOMING_BACKOFF_MM * 3.0f));
    movingDirection = false;
    motorRunning = false;
    state = BELLOW_HOMING_SLOW;
    enableMotor();
}

// Surveillance des deux approches. Le contact lui-meme est traite par l'arret d'urgence.
void BellowController::updateHomingTravel() {
    // Course maximale parcourue sans jamais rencontrer la butee : la cible est atteinte.
    // (L'ancien test `fabs(position) > HOMING_MAX_DISTANCE` etait inatteignable, puisque la
    // cible valait exactement -HOMING_MAX_DISTANCE : le defaut obtenu etait toujours un
    // timeout, jamais une distance.)
    if (stepper.motionComplete()) {
        setFault(FAULT_HOMING_DISTANCE);
        return;
    }

    if ((millis() - homingStartTime) > HOMING_TIMEOUT_MS) {
        setFault(FAULT_HOMING_TIMEOUT);
    }
}

void BellowController::updateBackoff() {
    if (!stepper.motionComplete()) {
        if ((millis() - homingStartTime) > HOMING_TIMEOUT_MS) {
            setFault(FAULT_HOMING_TIMEOUT);
        }
        return;
    }

    // Degagement termine : le contact doit s'etre relache.
    if (stableEndstopMin) {
        setFault(FAULT_ENDSTOP_STUCK);
        return;
    }
    startSlowApproach();
}

void BellowController::finishHoming() {
    stepper.setAccelerationInMillimetersPerSecondPerSecond(STEPPER_MIN_ACCEL);
    state = BELLOW_READY;
    applyDemand(); // Reprend une eventuelle demande d'air recue pendant le homing

    #if DEBUG
    Serial.println(F("[DEBUG] Homing termine"));
    #endif
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

// === CIBLE COURANTE ===
void BellowController::retarget() {
    if (!motorRunning) return;
    stepper.setTargetPositionInMillimeters(movingDirection ? (float)BELLOW_MAX_POSITION
                                                           : (float)BELLOW_MIN_POSITION);
}

// === DEMANDE D'AIR ===
void BellowController::setAirDemand(float demand) {
    airDemand = demand;
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
    // Pendant le homing, un arret ou en defaut, la demande est memorisee mais pas appliquee.
    if (state != BELLOW_READY) return;

    // CC7 = 0 (ou CC11 = 0) doit reellement couper la production de pression.
    if (airDemand <= 0.0f || volume == 0 || expression == 0) {
        stopPressure();
        return;
    }

    float scale = (volume / 127.0f) * (expression / 127.0f);
    float speed = (float)NORMAL_SPEED * airDemand * scale;
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
    // Deceleration controlee par la bibliotheque, au lieu d'une cible ramenee brutalement
    // sur la position courante.
    stepper.setTargetPositionToStop();
    motorRunning = false;
}

// === MISE AU REPOS ===
void BellowController::stopAndDisable() {
    stopPressure();
    disableMotor();
}

// === DEFAUT VERROUILLE ===
void BellowController::setFault(BellowFault code) {
    if (state == BELLOW_FAULT) return; // Premier defaut conserve
    fault = code;
    state = BELLOW_FAULT;

    motorRunning = false;
    disableMotor(); // Coupure du couple avant tout
    openValve();    // Libere la pression residuelle

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
