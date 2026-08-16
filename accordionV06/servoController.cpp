#include "servoController.h"

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

bool ServoController::begin() {
    // Configure le pin OE pour controler les sorties PWM des servos
    pinMode(PCA_OE_PIN, OUTPUT);
    digitalWrite(PCA_OE_PIN, HIGH);  // Sorties coupees pendant l'init
    pcaEnabled = false;

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
        pca[i].setPWMFreq(SERVO_PWM_FREQUENCY); // Frequence adaptee aux servos
    }

    lastCommandTime = millis();

    if (missingMask != 0) return false;

    enableServos(true); // Active les PCA au demarrage
    return true;
}

// Definit l'angle d'un servo, en reactivant les PCA si necessaire
void ServoController::setServoAngle(uint8_t pcaAddress, uint8_t channel, uint16_t angle) {
    if (!pcaEnabled) {
        enableServos(true); // Reactive les PCA s'ils avaient ete coupes
    }

    // Borne l'angle : une entree de mapping erronee ne doit pas produire un PWM hors plage.
    uint16_t safeAngle = constrain((int)angle, SERVO_MIN_ANGLE, SERVO_MAX_ANGLE);

    int pwmValue = map(safeAngle, SERVO_MIN_ANGLE, SERVO_MAX_ANGLE, SERVO_MIN_PWM, SERVO_MAX_PWM);
    for (int i = 0; i < NUM_PCA_TOTAL; i++) {
        if (PCA_TAB[i] == pcaAddress) {
            // setPWM() retourne le code d'erreur de l'ecriture I2C : un PCA qui cesse de
            // repondre en cours de jeu laisserait sinon des anches ouvertes sans que rien
            // ne le signale.
            uint8_t result = pca[i].setPWM(channel, 0, pwmValue);
            if (result != 0) {
                if (consecutiveErrors < 255) consecutiveErrors++;
            } else {
                consecutiveErrors = 0;
            }
            lastCommandTime = millis(); // Redemarre le compte a rebours de coupure de l'OE
            return;
        }
    }
}

// Active/Desactive tous les PCA et met a jour l'etat `pcaEnabled`
void ServoController::enableServos(bool state) {
    if (pcaEnabled == state) return; // Evite les changements inutiles

    digitalWrite(PCA_OE_PIN, state ? LOW : HIGH); // OE est actif bas
    pcaEnabled = state;
}
