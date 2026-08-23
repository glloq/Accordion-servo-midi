#ifndef SETTINGS_H
#define SETTINGS_H

// =========================================================================================
//  VALEURS DERIVEES ET VERIFICATIONS
// -----------------------------------------------------------------------------------------
//  Ce fichier ne contient AUCUN choix : tous les reglages sont dans config.h. On trouve ici
//  ce qui en decoule (pas par millimetre, bornes reellement tenables, niveaux logiques) et
//  les verifications a la compilation qui attrapent les configurations impossibles avant
//  qu'elles n'abiment la mecanique.
//
//  Pour changer la machine : editer config.h, ou mieux, la regenerer depuis
//  tools/configurator/index.html.
// =========================================================================================

#include <Arduino.h>     // Assure la prise en charge de `uint8_t`
#include "config.h"      // Les reglages de la machine
#include "noteMapping.h" // Table des notes, adresses PCA, angles
#include "staticAssert.h"

//===========================================================================================
// 1. VERIFICATIONS GENERALES
//===========================================================================================

#if !MIDI_TRANSPORT_DIN && !MIDI_TRANSPORT_USB
#error "Aucun transport MIDI actif : definir MIDI_TRANSPORT_DIN et/ou MIDI_TRANSPORT_USB."
#endif

#if MAX_SIMULTANEOUS_NOTES < 1
#error "MAX_SIMULTANEOUS_NOTES doit valoir au moins 1."
#endif

#if (NUM_NOTES_RIGHT + NUM_NOTES_LEFT) < 1
#error "L'instrument n'a aucune note : renseigner RIGHT_HAND_NOTE_LIST et/ou LEFT_HAND_NOTE_LIST."
#endif

#if NUM_PCA_TOTAL < 1
#error "Au moins un PCA9685 est necessaire pour piloter les actionneurs."
#endif
// ServoController suit l'etat des sorties dans un masque de 16 bits, un par PCA.
#if NUM_PCA_TOTAL > 16
#error "Plus de 16 PCA9685 : le masque d'etat des sorties ne suffit plus."
#endif

#if SERVO_MIN_PWM >= SERVO_MAX_PWM
#error "SERVO_MIN_PWM doit etre strictement inferieur a SERVO_MAX_PWM."
#endif

#if SERVO_MIN_ANGLE >= SERVO_MAX_ANGLE
#error "SERVO_MIN_ANGLE doit etre strictement inferieur a SERVO_MAX_ANGLE."
#endif

#if AIR_SOURCE < AIR_SOURCE_BELLOW_STEPPER || AIR_SOURCE > AIR_SOURCE_PUMP_ONOFF
#error "AIR_SOURCE ne designe aucune source d'air connue (voir airSourceTypes.h)."
#endif

// Broches du bus I2C. Sur Leonardo / Micro ce sont D2 (SDA) et D3 (SCL) ; un autre MCU peut
// les avoir ailleurs, d'ou la possibilite de les redefinir dans config.h.
#ifndef I2C_SDA_PIN
#define I2C_SDA_PIN 2
#endif
#ifndef I2C_SCL_PIN
#define I2C_SCL_PIN 3
#endif

// Toute broche partagee avec le bus I2C fait tomber les PCA9685 des qu'elle est utilisee.
#define ACCORDION_PIN_IS_I2C(p) ((p) == I2C_SDA_PIN || (p) == I2C_SCL_PIN)

//===========================================================================================
// 2. ACTIONNEURS ET NOTES
//===========================================================================================

// Taille des tableaux d'etat d'une main : la plus grande des deux mains suffit.
#if NUM_NOTES_RIGHT >= NUM_NOTES_LEFT
  #define MAX_NOTES_PER_HAND NOTE_TABLE_SIZE(NUM_NOTES_RIGHT)
#else
  #define MAX_NOTES_PER_HAND NOTE_TABLE_SIZE(NUM_NOTES_LEFT)
#endif

// Le vol de voix et le suivi d'anciennete manipulent des index sur un octet.
#if MAX_NOTES_PER_HAND > 127
#error "Plus de 127 notes par main : les index de note ne tiennent plus sur un int8_t."
#endif

