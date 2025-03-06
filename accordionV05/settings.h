#ifndef SETTINGS_H
#define SETTINGS_H

#include <Arduino.h>  // Assure la prise en charge de `uint8_t`
//===========================================================================================================
// === CONFIGURATION GÉNÉRALE ===
//===========================================================================================================

// === CONFIGURATION DES CANAUX MIDI ===
#define MIDI_CHANNEL_LEFT 1  // Canal MIDI dédié à la main gauche
#define MIDI_CHANNEL_RIGHT 2 // Canal MIDI dédié à la main droite


#define BEBUG false

//===========================================================================================================
//==== Gestion du moteur pas a pas
//===========================================================================================================
// === CONFIGURATION DU SOUFFLET ===
// ➜ Définition des vitesses et accélérations minimales et maximales
//voici la vitesse minimale pour activer la note la plus grave coté clavier
// la vitesse normale permet de definir un deplacement minimum (voir pour unité ?? )
#define NORMAL_SPEED 10 // Vitesse minimale de déplacement avec une seule note active

// ➜ Plage de mouvement du soufflet (position en mm)
#define BELLOW_MIN_POSITION 0
#define BELLOW_MAX_POSITION 200 // ouverture maximum du soufflet (jusqu'au fin de course)
#define BELLOW_MARGIN 10  // Marge en mm pour éviter d’atteindre les extrêmes mécaniques (inverse le sens de rotaion si en dessous de la marge sans attendre de nouvelle noteOn) 

// === CONFIGURATION DES FINS DE COURSE ===
#define LIMIT_SWITCH_MIN_PIN 2 // Fin de course bas (fermeture complète)
#define LIMIT_SWITCH_MAX_PIN 3 // Fin de course haut (ouverture maximale)

// === CONFIGURATION MOTEURS
#define STEPPER_MIN_SPEED 2 
#define STEPPER_MAX_SPEED 300
#define STEPPER_MIN_ACCEL 5
#define STEPPER_MAX_ACCEL 100

// Broches utilisées pour le moteur pas à pas en Step/Dir 
#define STEPPER_DIR_PIN 9     // Direction
#define STEPPER_STEP_PIN 10   // Step
#define STEPPER_EN_PIN 11     // Enable (actif bas)

#define STEP_PER_MM 25 // nombre de pas par mm (pour une vis pas 16mm avec rapport de reduction 1/2 avec les poulies)
#define MICRO_STEP 16 // micro pas pour limiter le bruit du moteur 




//===========================================================================================================
// === Gestion des notes / servomoteurs 
//===========================================================================================================
// ➜ Définition des plages de notes pour chaque main
#define FIRST_NOTE_RIGHT 54  // Fa#3 en MIDI (Main Droite)
#define FIRST_NOTE_LEFT 36   // Do2 en MIDI (Main Gauche)
#define NUM_NOTES_RIGHT 34
#define NUM_NOTES_LEFT 24

// === CONFIGURATION DES SERVOMOTEURS ===
// ➜ Angles standardisés pour tous les servos (initilaisé a 90 +/- 40° en fonction du sens d'ouverture openDirection)
#define SERVO_CLOSED_ANGLE 130 // Angle de fermeture (valve fermée)
#define SERVO_CLOSED_ANGLE_MIRROR 50 // Angle de fermeture (valve fermée) 

#define SERVO_OPEN_ANGLE 40    // Angle d’ouverture (valve ouverte)

// === CONFIGURATION DE LA VALVE GÉNÉRALE ===
// ➜ Utilisée uniquement pour les déplacements sans notes actives (calibrage)
#define VALVE_PCA_ADDRESS PCA_TAB[3]  // Adresse du PCA dédié à la valve
#define VALVE_PCA_PIN 15              // Numéro du canal PCA => broche 16 du 4 eme PCA
#define VALVE_PCA_ANGLE_OPEN 70
#define VALVE_PCA_ANGLE_CLOSE 120

