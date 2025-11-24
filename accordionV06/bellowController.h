#ifndef BELLOW_CONTROLLER_H
#define BELLOW_CONTROLLER_H

#include <Arduino.h>
#include "settings.h"
#include <FlexyStepper.h>
#include "servoController.h"

// États de la machine à états pour le calibrage
enum CalibrationState {
    CALIB_IDLE,       // Pas de calibration en cours
    CALIB_MOVING,     // En train de chercher le fin de course
    CALIB_DONE        // Calibration terminée
};

class BellowController {
public:
    // Constructeur prenant un ServoController en paramètre
    BellowController(ServoController &servoCtrl);

    void begin();         // Initialise le soufflet et le moteur pas à pas
    void update();        // Met à jour la position et vérifie les fins de course

    void updateSpeed(float totalAirFlow); // Ajuste la vitesse du soufflet en fonction du débit d'air
    void updateVolume(byte volume);       // Met à jour le volume pour moduler la vitesse du soufflet
    void startCalibration();              // Démarre le calibrage (non-bloquant)
    void stopWithDecay();                 // Arrêt progressif

    void openValve();  // Ouvre la valve d'air
    void closeValve(); // Ferme la valve d'air

    bool isCalibrating() const { return calibState != CALIB_IDLE; }

private:
    ServoController &servoController; // Référence vers le contrôleur des servos
    FlexyStepper stepper;             // Moteur pas à pas pour contrôler le soufflet

    bool valveOpen;        // Indique si la valve est ouverte
    bool movingDirection;  // true = ouverture, false = fermeture
    int16_t currentSpeed;  // Vitesse actuelle du soufflet
    uint32_t lastNoteTime; // Temps de la dernière note jouée
    byte volume;           // Volume actuel (0-127)
    float lastTotalAirFlow; // Dernier débit d'air connu (pour updateVolume)

    // Machine à états pour calibration non-bloquante
    CalibrationState calibState;
    void updateCalibration();  // Mise à jour de la calibration

    // Debounce des fins de course
    uint32_t lastEndstopMinTime;
    uint32_t lastEndstopMaxTime;
    bool lastEndstopMinState;
    bool lastEndstopMaxState;

    void checkEndStops();  // Vérifie si le soufflet atteint un fin de course
};

#endif