#if NOTE_ACTUATOR == ACTUATOR_SOLENOID
  #if SOLENOID_HOLD_PERCENT < 1 || SOLENOID_HOLD_PERCENT > 100
  #error "SOLENOID_HOLD_PERCENT doit etre compris entre 1 et 100."
  #endif
  // Comptes PCA9685 correspondant au courant de maintien (0-4095).
  #define SOLENOID_HOLD_COUNTS ((uint16_t)((4095L * SOLENOID_HOLD_PERCENT) / 100))
#endif

// --- OE des PCA9685 ---
#if PCA_OE_MODE == PCA_OE_SHARED
  #if ACCORDION_PIN_IS_I2C(PCA_OE_PIN)
  #error "PCA_OE_PIN est cablee sur le bus I2C : les PCA9685 ne repondront plus."
  #endif
#endif

//===========================================================================================
// 3. VALVE GENERALE
//===========================================================================================

#if AIR_VALVE_TYPE == AIR_VALVE_SERVO
  #if VALVE_PCA_INDEX >= NUM_PCA_TOTAL
  #error "VALVE_PCA_INDEX depasse le nombre de PCA9685 declares."
  #endif
  #if VALVE_PCA_PIN > 15
  #error "VALVE_PCA_PIN doit etre un canal PCA9685 valide (0-15)."
  #endif
#elif AIR_VALVE_TYPE == AIR_VALVE_SOLENOID
  #if ACCORDION_PIN_IS_I2C(VALVE_SOLENOID_PIN)
  #error "VALVE_SOLENOID_PIN est cablee sur le bus I2C."
  #endif
#endif

//===========================================================================================
// 4. SOURCE D'AIR : VALEURS DERIVEES ET VERIFICATIONS
//===========================================================================================

// Les deux familles de soufflets partagent la logique d'oscillation : la pression se cree
// en ouvrant ET en fermant, avec inversion avant les butees. Les turbines et pompes, elles,
// sont unidirectionnelles.
#if AIR_SOURCE == AIR_SOURCE_BELLOW_STEPPER || AIR_SOURCE == AIR_SOURCE_BELLOW_SERVO
  #define AIR_SOURCE_IS_BELLOW 1
#else
  #define AIR_SOURCE_IS_BELLOW 0
#endif

// Seule la source pas a pas possede une course mesuree, des fins de course et un homing.
#if AIR_SOURCE == AIR_SOURCE_BELLOW_STEPPER
  #define AIR_SOURCE_HAS_HOMING 1
#else
  #define AIR_SOURCE_HAS_HOMING 0
#endif

//-------------------------------------------------------------------------------------------
#if AIR_SOURCE == AIR_SOURCE_BELLOW_STEPPER
//-------------------------------------------------------------------------------------------

// --- Pas par millimetre, CALCULE et non code en dur --------------------------------------
// Une erreur ici se traduit directement par une erreur de course d'un facteur MICRO_STEP
// (jusqu'a x16). D'ou le calcul a partir des grandeurs physiques mesurables.
#if BELLOW_TRANSMISSION == TRANSMISSION_SCREW
  #define TRAVEL_PER_OUTPUT_REV_MM (SCREW_LEAD_MM)
#elif BELLOW_TRANSMISSION == TRANSMISSION_BELT
  #define TRAVEL_PER_OUTPUT_REV_MM ((float)BELT_PITCH_MM * (float)BELT_PULLEY_TEETH)
#else
  #error "BELLOW_TRANSMISSION inconnue (voir TRANSMISSION_* dans airSourceTypes.h)."
#endif

// Ex. 200 pas * 16 micro-pas * 2 (reduction) / 16 mm = 400 pas/mm.
#define STEPS_PER_MM (((float)MOTOR_STEPS_PER_REV * (float)MICRO_STEP * (float)GEAR_RATIO) \
                      / (float)TRAVEL_PER_OUTPUT_REV_MM)

