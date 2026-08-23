#include "servoController.h"

#if PCA_OE_MODE == PCA_OE_PER_PCA
// Une broche OE par PCA, dans le meme ordre que PCA_ADDRESS_LIST. Permet de couper les
// anches en gardant la valve generale alimentee, ce qu'un OE partage interdit.
static const uint8_t PCA_OE_PINS[NUM_PCA_TOTAL] = {PCA_OE_PIN_LIST};
#endif

ServoController::ServoController()
    : bankMask(0), lastCommandTime(0), missingMask(0), consecutiveErrors(0) {
    for (int i = 0; i < NUM_PCA_TOTAL; i++) {
        pca[i] = Adafruit_PWMServoDriver(PCA_TAB[i]);
    }
}

// Presence d'un composant a l'adresse donnee : un simple START/STOP suffit, l'ACK du
// composant fait foi. Volontairement fait avec Wire plutot qu'avec la bibliotheque
// Adafruit, dont la signature de begin() varie selon les versions.
bool ServoController::probe(uint8_t address) {
    Wire.beginTransmission(address);
    return (Wire.endTransmission() == 0);
}

void ServoController::writeOePin(uint8_t pin, bool enabled) {
    digitalWrite(pin, enabled ? LOW : HIGH); // OE est actif bas
}

bool ServoController::isBankEnabled(uint8_t pcaIndex) const {
    if (pcaIndex >= NUM_PCA_TOTAL) return false;
    return (bankMask & (uint16_t)(1UL << pcaIndex)) != 0;
}

int8_t ServoController::indexOfAddress(uint8_t address) {
    for (int8_t i = 0; i < (int8_t)NUM_PCA_TOTAL; i++) {
        if (PCA_TAB[i] == address) return i;
    }
    return -1;
}

bool ServoController::begin() {
    // Sorties coupees pendant l'init : les servos ne doivent pas forcer avant d'avoir
    // recu leur premiere consigne.
#if PCA_OE_MODE == PCA_OE_SHARED
    pinMode(PCA_OE_PIN, OUTPUT);
    writeOePin(PCA_OE_PIN, false);
    bankMask = 0;
#elif PCA_OE_MODE == PCA_OE_PER_PCA
    for (int i = 0; i < NUM_PCA_TOTAL; i++) {
        pinMode(PCA_OE_PINS[i], OUTPUT);
        writeOePin(PCA_OE_PINS[i], false);
    }
    bankMask = 0;
#else
    // OE cable a la masse : les sorties sont toujours actives, il n'y a rien a piloter.
    bankMask = (uint16_t)~0u;
#endif

    missingMask = 0;
    consecutiveErrors = 0;

    for (int i = 0; i < NUM_PCA_TOTAL; i++) {
        if (!probe(PCA_TAB[i])) {
            missingMask |= (uint8_t)(1 << i);
            #if DEBUG
            Serial.print(F("[DEBUG] PCA9685 absent a l'adresse 0x"));
            Serial.println((int)PCA_TAB[i]);
            #endif
            continue; // Ne pas configurer un composant qui ne repond pas
        }
        pca[i].begin();
        pca[i].setPWMFreq(SERVO_PWM_FREQUENCY);
    }

    lastCommandTime = millis();

    if (missingMask != 0) return false;

    enableServos(true);
    return true;
}

// === ECRITURE EFFECTIVE ===
// setPWM() retourne le code d'erreur de l'ecriture I2C : un PCA qui cesse de repondre en
// cours de jeu laisserait sinon des anches ouvertes sans que rien ne le signale.
void ServoController::write(uint8_t pcaAddress, uint8_t channel, uint16_t onCounts,
                            uint16_t offCounts) {
    int8_t index = indexOfAddress(pcaAddress);
    if (index < 0) return; // Adresse absente de la configuration : rien a piloter

    // Reactive le banc concerne s'il avait ete coupe : sans cela la commande partirait dans
    // le vide, et l'anche resterait fermee sans que rien ne le signale.
    if (!isBankEnabled((uint8_t)index)) enableBank((uint8_t)index, true);

    uint8_t result = pca[index].setPWM(channel, onCounts, offCounts);
    if (result != 0) {
        if (consecutiveErrors < 255) consecutiveErrors++;
    } else {
        consecutiveErrors = 0;
    }
    lastCommandTime = millis(); // Redemarre le compte a rebours de coupure de l'OE
}

// Definit l'angle d'un servo, en reactivant les PCA si necessaire
void ServoController::setServoAngle(uint8_t pcaAddress, uint8_t channel, uint16_t angle) {
    // Borne l'angle : une entree de mapping erronee ne doit pas produire un PWM hors plage.
    uint16_t safeAngle = constrain((int)angle, SERVO_MIN_ANGLE, SERVO_MAX_ANGLE);
    int pwmValue = map(safeAngle, SERVO_MIN_ANGLE, SERVO_MAX_ANGLE, SERVO_MIN_PWM, SERVO_MAX_PWM);
    write(pcaAddress, channel, 0, (uint16_t)pwmValue);
}

void ServoController::setPulseMicroseconds(uint8_t pcaAddress, uint8_t channel,
                                           uint16_t pulseUs) {
    write(pcaAddress, channel, 0, pulseUsToCounts(pulseUs));
}

void ServoController::setRawDuty(uint8_t pcaAddress, uint8_t channel, uint16_t counts) {
    // Le PCA9685 dispose de bits "full ON" / "full OFF" (0x1000) : les utiliser evite un
    // resteau de commutation aux extremes, ce qui compte pour un electroaimant.
    if (counts == 0)      { write(pcaAddress, channel, 0, 4096); return; }
    if (counts >= 4095)   { write(pcaAddress, channel, 4096, 0); return; }
    write(pcaAddress, channel, 0, counts);
}

// === SORTIES PWM (OE) ===
void ServoController::enableServos(bool state) {
    // 1UL et non 1u : sur AVR un `unsigned int` fait 16 bits, et decaler de 16 y est
    // un comportement indefini.
    uint16_t wanted = state ? (uint16_t)((1UL << NUM_PCA_TOTAL) - 1UL) : 0u;

#if PCA_OE_MODE == PCA_OE_NONE
    (void)wanted; // Rien a piloter : l'OE est cable a la masse, les sorties restent actives
#else
    if (bankMask == wanted) return; // Evite les ecritures inutiles

  #if PCA_OE_MODE == PCA_OE_SHARED
    writeOePin(PCA_OE_PIN, state);
  #else
    for (int i = 0; i < NUM_PCA_TOTAL; i++) writeOePin(PCA_OE_PINS[i], state);
  #endif
    bankMask = wanted;
#endif
}

void ServoController::enableBank(uint8_t pcaIndex, bool state) {
    if (pcaIndex >= NUM_PCA_TOTAL) return;

#if PCA_OE_MODE == PCA_OE_PER_PCA
    uint16_t bit = (uint16_t)(1UL << pcaIndex);
    if (((bankMask & bit) != 0) == state) return;
    writeOePin(PCA_OE_PINS[pcaIndex], state);
    if (state) bankMask |= bit; else bankMask &= (uint16_t)~bit;
#else
    // Sans broche par PCA, couper un banc seul est physiquement impossible : la demande
    // porte forcement sur l'ensemble. C'est aussi pour cela que couper l'OE partage coupe
    // la valve generale en meme temps que les anches.
    enableServos(state);
#endif
}