// === CONFIGURATION DES PCA9685 ===
// ➜ Adresses I2C des 4 PCA9685 utilisés
#define NUM_PCA_TOTAL 4
const uint8_t PCA_TAB[NUM_PCA_TOTAL] = {0x40, 0x41, 0x42, 0x43};
// ➜ Pins utilisé pour la broche OE des 4 PCA9685 utilisés (pour reduire le bruit des servomoteur)
const uint8_t PCA_OE_PIN[NUM_PCA_TOTAL] = {4, 5, 6, 7};//=> voir pour utiliser 1 pin  pour les 4 si trop complexe ?

// === STRUCTURE DE CONFIGURATION DES SERVOMOTEURS ===
struct ServoConfig {
    uint8_t pcaAddress;      // Adresse I2C du PCA9685
    uint8_t channel;         // Canal PCA (0-15)
    float airFlowMultiplier; // Facteur de débit d’air (influence le mouvement du soufflet)
    uint8_t closedPosition;  // Position fermée (angle en degrés)
    bool openDirection;      // Sens d’ouverture (true = normal, false = inversé)
};

// === MAPPING DES NOTES POUR LA MAIN DROITE ===
// ➜ Répartition sur les PCA pour optimiser l'activation des servos
const ServoConfig RIGHT_HAND_MAPPING[NUM_NOTES_RIGHT] = {
    // Premier bloc de 18 servos (PCA 0x40)
    { PCA_TAB[0], 0, 1.00, SERVO_CLOSED_ANGLE, true}, { PCA_TAB[0], 1, 0.98, SERVO_CLOSED_ANGLE, true},
    { PCA_TAB[0], 2, 0.96, SERVO_CLOSED_ANGLE, true}, { PCA_TAB[0], 3, 0.94, SERVO_CLOSED_ANGLE, true},
    { PCA_TAB[0], 4, 0.92, SERVO_CLOSED_ANGLE, true}, { PCA_TAB[0], 5, 0.90, SERVO_CLOSED_ANGLE, true},
    { PCA_TAB[0], 6, 0.88, SERVO_CLOSED_ANGLE, true}, { PCA_TAB[0], 7, 0.86, SERVO_CLOSED_ANGLE, true},
    { PCA_TAB[0], 8, 0.84, SERVO_CLOSED_ANGLE, true}, { PCA_TAB[0], 9, 0.82, SERVO_CLOSED_ANGLE, true},
    { PCA_TAB[0], 10, 0.80, SERVO_CLOSED_ANGLE, true}, { PCA_TAB[0], 11, 0.78, SERVO_CLOSED_ANGLE, true},
    { PCA_TAB[0], 12, 0.76, SERVO_CLOSED_ANGLE, true}, { PCA_TAB[0], 13, 0.74, SERVO_CLOSED_ANGLE, true},
    { PCA_TAB[0], 14, 0.72, SERVO_CLOSED_ANGLE, true}, { PCA_TAB[0], 15, 0.70, SERVO_CLOSED_ANGLE, true},
    { PCA_TAB[3], 13, 0.68, SERVO_CLOSED_ANGLE, true}, { PCA_TAB[3], 14, 0.66, SERVO_CLOSED_ANGLE, true},

    // Deuxième bloc de 16 servos (PCA 0x41)
    { PCA_TAB[1], 0, 0.66, SERVO_CLOSED_ANGLE_MIRROR, false}, { PCA_TAB[1], 1, 0.66, SERVO_CLOSED_ANGLE_MIRROR, false},
    { PCA_TAB[1], 2, 0.64, SERVO_CLOSED_ANGLE_MIRROR, false}, { PCA_TAB[1], 3, 0.62, SERVO_CLOSED_ANGLE_MIRROR, false},
    { PCA_TAB[1], 4, 0.60, SERVO_CLOSED_ANGLE_MIRROR, false}, { PCA_TAB[1], 5, 0.58, SERVO_CLOSED_ANGLE_MIRROR, false},
    { PCA_TAB[1], 6, 0.56, SERVO_CLOSED_ANGLE_MIRROR, false}, { PCA_TAB[1], 7, 0.54, SERVO_CLOSED_ANGLE_MIRROR, false},
    { PCA_TAB[1], 8, 0.52, SERVO_CLOSED_ANGLE_MIRROR, false}, { PCA_TAB[1], 9, 0.50, SERVO_CLOSED_ANGLE_MIRROR, false},
    { PCA_TAB[1], 10, 0.48, SERVO_CLOSED_ANGLE_MIRROR, false}, { PCA_TAB[1], 11, 0.46, SERVO_CLOSED_ANGLE_MIRROR, false},
    { PCA_TAB[1], 12, 0.44, SERVO_CLOSED_ANGLE_MIRROR, false}, { PCA_TAB[1], 13, 0.42, SERVO_CLOSED_ANGLE_MIRROR, false},
    { PCA_TAB[1], 14, 0.40, SERVO_CLOSED_ANGLE_MIRROR, false}, { PCA_TAB[1], 15, 0.38, SERVO_CLOSED_ANGLE_MIRROR, false}
};
// === MAPPING DES NOTES POUR LA MAIN GAUCHE ===
const ServoConfig LEFT_HAND_MAPPING[NUM_NOTES_LEFT] = {
      // ➜ premier bloc de 12  servos
    { PCA_TAB[2], 0, 1.0, SERVO_CLOSED_ANGLE, true}, { PCA_TAB[2], 1, 1.0, SERVO_CLOSED_ANGLE, true},
    { PCA_TAB[2], 2, 1.0, SERVO_CLOSED_ANGLE, true}, { PCA_TAB[2], 3, 1.0, SERVO_CLOSED_ANGLE, true},
    { PCA_TAB[2], 4, 1.0, SERVO_CLOSED_ANGLE, true}, { PCA_TAB[2], 5, 1.0, SERVO_CLOSED_ANGLE, true},
    { PCA_TAB[2], 6, 1.0, SERVO_CLOSED_ANGLE, true}, { PCA_TAB[2], 7, 1.0, SERVO_CLOSED_ANGLE, true},
    { PCA_TAB[2], 8, 1.0, SERVO_CLOSED_ANGLE, true}, { PCA_TAB[2], 9, 1.0, SERVO_CLOSED_ANGLE, true},
    { PCA_TAB[2], 10, 1.0, SERVO_CLOSED_ANGLE, true}, { PCA_TAB[2], 11, 1.0, SERVO_CLOSED_ANGLE, true},
      // ➜ second bloc de 12  servos
    { PCA_TAB[2], 12, 1.0, SERVO_CLOSED_ANGLE_MIRROR, false}, { PCA_TAB[2], 13, 1.0, SERVO_CLOSED_ANGLE_MIRROR, false},
    { PCA_TAB[2], 14, 1.0, SERVO_CLOSED_ANGLE_MIRROR, false}, { PCA_TAB[2], 15, 1.0, SERVO_CLOSED_ANGLE_MIRROR, false},
    { PCA_TAB[3], 1, 1.0, SERVO_CLOSED_ANGLE_MIRROR, false}, { PCA_TAB[3], 2, 1.0, SERVO_CLOSED_ANGLE_MIRROR, false},
    { PCA_TAB[3], 3, 1.0, SERVO_CLOSED_ANGLE_MIRROR, false}, { PCA_TAB[3], 4, 1.0, SERVO_CLOSED_ANGLE_MIRROR, false},
    { PCA_TAB[3], 5, 1.0, SERVO_CLOSED_ANGLE_MIRROR, false}, { PCA_TAB[3], 6, 1.0, SERVO_CLOSED_ANGLE_MIRROR, false},
    { PCA_TAB[3], 7, 1.0, SERVO_CLOSED_ANGLE_MIRROR, false}, { PCA_TAB[3], 8, 1.0, SERVO_CLOSED_ANGLE_MIRROR, false}
};

#endif
