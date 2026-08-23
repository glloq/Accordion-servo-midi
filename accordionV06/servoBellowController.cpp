#include "servoBellowController.h"
#if AIR_SOURCE == AIR_SOURCE_BELLOW_SERVO

#include <math.h>

// Course signee du soufflet : positive si l'ouverture correspond a un angle croissant.
// Tout le raisonnement interne se fait en ANGLES, pas en fraction de course : c'est la
// grandeur reellement envoyee au servo, et cela evite une conversion a chaque increment.
static const float SERVO_BELLOW_SPAN =
    (float)BELLOW_SERVO_ANGLE_OPEN - (float)BELLOW_SERVO_ANGLE_CLOSED;

ServoBellowController::ServoBellowController(ServoController &servoCtrl, AirValve &valve,
                                             PressureRegulator &regulator)
    : servoController(servoCtrl), airValve(valve), pressure(regulator),
      angle((float)BELLOW_SERVO_ANGLE_CLOSED), movingDirection(true), running(false),
      ready(false), fault(FAULT_NONE),
      airDemand(0.0f), volume(100), expression(127), lastUpdate(0) {}

float ServoBellowController::getOpening() const {
    return (angle - (float)BELLOW_SERVO_ANGLE_CLOSED) / SERVO_BELLOW_SPAN;
}

void ServoBellowController::begin() {
    pressure.begin();
    startCalibration();
}

// "Calibration" reduite a une mise en position connue : un servo ne cherche pas son zero.
// La methode existe pour offrir la meme interface que le soufflet pas a pas, et parce que
// clearFault() doit pouvoir remettre la mecanique dans un etat sur.
void ServoBellowController::startCalibration() {
    airValve.open();
    pressure.reset();
    angle = (float)BELLOW_SERVO_ANGLE_CLOSED;
    movingDirection = true;
    running = false;
    lastUpdate = millis();
    writeAngle();
    ready = true;
}

void ServoBellowController::writeAngle() {
    float clamped = angle;
    // Bornage dans l'ordre reel des angles, quel que soit le sens de montage.
    float lo = (SERVO_BELLOW_SPAN > 0.0f) ? (float)BELLOW_SERVO_ANGLE_CLOSED
                                          : (float)BELLOW_SERVO_ANGLE_OPEN;
    float hi = (SERVO_BELLOW_SPAN > 0.0f) ? (float)BELLOW_SERVO_ANGLE_OPEN
                                          : (float)BELLOW_SERVO_ANGLE_CLOSED;
    if (clamped < lo) clamped = lo;
    if (clamped > hi) clamped = hi;
    angle = clamped;

    servoController.setServoAngle(BELLOW_SERVO_PCA_ADDRESS, BELLOW_SERVO_PCA_PIN,
                                  (uint16_t)(clamped + 0.5f));
}

// Vitesse de balayage pour la demande courante. La demande d'air d'une seule note vaut
// environ 1.0 : BELLOW_SERVO_MIN_SPEED_DPS est donc atteinte des la premiere note, et la
// pleine vitesse quand plusieurs anches consomment ensemble.
float ServoBellowController::sweepSpeed() const {
    float scale = (volume / 127.0f) * (expression / 127.0f);
    float demand = airDemand * scale * pressure.scale();
    if (demand <= 0.0f) return 0.0f;

    float speed = BELLOW_SERVO_MIN_SPEED_DPS +
                  (demand - 1.0f) * (BELLOW_SERVO_MAX_SPEED_DPS - BELLOW_SERVO_MIN_SPEED_DPS) /
                  ((float)BELLOW_SERVO_DEMAND_FULL_SCALE - 1.0f);
    if (speed < BELLOW_SERVO_MIN_SPEED_DPS) speed = BELLOW_SERVO_MIN_SPEED_DPS;
    if (speed > BELLOW_SERVO_MAX_SPEED_DPS) speed = BELLOW_SERVO_MAX_SPEED_DPS;
    return speed;
}

void ServoBellowController::update() {
    if (fault != FAULT_NONE) return;

    pressure.update();
    if (pressure.overPressure()) {
        setFault(FAULT_OVERPRESSURE);
        return;
    }

    uint32_t now = millis();
    uint32_t elapsed = now - lastUpdate;
    if (elapsed < (uint32_t)BELLOW_SERVO_UPDATE_MS) return;
    lastUpdate = now;

    if (!running) return;

    float speed = sweepSpeed();
    if (speed <= 0.0f) return;

    // Increment d'angle pour la periode ecoulee, dans le sens de deplacement courant.
    float step = speed * ((float)elapsed / 1000.0f);
    float delta = (SERVO_BELLOW_SPAN > 0.0f) ? step : -step;
    angle += movingDirection ? delta : -delta;

    // Inversion avant les extremes, exactement comme les seuils 30/70 % du soufflet pas a
    // pas : evaluee en continu, une note tenue fait osciller le soufflet sans jamais taper
    // dans les butees d'angle.
    float opening = getOpening();
    if (movingDirection && opening >= BELLOW_SERVO_REVERSE_OPEN) {
        movingDirection = false;
    } else if (!movingDirection && opening <= BELLOW_SERVO_REVERSE_CLOSE) {
        movingDirection = true;
    }

    writeAngle();
}

void ServoBellowController::setAirDemand(float demand) {
    airDemand = demand;
    pressure.setDemand(demand);

    bool wantRunning = (demand > 0.0f && volume > 0 && expression > 0);
    if (wantRunning && !running) lastUpdate = millis(); // Pas de rattrapage d'un long arret
    running = wantRunning;
    if (!running) pressure.reset();
}

void ServoBellowController::setVolume(byte volumeValue) {
    volume = volumeValue;
    setAirDemand(airDemand); // Re-evalue running : CC7 = 0 doit reellement arreter
}

void ServoBellowController::setExpression(byte expressionValue) {
    expression = expressionValue;
    setAirDemand(airDemand);
}

// Mise au repos : le soufflet revient a l'angle ferme, ce qui est aussi la position la
// moins contraignante pour la mecanique.
void ServoBellowController::stopAndDisable() {
    running = false;
    pressure.reset();
    angle = (float)BELLOW_SERVO_ANGLE_CLOSED;
    writeAngle();
}

void ServoBellowController::openValve()  { airValve.open(); }
void ServoBellowController::closeValve() { airValve.close(); }

void ServoBellowController::setFault(AirFault code) {
    if (fault != FAULT_NONE) return; // Premier defaut conserve
    fault = code;
    ready = false;
    running = false;
    airValve.open(); // Libere la pression residuelle
    pressure.reset();

    #if DEBUG
    Serial.print(F("[DEBUG] Defaut soufflet a servo: "));
    Serial.println((int)code);
    #endif
}

void ServoBellowController::clearFault() {
    if (fault == FAULT_NONE) return;
    fault = FAULT_NONE;
    startCalibration();
}

#endif // AIR_SOURCE_BELLOW_SERVO
