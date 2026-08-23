#include "servoController.h"

#if PCA_OE_MODE == PCA_OE_PER_PCA
// Une broche OE par PCA, dans le meme ordre que PCA_ADDRESS_LIST. Permet de couper les
// anches en gardant la valve generale alimentee, ce qu'un OE partage interdit.
static const uint8_t PCA_OE_PINS[NUM_PCA_TOTAL] = {PCA_OE_PIN_LIST};
#endif

ServoController::ServoController()
    : pcaEnabled(true), lastCommandTime(0), missingMask(0), consecutiveErrors(0) {
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
    digitalWrite(PCA_OE_PIN, HIGH); // OE actif bas
    pcaEnabled = false;
#elif PCA_OE_MODE == PCA_OE_PER_PCA
    for (int i = 0; i < NUM_PCA_TOTAL; i++) {
        pinMode(PCA_OE_PINS[i], OUTPUT);
        digitalWrite(PCA_OE_PINS[i], HIGH);
    }
    pcaEnabled = false;
#else
    // OE cable a la masse : les sorties sont toujours actives, il n'y a rien a piloter.
    pcaEnabled = true;
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
    if (!pcaEnabled) {
        enableServos(true); // Reactive les sorties si elles avaient ete coupees
    }

    int8_t index = indexOfAddress(pcaAddress);
    if (index < 0) return; // Adresse absente de la configuration : rien a piloter

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
#if PCA_OE_MODE == PCA_OE_NONE
    (void)state; // Rien a piloter : l'OE est cable a la masse
#else
    if (pcaEnabled == state) return; // Evite les changements inutiles

  #if PCA_OE_MODE == PCA_OE_SHARED
    digitalWrite(PCA_OE_PIN, state ? LOW : HIGH); // OE est actif bas
  #else
    for (int i = 0; i < NUM_PCA_TOTAL; i++) {
        digitalWrite(PCA_OE_PINS[i], state ? LOW : HIGH);
    }
  #endif
    pcaEnabled = state;
#endif
}

void ServoController::enableBank(uint8_t pcaIndex, bool state) {
#if PCA_OE_MODE == PCA_OE_PER_PCA
    if (pcaIndex >= NUM_PCA_TOTAL) return;
    digitalWrite(PCA_OE_PINS[pcaIndex], state ? LOW : HIGH);
    // pcaEnabled decrit l'etat d'ensemble : des qu'un banc est actif, une commande peut
    // partir sans reactivation globale.
    if (state) pcaEnabled = true;
#else
    // Sans broche par PCA, couper un banc seul est impossible : la demande porte donc sur
    // l'ensemble. Le documenter vaut mieux que l'ignorer silencieusement.
    (void)pcaIndex;
    enableServos(state);
#endif
}