// Vitesse maximale reellement atteignable, compte tenu du debit de pas. Au-dela, le moteur
// decroche silencieusement. Pour aller plus vite il faut REDUIRE MICRO_STEP (1/8 -> x2,
// 1/4 -> x4) et non augmenter STEPPER_MAX_SPEED.
#define STEPPER_RATE_LIMITED_SPEED (STEPPER_MAX_STEP_RATE_HZ / STEPS_PER_MM)

#if BELLOW_MAX_POSITION <= BELLOW_MIN_POSITION
#error "BELLOW_MAX_POSITION doit etre strictement superieure a BELLOW_MIN_POSITION."
#endif
#if STEPPER_MIN_SPEED >= STEPPER_MAX_SPEED
#error "STEPPER_MIN_SPEED doit etre strictement inferieure a STEPPER_MAX_SPEED."
#endif
#if ACCORDION_PIN_IS_I2C(LIMIT_SWITCH_MIN_PIN) || ACCORDION_PIN_IS_I2C(LIMIT_SWITCH_MAX_PIN)
#error "Un fin de course est cable sur SDA/SCL : le bus des PCA9685 serait perdu."
#endif
#if LIMIT_SWITCH_MIN_PIN == LIMIT_SWITCH_MAX_PIN
#error "Les deux fins de course partagent la meme broche."
#endif
#if ACCORDION_PIN_IS_I2C(STEPPER_STEP_PIN) || ACCORDION_PIN_IS_I2C(STEPPER_DIR_PIN) || \
    ACCORDION_PIN_IS_I2C(STEPPER_EN_PIN)
#error "Une broche du driver pas a pas est cablee sur SDA/SCL."
#endif
#if STEPPER_STEP_PIN == STEPPER_DIR_PIN || STEPPER_STEP_PIN == STEPPER_EN_PIN || \
    STEPPER_DIR_PIN == STEPPER_EN_PIN
#error "Deux broches du driver pas a pas partagent le meme numero."
#endif
// Comparaisons sur des flottants : le preprocesseur ne sait pas les evaluer, ces
// verifications passent donc par static_assert (voir staticAssert.h).
ACCORDION_STATIC_ASSERT(HOMING_BACKOFF_MM > 0.0f, HOMING_BACKOFF_MM_doit_etre_positif);
ACCORDION_STATIC_ASSERT(HOMING_SLOW_SPEED > 0.0f, HOMING_SLOW_SPEED_doit_etre_positif);
ACCORDION_STATIC_ASSERT(HOMING_SPEED > 0.0f, HOMING_SPEED_doit_etre_positif);
ACCORDION_STATIC_ASSERT(HOMING_MAX_DISTANCE > HOMING_BACKOFF_MM,
                        HOMING_MAX_DISTANCE_trop_courte);
ACCORDION_STATIC_ASSERT(BELLOW_REVERSE_THRESHOLD_CLOSE > 0.0f &&
                        BELLOW_REVERSE_THRESHOLD_CLOSE < BELLOW_REVERSE_THRESHOLD_OPEN &&
                        BELLOW_REVERSE_THRESHOLD_OPEN < 1.0f,
                        Seuils_d_inversion_du_soufflet_incoherents);
ACCORDION_STATIC_ASSERT(STEPS_PER_MM > 0.0f, STEPS_PER_MM_doit_etre_positif);

// Inversion logicielle du sens. FlexyStepper ne l'expose pas : le signe est donc applique
// a la frontiere de la bibliotheque, sur les positions et les cibles. Le firmware, lui,
// continue de raisonner dans le repere de la machine (0 = soufflet ferme).
#if STEPPER_INVERT_DIR
  #define STEPPER_DIR_SIGN (-1.0f)
#else
  #define STEPPER_DIR_SIGN (1.0f)
#endif

// Niveaux logiques, deduits du cablage declare.
#if STEPPER_EN_ACTIVE_LOW
  #define STEPPER_EN_ON  LOW
  #define STEPPER_EN_OFF HIGH
#else
  #define STEPPER_EN_ON  HIGH
  #define STEPPER_EN_OFF LOW
