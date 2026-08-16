#ifndef SETTINGS_H
#define SETTINGS_H

#include <Arduino.h>   // Assure la prise en charge de `uint8_t`
#include "noteMapping.h" // Table des notes, adresses PCA et angles servos

//===========================================================================================================
// === CONFIGURATION GENERALE ===
//===========================================================================================================

// === CONFIGURATION DES CANAUX MIDI ===
#define MIDI_CHANNEL_LEFT 1  // Canal MIDI dedie a la main gauche
#define MIDI_CHANNEL_RIGHT 2 // Canal MIDI dedie a la main droite

// === TRANSPORTS MIDI ===
// Definissables depuis la ligne de compilation (-DMIDI_TRANSPORT_USB=1).
// DIN  : MIDI serie classique sur Serial1 (broches 0/1 du Leonardo).
// USB  : MIDI USB natif du Leonardo/Micro, necessite la bibliotheque "USB-MIDI" (lathoub).
//        Desactive par defaut pour ne pas imposer la dependance a l'IDE Arduino.
#ifndef MIDI_TRANSPORT_DIN
#define MIDI_TRANSPORT_DIN 1
#endif
#ifndef MIDI_TRANSPORT_USB
#define MIDI_TRANSPORT_USB 0
#endif

#define DEBUG false

// Frequence du bus I2C des PCA9685. Le PCA9685 supporte 1 MHz ; 400 kHz raccourcit
// nettement le temps de commande des 59 servos. Repasser a 100000 en cas de bus long
// ou d'erreurs de transmission.
#define I2C_CLOCK_HZ 400000L

// Delai de debounce pour les fins de course (en millisecondes)
#define ENDSTOP_DEBOUNCE_MS 50

// Nombre maximum de notes simultanees (protection alimentation 5V/10A)
#define MAX_SIMULTANEOUS_NOTES 15

//===========================================================================================================
//==== Gestion du moteur pas a pas
//===========================================================================================================
// === CONFIGURATION DU SOUFFLET ===
// Vitesse de reference pour une seule note active (airFlowMultiplier = 1.0) en mm/s.
#define NORMAL_SPEED 10

// Plage de mouvement du soufflet (position en mm)
#define BELLOW_MIN_POSITION 0
#define BELLOW_MAX_POSITION 200 // ouverture maximum du soufflet en mm (jusqu'au fin de course)

// === SEUILS DE SECURITE DU SOUFFLET ===
// Inversion du sens AVANT d'atteindre les fins de course. Ces seuils sont evalues en
// continu dans BellowController::update(), pas uniquement sur evenement MIDI.
#define BELLOW_REVERSE_THRESHOLD_OPEN 0.7f  // Inversion a 70% d'ouverture
#define BELLOW_REVERSE_THRESHOLD_CLOSE 0.3f // Inversion a 30% de fermeture

// === CONFIGURATION DES FINS DE COURSE ===
// ATTENTION : sur Arduino Leonardo / Micro, D2 et D3 sont SDA et SCL (bus I2C des PCA9685).
// Les fins de course ne doivent donc JAMAIS utiliser ces broches.
#define LIMIT_SWITCH_MIN_PIN 5 // Fin de course bas (fermeture complete)
#define LIMIT_SWITCH_MAX_PIN 6 // Fin de course haut (ouverture maximale)

// === CONFIGURATION MOTEURS ===
// Vitesses minimale et maximale (en mm/s)
#define STEPPER_MIN_SPEED 2
#define STEPPER_MAX_SPEED 300
// Accelerations minimale et maximale (en mm/s^2)
#define STEPPER_MIN_ACCEL 5
#define STEPPER_MAX_ACCEL 100

// Frequence de pas maximale reellement tenable par le MCU avec FlexyStepper.
// Sur AVR 16 MHz, au-dela de ~10 kHz la generation de pas decroche.
// BellowController borne la vitesse a STEPPER_MAX_STEP_RATE_HZ / STEPS_PER_MM.
#define STEPPER_MAX_STEP_RATE_HZ 10000.0f

// Temps avant de desactiver le moteur et fermer la valve du soufflet (en millisecondes)
#define BELLOW_INACTIVITY_TIMEOUT 60000UL // 1 minute

// Broches utilisees pour le moteur pas a pas en Step/Dir
#define STEPPER_DIR_PIN 9     // Direction
#define STEPPER_STEP_PIN 10   // Step
#define STEPPER_EN_PIN 11     // Enable (actif bas)

