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

// Delai de debounce pour les fins de course (en millisecondes).
// ATTENTION : ce delai ne retarde JAMAIS l'arret. Le contact brut coupe immediatement le
// driver ; le debounce ne sert qu'a decider ensuite si le contact etait reel (recalage +
// inversion) ou parasite (reprise). A 25 mm/s, attendre 50 ms avant de couper aurait
// represente 1,25 mm de surcourse.
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
// FlexyStepper ne produit qu'UN pas par appel a processMovement() : la frequence reelle
// est donc bornee par la periode de la boucle principale, pas seulement par le CPU.
// D'ou l'ordonnancement retenu : un seul message MIDI par tour de boucle et plusieurs
// appels de service au generateur de pas (STEPPER_SERVICE_CALLS).
// Le test test_timing mesure la frequence reellement atteinte ; ajuster cette valeur en
// fonction de la mesure sur la machine reelle.
#define STEPPER_MAX_STEP_RATE_HZ 10000.0f

// Nombre d'appels a processMovement() par tour de boucle. processMovement() est
// auto-limite par micros() : les appels en trop sont quasi gratuits, mais ils permettent
// de rattraper les pas perdus pendant une rafale MIDI ou une ecriture I2C (~110 us).
#define STEPPER_SERVICE_CALLS 4

// Deceleration utilisee pour l'arret d'urgence sur fin de course (mm/s^2).
// Le driver etant deja coupe a ce moment-la, cette rampe ne produit aucun mouvement
// physique : elle sert uniquement a ramener rapidement l'etat interne de FlexyStepper a
// l'arret, seule condition ou setCurrentPosition() est legitime.
#define STEPPER_EMERGENCY_DECEL 5000.0f

// Duree max de la sequence d'arret avant declaration de defaut (ms)
#define STEPPER_STOP_TIMEOUT_MS 1000UL

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

// Compromis microstepping / vitesse : la vitesse maximale utile vaut
// STEPPER_MAX_STEP_RATE_HZ / STEPS_PER_MM, soit ~25 mm/s a 400 pas/mm. Si le soufflet
// doit aller plus vite (beaucoup de notes simultanees), il faut REDUIRE MICRO_STEP
// (1/8 -> 50 mm/s, 1/4 -> 100 mm/s) et non augmenter STEPPER_MAX_SPEED, qui est de toute
// facon borne a l'execution par BellowController::begin().
//-----------------------------------------------------------------------------------------

// === CALIBRATION (HOMING) ===
// Homing en deux passes, comme sur une machine-outil : approche rapide, degagement, puis
// reapproche lente. Le premier contact sert seulement a localiser grossierement la butee ;
// la surcourse eventuelle (driver coupe en urgence) est donc sans effet sur le zero final,
// qui est etabli par la seconde approche, lente.
#define HOMING_SPEED 10.0f            // Approche rapide (mm/s)
#define HOMING_SLOW_SPEED 1.5f        // Reapproche lente, fixe le zero (mm/s)
#define HOMING_BACKOFF_MM 4.0f        // Degagement entre les deux approches (mm)
#define HOMING_MAX_DISTANCE 250.0f    // Course max de l'approche rapide (mm)
#define HOMING_TIMEOUT_MS 60000UL     // Duree max de TOUT le homing, deux passes comprises

//===========================================================================================================
// === Gestion des notes / servomoteurs
//===========================================================================================================
// La table des notes (NoteConfig, RIGHT_HAND_MAPPING, LEFT_HAND_MAPPING), les adresses des
// PCA9685 et les angles servos sont definis dans noteMapping.h.

// === REPONSE A LA VELOCITE ===
// La velocite n'agit pas sur les servos (une valve est ouverte ou fermee) mais sur la
// demande d'air. Elle est appliquee PAR NOTE : chaque note ouverte pondere son propre
// debit. Une note jouee doucement au milieu d'un accord fort ne fait donc plus chuter le
// debit de tout l'accord, et inversement.
//   debit_total = somme( airFlowMultiplier_i * poidsTenu(velocity_i) )
//   poidsTenu(v) = VELOCITY_SUSTAIN_MIN + (1 - VELOCITY_SUSTAIN_MIN) * v/127
// L'attaque est un supplement temporaire applique a la SEULE derniere note declenchee :
//   bonus = airFlow_derniere * poidsTenu(v) * VELOCITY_ATTACK_BOOST * v/127
#define VELOCITY_SUSTAIN_MIN 0.60f  // Facteur de debit pour velocity = 1
#define VELOCITY_ATTACK_BOOST 0.50f // Surcroit de debit pendant l'attaque (a velocity = 127)
#define VELOCITY_ATTACK_MS 120UL    // Duree de la phase d'attaque

// Poids du niveau tenu pour une velocite donnee.
inline float velocitySustainWeight(uint8_t velocity) {
    return VELOCITY_SUSTAIN_MIN + (1.0f - VELOCITY_SUSTAIN_MIN) * ((float)velocity / 127.0f);
}

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
// Nombre d'erreurs I2C consecutives tolerees avant passage en defaut.
// Un PCA9685 qui cesse de repondre en cours de jeu laisse des anches ouvertes.
#define SERVO_I2C_ERROR_LIMIT 8

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