#endif

#endif // AIR_SOURCE_BELLOW_STEPPER

//-------------------------------------------------------------------------------------------
#if AIR_SOURCE == AIR_SOURCE_BELLOW_SERVO
//-------------------------------------------------------------------------------------------
#if BELLOW_SERVO_PCA_INDEX >= NUM_PCA_TOTAL
#error "BELLOW_SERVO_PCA_INDEX depasse le nombre de PCA9685 declares."
#endif
#if BELLOW_SERVO_PCA_PIN > 15
#error "BELLOW_SERVO_PCA_PIN doit etre un canal PCA9685 valide (0-15)."
#endif
#if BELLOW_SERVO_ANGLE_CLOSED == BELLOW_SERVO_ANGLE_OPEN
#error "Le soufflet a servo n'a aucune course : angles ouvert et ferme identiques."
#endif
ACCORDION_STATIC_ASSERT(BELLOW_SERVO_MIN_SPEED_DPS > 0.0f &&
                        BELLOW_SERVO_MIN_SPEED_DPS <= BELLOW_SERVO_MAX_SPEED_DPS,
                        Vitesses_de_balayage_du_soufflet_a_servo_incoherentes);
ACCORDION_STATIC_ASSERT(BELLOW_SERVO_DEMAND_FULL_SCALE > 1.0f,
                        BELLOW_SERVO_DEMAND_FULL_SCALE_doit_depasser_1);
ACCORDION_STATIC_ASSERT(BELLOW_SERVO_REVERSE_CLOSE >= 0.0f &&
                        BELLOW_SERVO_REVERSE_CLOSE < BELLOW_SERVO_REVERSE_OPEN &&
                        BELLOW_SERVO_REVERSE_OPEN <= 1.0f,
                        Seuils_d_inversion_du_soufflet_a_servo_incoherents);
#endif // AIR_SOURCE_BELLOW_SERVO

//-------------------------------------------------------------------------------------------
#if AIR_SOURCE == AIR_SOURCE_BLOWER_PWM
//-------------------------------------------------------------------------------------------
#if ACCORDION_PIN_IS_I2C(BLOWER_PWM_PIN)
#error "BLOWER_PWM_PIN est cablee sur le bus I2C."
#endif
#if BLOWER_DUTY_MIN >= BLOWER_DUTY_MAX
#error "BLOWER_DUTY_MIN doit etre strictement inferieur a BLOWER_DUTY_MAX."
#endif
#if BLOWER_DUTY_MAX > 255
#error "BLOWER_DUTY_MAX depasse la resolution d'analogWrite (0-255)."
#endif
ACCORDION_STATIC_ASSERT(BLOWER_DEMAND_FULL_SCALE > 0.0f,
                        BLOWER_DEMAND_FULL_SCALE_doit_etre_positif);
#endif // AIR_SOURCE_BLOWER_PWM

//-------------------------------------------------------------------------------------------
#if AIR_SOURCE == AIR_SOURCE_BLOWER_ESC
//-------------------------------------------------------------------------------------------
#if ESC_PCA_INDEX >= NUM_PCA_TOTAL
#error "ESC_PCA_INDEX depasse le nombre de PCA9685 declares."
#endif
#if ESC_PCA_PIN > 15
#error "ESC_PCA_PIN doit etre un canal PCA9685 valide (0-15)."
#endif
#if ESC_PULSE_MIN_US >= ESC_PULSE_MAX_US
#error "ESC_PULSE_MIN_US doit etre strictement inferieure a ESC_PULSE_MAX_US."
#endif
#if ESC_PULSE_IDLE_US < ESC_PULSE_MIN_US || ESC_PULSE_IDLE_US > ESC_PULSE_MAX_US
#error "ESC_PULSE_IDLE_US doit rester entre ESC_PULSE_MIN_US et ESC_PULSE_MAX_US."
#endif
ACCORDION_STATIC_ASSERT(ESC_DEMAND_FULL_SCALE > 0.0f, ESC_DEMAND_FULL_SCALE_doit_etre_positif);
// Une impulsion de 2 ms ne tient pas dans une periode de 5 ms si la frequence PCA est trop
// haute ; a 50 Hz la periode vaut 20 ms, ce qui convient a tous les ESC courants.
#if SERVO_PWM_FREQUENCY > 400
#error "SERVO_PWM_FREQUENCY est trop elevee pour un ESC : rester a 50 Hz (ou 400 Hz maximum)."
#endif
#endif // AIR_SOURCE_BLOWER_ESC