//-----------------------------------------------------------------------------------------
// === PARAMETRES MECANIQUES DE LA TRANSMISSION ===
// STEPS_PER_MM est calcule, plus code en dur : une erreur ici se traduit directement par
// une erreur de course d'un facteur MICRO_STEP (jusqu'a x16).
//
// !! MICRO_STEP doit correspondre au reglage PHYSIQUE du TMC2209 (cavaliers MS1/MS2,
// ou configuration UART). En mode standalone, MS1=MS2=LOW donne 1/8 sur la plupart des
// modules TMC2209, PAS 1/16. Verifier la doc du module avant le premier essai mecanique.
#define MOTOR_STEPS_PER_REV 200  // Moteur NEMA17 1.8 deg => 200 pas/tour
#define MICRO_STEP 16            // Micro-pas du driver (doit refleter le cablage MS1/MS2)
#define SCREW_LEAD_MM 16.0f      // Pas de la tige filetee : avance en mm par tour de vis
#define GEAR_RATIO 2.0f          // Reduction poulies 1/2 : 2 tours moteur = 1 tour de vis

// Pas par millimetre reellement envoyes au driver.
// Ex. 200 * 16 * 2 / 16 = 400 pas/mm.
#define STEPS_PER_MM (((float)MOTOR_STEPS_PER_REV * (float)MICRO_STEP * GEAR_RATIO) / SCREW_LEAD_MM)
//-----------------------------------------------------------------------------------------

// === CALIBRATION (HOMING) ===
#define HOMING_SPEED 10.0f            // Vitesse de recherche du zero en mm/s
#define HOMING_MAX_DISTANCE 250.0f    // Course max autorisee avant declaration de defaut (mm)
#define HOMING_TIMEOUT_MS 45000UL     // Duree max du homing (doit couvrir HOMING_MAX_DISTANCE / HOMING_SPEED)

//===========================================================================================================
// === Gestion des notes / servomoteurs
//===========================================================================================================
// La table des notes (NoteConfig, RIGHT_HAND_MAPPING, LEFT_HAND_MAPPING), les adresses des
// PCA9685 et les angles servos sont definis dans noteMapping.h.

// === REPONSE A LA VELOCITE ===
// La velocite n'agit pas sur les servos (une valve est ouverte ou fermee) mais sur la
// demande d'air : attaque breve plus franche, puis niveau tenu proportionnel.
//   facteurTenu   = VELOCITY_SUSTAIN_MIN + (1 - VELOCITY_SUSTAIN_MIN) * velocity/127
//   facteurAttaque = facteurTenu * (1 + VELOCITY_ATTACK_BOOST * velocity/127)
#define VELOCITY_SUSTAIN_MIN 0.60f  // Facteur de debit pour velocity = 1
#define VELOCITY_ATTACK_BOOST 0.50f // Surcroit de debit pendant l'attaque (a velocity = 127)
#define VELOCITY_ATTACK_MS 120UL    // Duree de la phase d'attaque

// === CONFIGURATION DES SERVOMOTEURS ===
// Frequence des servomoteurs (50Hz recommande pour la plupart des servos)
#define SERVO_PWM_FREQUENCY 50
// Delai avant desactivation des PCA apres la DERNIERE commande servo (en millisecondes).
// Compte a partir du dernier setServoAngle() et non de la derniere note : cela garantit
// qu'un servo (notamment la valve generale) a le temps d'atteindre sa position avant que
// l'OE ne coupe le signal PWM.
#define PCA_DISABLE_DELAY 500
// Espacement entre servos lors de la fermeture initiale, pour eviter un appel de courant
// simultane des 59 servos au demarrage (en millisecondes).
#define SERVO_INIT_STAGGER_MS 15

// Valeurs standard pour les positions des servos
#define SERVO_MIN_ANGLE 0    // Angle minimum du servo
#define SERVO_MAX_ANGLE 180  // Angle maximum du servo

// Plage PWM correspondant aux angles (adapte aux servos 50Hz)
#define SERVO_MIN_PWM 150   // Correspond a ~0 deg (PWM bas)
#define SERVO_MAX_PWM 600   // Correspond a ~180 deg (PWM haut)

// === CONFIGURATION DE LA VALVE GENERALE ===
// VALVE_PCA_ADDRESS / VALVE_PCA_PIN / VALVE_PCA_ANGLE_* sont definis dans noteMapping.h,
// avec l'allocation des canaux PCA.

// === BROCHE OE DES PCA9685 ===
// Une seule broche pilote l'OE des 4 PCA9685 (simplification du cablage).
// ATTENTION : OE ne coupe QUE les sorties PWM du PCA9685. Le rail +5V des 59 servos reste
// alimente. Pour une vraie mise hors tension il faudrait un load switch / MOSFET high-side
// en amont de chaque banc de servos.
// Consequence : la valve generale partage cet OE avec toutes les notes. Instrument ne coupe
// l'OE qu'a l'etat READY/FAULT et seulement PCA_DISABLE_DELAY apres la derniere commande.
#define PCA_OE_PIN 4

#endif
