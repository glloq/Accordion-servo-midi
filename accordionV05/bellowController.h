#ifndef BELLOW_CONTROLLER_H
#define BELLOW_CONTROLLER_H

#include <Arduino.h>
#include "settings.h"
#include <FlexyStepper.h>
#include "servoController.h"

class BellowController {
public:
    // Constructeur prenant un ServoController en paramètre
    BellowController(ServoController &servoCtrl);

    void begin();         // Initialise le soufflet et le moteur pas à pas
    void update();        // Met à jour la position et vérifie les fins de course

    void updateSpeed(float totalAirFlow); // Ajuste la vitesse du soufflet en fonction du débit d'air
    void updateVolume(byte volume);       // Met à jour le volume pour moduler la vitesse du soufflet
    void calibrateZero();                 // Calibrage initial (homing) du soufflet
    void stopWithDecay();                 // Arrêt progressif

    void openValve();  // Ouvre la valve d'air
    void closeValve(); // Ferme la valve d'air

private:
    ServoController &servoController; // Référence vers le contrôleur des servos
    FlexyStepper stepper;             // Moteur pas à pas pour contrôler le soufflet

    bool valveOpen;        // Indique si la valve est ouverte
    bool movingDirection;  // true = ouverture, false = fermeture
    int16_t currentSpeed;  // Vitesse actuelle du soufflet
    uint32_t lastNoteTime; // Temps de la dernière note jouée
    byte volume;           // Volume actuel (0-127)

    void checkEndStops();  // Vérifie si le soufflet atteint une fin de course
};

#endif