//-------------------------------------------------------------------------------------------
#if AIR_SOURCE == AIR_SOURCE_PUMP_ONOFF
//-------------------------------------------------------------------------------------------
#if ACCORDION_PIN_IS_I2C(PUMP_PIN)
#error "PUMP_PIN est cablee sur le bus I2C."
#endif
#if PUMP_MIN_DUTY_PERCENT < 1 || PUMP_MAX_DUTY_PERCENT > 100
#error "Les rapports cycliques de la pompe doivent rester entre 1 et 100 %."
#endif
#if PUMP_MIN_DUTY_PERCENT > PUMP_MAX_DUTY_PERCENT
#error "PUMP_MIN_DUTY_PERCENT depasse PUMP_MAX_DUTY_PERCENT."
#endif
#if PUMP_CYCLE_MS < 20
#error "PUMP_CYCLE_MS trop court : une pompe tout-ou-rien ne suit pas une modulation rapide."
#endif
ACCORDION_STATIC_ASSERT(PUMP_DEMAND_FULL_SCALE > 0.0f, PUMP_DEMAND_FULL_SCALE_doit_etre_positif);
#endif // AIR_SOURCE_PUMP_ONOFF

//===========================================================================================
// 5. REGULATION DE PRESSION
//===========================================================================================

#if PRESSURE_SENSOR_ENABLED
  ACCORDION_STATIC_ASSERT(PRESSURE_ADC_PER_KPA > 0.0f,
                          PRESSURE_ADC_PER_KPA_doit_etre_positif);
  ACCORDION_STATIC_ASSERT(PRESSURE_MAX_KPA > PRESSURE_TARGET_KPA,
                          PRESSURE_MAX_KPA_doit_depasser_PRESSURE_TARGET_KPA);
  ACCORDION_STATIC_ASSERT(PRESSURE_FILTER_ALPHA > 0.0f && PRESSURE_FILTER_ALPHA <= 1.0f,
                          PRESSURE_FILTER_ALPHA_hors_de_l_intervalle_0_1);
  ACCORDION_STATIC_ASSERT(PRESSURE_SCALE_MIN > 0.0f && PRESSURE_SCALE_MIN < 1.0f &&
                          PRESSURE_SCALE_MAX > 1.0f,
                          Bornes_du_facteur_correctif_de_pression_incoherentes);
  #if PRESSURE_SAMPLE_MS < 1
  #error "PRESSURE_SAMPLE_MS doit valoir au moins 1 ms."
  #endif
  // La pompe tout-ou-rien n'a pas d'autre reglage continu : sans capteur elle module son
  // rapport cyclique, avec capteur elle regule par hysteresis.
  #define PRESSURE_CLOSED_LOOP (PRESSURE_CONTROL != PRESSURE_CONTROL_OPEN_LOOP)
#else
  #define PRESSURE_CLOSED_LOOP 0
#endif

//===========================================================================================
// 6. CONFLITS DE BROCHES ENTRE MODULES
//===========================================================================================
// Chaque module verifie deja ses propres broches ; ce qui manque est la verification
// CROISEE, celle qu'un fichier ecrit a la main rate systematiquement : la broche OE des PCA
// reutilisee pour la pompe, l'electrovanne posee sur ENABLE du driver... Ces erreurs ne se
// voient pas a la compilation et se paient sur la mecanique.
//
// Seules les broches de la configuration REELLEMENT compilee sont confrontees : les
// parametres des sources d'air inutilisees n'ont pas a etre coherents.

// Broche de la source d'air ayant besoin d'etre distincte du reste (0 = aucune).
#if AIR_SOURCE == AIR_SOURCE_BLOWER_PWM
  #define AIR_SOURCE_POWER_PIN BLOWER_PWM_PIN
#elif AIR_SOURCE == AIR_SOURCE_PUMP_ONOFF
  #define AIR_SOURCE_POWER_PIN PUMP_PIN
#endif

#if PCA_OE_MODE == PCA_OE_SHARED
  #if AIR_SOURCE == AIR_SOURCE_BELLOW_STEPPER
    #if PCA_OE_PIN == STEPPER_STEP_PIN || PCA_OE_PIN == STEPPER_DIR_PIN || \
        PCA_OE_PIN == STEPPER_EN_PIN || PCA_OE_PIN == LIMIT_SWITCH_MIN_PIN || \
        PCA_OE_PIN == LIMIT_SWITCH_MAX_PIN
    #error "PCA_OE_PIN entre en conflit avec une broche du soufflet pas a pas."
    #endif
  #endif
  #ifdef AIR_SOURCE_POWER_PIN
    #if PCA_OE_PIN == AIR_SOURCE_POWER_PIN
    #error "PCA_OE_PIN entre en conflit avec la broche de puissance de la source d'air."
    #endif
  #endif
  #if AIR_VALVE_TYPE == AIR_VALVE_SOLENOID && PCA_OE_PIN == VALVE_SOLENOID_PIN
  #error "PCA_OE_PIN entre en conflit avec l'electrovanne de mise a l'air libre."
  #endif
#endif

#if AIR_VALVE_TYPE == AIR_VALVE_SOLENOID
  #if AIR_SOURCE == AIR_SOURCE_BELLOW_STEPPER
    #if VALVE_SOLENOID_PIN == STEPPER_STEP_PIN || VALVE_SOLENOID_PIN == STEPPER_DIR_PIN || \
        VALVE_SOLENOID_PIN == STEPPER_EN_PIN || VALVE_SOLENOID_PIN == LIMIT_SWITCH_MIN_PIN || \
        VALVE_SOLENOID_PIN == LIMIT_SWITCH_MAX_PIN
    #error "VALVE_SOLENOID_PIN entre en conflit avec une broche du soufflet pas a pas."
    #endif
  #endif
  #ifdef AIR_SOURCE_POWER_PIN
    #if VALVE_SOLENOID_PIN == AIR_SOURCE_POWER_PIN
    #error "VALVE_SOLENOID_PIN entre en conflit avec la broche de puissance de la source d'air."
    #endif
  #endif
#endif

// Une valve generale portee par un PCA doit occuper un canal libre. Le canal de l'ESC et
// celui du servo de soufflet aussi : test_note_mapping verifie qu'aucune note ne les
// reclame, mais rien n'empeche deux organes de se poser sur le meme canal.
#if AIR_VALVE_TYPE == AIR_VALVE_SERVO && AIR_SOURCE == AIR_SOURCE_BELLOW_SERVO
  #if VALVE_PCA_INDEX == BELLOW_SERVO_PCA_INDEX && VALVE_PCA_PIN == BELLOW_SERVO_PCA_PIN
  #error "La valve generale et le servo de soufflet partagent le meme canal PCA."
  #endif
#endif
#if AIR_VALVE_TYPE == AIR_VALVE_SERVO && AIR_SOURCE == AIR_SOURCE_BLOWER_ESC
  #if VALVE_PCA_INDEX == ESC_PCA_INDEX && VALVE_PCA_PIN == ESC_PCA_PIN
  #error "La valve generale et l'ESC partagent le meme canal PCA."
  #endif
#endif

//===========================================================================================
// 7. HELPERS PARTAGES
//===========================================================================================

// Poids du niveau tenu pour une velocite donnee (voir config.h, section velocite).
inline float velocitySustainWeight(uint8_t velocity) {
    return VELOCITY_SUSTAIN_MIN + (1.0f - VELOCITY_SUSTAIN_MIN) * ((float)velocity / 127.0f);
}

#endif
