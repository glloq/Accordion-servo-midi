#ifndef CONFIG_H
#define CONFIG_H

// =========================================================================================
//  CONFIGURATION DE LA MACHINE  --  C'EST LE SEUL FICHIER A MODIFIER
// -----------------------------------------------------------------------------------------
//  Ce fichier decrit UNE machine : quelle source d'air, quels actionneurs, quelles notes.
//  Tout le reste du firmware en derive. Il peut etre ecrit a la main, mais il est surtout
//  destine a etre GENERE par l'interface de configuration :
//
//      tools/configurator/index.html   (page autonome, a ouvrir dans un navigateur)
//
//  L'UI verifie ce qu'un fichier ecrit a la main ne verifie pas : collisions de canaux
//  PCA, doublons de notes, broches I2C reutilisees en fin de course, vitesse demandee
//  incompatible avec le microstepping choisi.
//
//  Contraintes de ce fichier : uniquement des directives du preprocesseur, aucun #include
//  autre que airSourceTypes.h, aucun type Arduino. Il est inclus par noteMapping.h, qui
//  doit rester compilable sur PC pour les tests natifs.
//
//  Les valeurs DERIVEES (pas/mm, angle d'ouverture effectif, bornes de vitesse reellement
//  tenables...) ne sont pas ici : elles sont calculees et verifiees dans settings.h.
// =========================================================================================

#include "airSourceTypes.h"

// La plupart des choix structurants ci-dessous sont proteges par un #ifndef : ils peuvent
// donc etre surcharges depuis la ligne de compilation (-DAIR_SOURCE=AIR_SOURCE_BLOWER_PWM).
// C'est ce qui permet a l'integration continue de compiler et de tester toutes les
// combinaisons a partir d'un seul fichier de configuration.

// Nom libre de la configuration. Sert uniquement a s'y retrouver entre plusieurs machines.
#define CONFIG_NAME "Accordeon 59 anches - soufflet pas a pas"

#ifndef DEBUG
#define DEBUG false
#endif

//===========================================================================================
// 1. MIDI
//===========================================================================================

// Transports. Surchargables depuis la ligne de compilation (-DMIDI_TRANSPORT_USB=1), d'ou
// les gardes : platformio.ini definit un environnement par combinaison.
#ifndef MIDI_TRANSPORT_DIN
#define MIDI_TRANSPORT_DIN 1   // MIDI serie classique sur Serial1 (broches 0/1 du Leonardo)
#endif
#ifndef MIDI_TRANSPORT_USB
#define MIDI_TRANSPORT_USB 0   // MIDI USB natif : necessite la bibliotheque USB-MIDI (lathoub)
#endif

// Comment une note est attribuee a une main :
//   MIDI_ROUTING_CHANNEL : un canal par main. Le plus propre, demande un sequenceur qui
//                          sait emettre sur deux canaux.
//   MIDI_ROUTING_SPLIT   : point de partage sur le numero de note, tous canaux confondus.
//                          Permet de jouer un fichier MIDI mono-canal.
//   MIDI_ROUTING_MERGE   : tous canaux, la main droite est essayee en premier, la main
//                          gauche recupere ce qu'elle sait jouer.
#ifndef MIDI_ROUTING
#define MIDI_ROUTING MIDI_ROUTING_CHANNEL
#endif

#define MIDI_CHANNEL_LEFT  1   // Utilise si MIDI_ROUTING == MIDI_ROUTING_CHANNEL
#define MIDI_CHANNEL_RIGHT 2   // idem
#define MIDI_SPLIT_NOTE    54  // Utilise si MIDI_ROUTING == MIDI_ROUTING_SPLIT
                               // note < MIDI_SPLIT_NOTE -> main gauche, sinon main droite

//===========================================================================================
// 2. POLYPHONIE ET REPONSE A LA VELOCITE
//===========================================================================================

// Protection de l'alimentation 5 V : au-dela, une note entrante doit voler une voix moins
// prioritaire (voir NOTE_PRIORITY_* plus bas).
#define MAX_SIMULTANEOUS_NOTES 15

// La velocite n'agit PAS sur les actionneurs (une valve d'anche est ouverte ou fermee) mais
// sur la demande d'air, note par note :
//   debit_total = somme( airFlow_i * poidsTenu(velocity_i) )
//   poidsTenu(v) = VELOCITY_SUSTAIN_MIN + (1 - VELOCITY_SUSTAIN_MIN) * v/127
// L'attaque est un supplement temporaire applique a la SEULE derniere note declenchee.
#define VELOCITY_SUSTAIN_MIN  0.60f  // Debit relatif pour velocity = 1
#define VELOCITY_ATTACK_BOOST 0.50f  // Surcroit de debit pendant l'attaque (a velocity = 127)
#define VELOCITY_ATTACK_MS    120UL  // Duree de la phase d'attaque

//===========================================================================================
// 3. ACTIONNEURS DES ANCHES ET BUS I2C
//===========================================================================================

// Type d'actionneur des valves d'anches.
//   ACTUATOR_SERVO    : servomoteur, angle ferme -> angle ouvert (montage du README)
//   ACTUATOR_SOLENOID : electroaimant pilote en tout-ou-rien par le PCA9685
#ifndef NOTE_ACTUATOR
#define NOTE_ACTUATOR ACTUATOR_SERVO
#endif

// Nombre de PCA9685 et adresses I2C, dans l'ordre. NUM_PCA_TOTAL doit valoir exactement le
// nombre d'adresses listees : settings.h le verifie a la compilation.
#define NUM_PCA_TOTAL 4
#define PCA_ADDRESS_LIST 0x40, 0x41, 0x42, 0x43

// Frequence du bus I2C. Le PCA9685 supporte 1 MHz ; 400 kHz raccourcit nettement le temps
// de commande de dizaines de servos. Repasser a 100000L en cas de bus long ou bruite.
#define I2C_CLOCK_HZ 400000L

// Nombre d'erreurs I2C consecutives tolerees avant passage en defaut. Un PCA9685 qui cesse
// de repondre en cours de jeu laisse des anches ouvertes sans que rien ne le signale.
#define SERVO_I2C_ERROR_LIMIT 8

// --- Pilotage de l'OE des PCA9685 --------------------------------------------------------
// OE coupe les sorties PWM (les servos cessent de forcer, l'instrument devient silencieux).
// ATTENTION : OE ne coupe PAS le rail +5 V des servos. Une vraie mise hors tension demande
// un load switch / MOSFET high-side par banc.
//   PCA_OE_SHARED  : une seule broche pour tous les PCA
//   PCA_OE_PER_PCA : une broche par PCA, listees dans PCA_OE_PIN_LIST (meme ordre que les
//                    adresses). Permet de garder la valve alimentee en coupant les anches.
//   PCA_OE_NONE    : OE cable a la masse, jamais coupe
#ifndef PCA_OE_MODE
#define PCA_OE_MODE PCA_OE_SHARED
#endif
#define PCA_OE_PIN 4                      // Utilise si PCA_OE_MODE == PCA_OE_SHARED
#define PCA_OE_PIN_LIST 4, 7, 8, 12       // Utilise si PCA_OE_MODE == PCA_OE_PER_PCA

// Delai avant coupure de l'OE, compte depuis la DERNIERE commande d'actionneur (et non
// depuis la derniere note) : un servo, la valve generale notamment, doit avoir le temps
// d'atteindre sa position avant que le signal ne soit coupe.
#define PCA_DISABLE_DELAY 500

// --- Servomoteurs ------------------------------------------------------------------------
#define SERVO_PWM_FREQUENCY 50   // 50 Hz convient a la majorite des servos analogiques
#define SERVO_MIN_ANGLE 0        // Angle minimal admis (bornage de securite)
#define SERVO_MAX_ANGLE 180      // Angle maximal admis
#define SERVO_MIN_PWM 150        // Comptes PCA9685 correspondant a SERVO_MIN_ANGLE
#define SERVO_MAX_PWM 600        // Comptes PCA9685 correspondant a SERVO_MAX_ANGLE

// Angles des valves d'anches. Deux sens de montage coexistent sur la machine du README :
//   montage normal : repos a SERVO_CLOSED_ANGLE, ouverture a CLOSED - SERVO_OPEN_ANGLE
//   montage miroir : repos a SERVO_CLOSED_ANGLE_MIRROR, ouverture a CLOSED + SERVO_OPEN_ANGLE
// Le sens est porte par chaque note (champ openDirection des tables plus bas).
#define SERVO_CLOSED_ANGLE        130
#define SERVO_CLOSED_ANGLE_MIRROR 50
#define SERVO_OPEN_ANGLE          40

// Espacement entre servos lors de la fermeture initiale : refermer des dizaines de servos
// au meme instant provoque un appel de courant que l'alimentation n'encaisse pas.
#define SERVO_INIT_STAGGER_MS 15

// --- Electroaimants (utilise seulement si NOTE_ACTUATOR == ACTUATOR_SOLENOID) -------------
// Un electroaimant maintenu a pleine puissance chauffe et finit par bruler. Le PCA9685
// permet de reduire le courant de maintien apres l'appel : pleine puissance pendant
// SOLENOID_PULLIN_MS, puis SOLENOID_HOLD_PERCENT.
// Le renfort d'appel n'est applique qu'a la DERNIERE note declenchee, comme le supplement
// de debit d'attaque : suivre une echeance par note couterait plus de SRAM que la machine
// n'en a.
#define SOLENOID_PULLIN_MS 40
#define SOLENOID_HOLD_PERCENT 45

//===========================================================================================
// 4. VALVE GENERALE (MISE A L'AIR LIBRE)
//===========================================================================================
// Ouverte : la pression s'echappe, le soufflet peut bouger sans produire de son. Elle est
// ouverte au demarrage, pendant la calibration, en defaut et au repos ; fermee des la
// premiere note.
//   AIR_VALVE_SERVO    : servo sur un canal PCA9685
//   AIR_VALVE_SOLENOID : electrovanne tout-ou-rien sur une broche du MCU
//   AIR_VALVE_NONE     : pas de valve (machine a turbine qui s'arrete simplement)
#ifndef AIR_VALVE_TYPE
#define AIR_VALVE_TYPE AIR_VALVE_SERVO
#endif

// --- si AIR_VALVE_SERVO --------------------------------------------------------------
#define VALVE_PCA_INDEX 3        // Index dans la liste PCA_ADDRESS_LIST (0 = premier PCA)
#define VALVE_PCA_PIN 15         // Canal PCA (0-15). Aucune note ne doit l'utiliser :
                                 // test_note_mapping le verifie.
#define VALVE_PCA_ANGLE_OPEN 70
#define VALVE_PCA_ANGLE_CLOSE 120

// --- si AIR_VALVE_SOLENOID -----------------------------------------------------------
#define VALVE_SOLENOID_PIN 13
#define VALVE_SOLENOID_OPEN_LEVEL 1  // Niveau logique a ecrire pour OUVRIR (1 = HIGH)

//===========================================================================================
// 5. SOURCE D'AIR
//===========================================================================================
// Une seule est compilee. Les blocs de parametres des autres sources sont ignores : ils
// restent presents pour pouvoir changer de systeme sans reecrire le fichier.
#ifndef AIR_SOURCE
#define AIR_SOURCE AIR_SOURCE_BELLOW_STEPPER
#endif

// Sens de l'air pour les sources unidirectionnelles (turbine, pompe). Sans effet sur les
// soufflets, qui produisent la pression dans les deux sens de deplacement.
#ifndef AIR_DIRECTION
#define AIR_DIRECTION AIR_DIRECTION_BLOW
#endif

// Temps sans aucune note avant mise au repos : source d'air coupee, valve ouverte.
#define AIR_INACTIVITY_TIMEOUT 60000UL

//-------------------------------------------------------------------------------------------
// 5.a  SOUFFLET PAS A PAS  (AIR_SOURCE_BELLOW_STEPPER)
//-------------------------------------------------------------------------------------------

// Broches Step/Dir/Enable du driver (TMC2209, A4988, DRV8825...).
#define STEPPER_STEP_PIN 10
#define STEPPER_DIR_PIN 9
#define STEPPER_EN_PIN 11
#define STEPPER_EN_ACTIVE_LOW 1   // 1 = ENABLE actif bas (cas de la quasi-totalite des drivers)
#define STEPPER_INVERT_DIR 0      // 1 = inverse le sens sans retoucher au cablage

// --- Transmission ------------------------------------------------------------------------
// !! MICRO_STEP doit correspondre au reglage PHYSIQUE du driver (cavaliers MS1/MS2 ou
// configuration UART). En mode standalone, MS1=MS2=LOW donne souvent 1/8 et non 1/16 :
// une erreur ici fausse la course d'un facteur 2 a 16.
#define MOTOR_STEPS_PER_REV 200   // NEMA17 1,8 deg => 200 pas/tour
#define MICRO_STEP 16
#define GEAR_RATIO 2.0f           // Reduction poulies : 2 tours moteur = 1 tour de sortie

#ifndef BELLOW_TRANSMISSION
#define BELLOW_TRANSMISSION TRANSMISSION_SCREW
#endif
#define SCREW_LEAD_MM 16.0f       // si TRANSMISSION_SCREW : avance en mm par tour de vis
#define BELT_PITCH_MM 2.0f        // si TRANSMISSION_BELT  : pas de la courroie (GT2 = 2 mm)
#define BELT_PULLEY_TEETH 20      // si TRANSMISSION_BELT  : dents de la poulie motrice

// --- Course et inversion -----------------------------------------------------------------
#define BELLOW_MIN_POSITION 0     // Soufflet ferme (zero fixe par la calibration), en mm
#define BELLOW_MAX_POSITION 200   // Ouverture maximale, en mm

// Inversion du sens AVANT d'atteindre les fins de course, evaluee en continu : une note
// tenue ne genere aucun evenement MIDI et doit malgre tout faire osciller le soufflet.
#define BELLOW_REVERSE_THRESHOLD_OPEN 0.7f
#define BELLOW_REVERSE_THRESHOLD_CLOSE 0.3f

// --- Vitesses ----------------------------------------------------------------------------
#define NORMAL_SPEED 10           // Vitesse pour une seule note a debit 1.0 (mm/s)
#define STEPPER_MIN_SPEED 2       // mm/s
#define STEPPER_MAX_SPEED 300     // mm/s (borne haute de configuration, voir settings.h)
#define STEPPER_MIN_ACCEL 5       // mm/s^2
#define STEPPER_MAX_ACCEL 100     // mm/s^2

// Frequence de pas maximale reellement tenable par le MCU. FlexyStepper ne produit qu'UN
// pas par appel a processMovement() : la frequence est bornee par la periode de la boucle
// principale, pas seulement par le CPU. test_timing mesure la valeur atteinte.
#define STEPPER_MAX_STEP_RATE_HZ 10000.0f

// Appels a processMovement() par tour de boucle. Auto-limites par micros() : les appels en
// trop sont quasi gratuits, mais rattrapent les pas perdus pendant une rafale MIDI ou une
// ecriture I2C (~110 us).
#define STEPPER_SERVICE_CALLS 4

// Deceleration de l'arret d'urgence. Le driver etant deja coupe, cette rampe ne produit
// aucun mouvement : elle ramene l'etat interne de FlexyStepper a l'arret, seule condition
// ou setCurrentPosition() est legitime.
#define STEPPER_EMERGENCY_DECEL 5000.0f
#define STEPPER_STOP_TIMEOUT_MS 1000UL

// --- Fins de course ----------------------------------------------------------------------
// ATTENTION : sur Leonardo / Micro, D2 et D3 sont SDA et SCL (bus des PCA9685). Les fins de
// course ne doivent JAMAIS y etre cables. settings.h refuse de compiler si c'est le cas.
#define LIMIT_SWITCH_MIN_PIN 5    // Butee basse (soufflet ferme) : sert de reference au zero
#define LIMIT_SWITCH_MAX_PIN 6    // Butee haute (ouverture maximale)
#define LIMIT_SWITCH_ACTIVE_LOW 1 // 1 = contact ferme -> niveau bas (capteur optique NPN,
                                  //     ou contact mecanique avec pull-up interne)
#define ENDSTOP_DEBOUNCE_MS 50    // Ne retarde JAMAIS l'arret : le contact brut coupe
                                  // immediatement le driver. Le debounce sert seulement a
                                  // decider ensuite si le contact etait reel ou parasite.

// --- Calibration (homing) en deux passes -------------------------------------------------
// Approche rapide, degagement, reapproche lente : le premier contact ne fait que localiser
// grossierement la butee (surcourse inconnue, driver coupe sans rampe maitrisee) ; seule la
// seconde approche fixe un zero repetable.
#define HOMING_SPEED 10.0f
#define HOMING_SLOW_SPEED 1.5f
#define HOMING_BACKOFF_MM 4.0f
#define HOMING_MAX_DISTANCE 250.0f
#define HOMING_TIMEOUT_MS 60000UL

//-------------------------------------------------------------------------------------------
// 5.b  SOUFFLET A SERVOMOTEUR  (AIR_SOURCE_BELLOW_SERVO)
//-------------------------------------------------------------------------------------------
// Le servo porte directement l'ouverture du soufflet : pas de fin de course, pas de homing,
// la butee est l'angle lui-meme. La demande d'air fixe la VITESSE de balayage, et le sens
// s'inverse aux memes seuils que le soufflet pas a pas.
#define BELLOW_SERVO_PCA_INDEX 3
#define BELLOW_SERVO_PCA_PIN 12       // Canal libre : 13 et 14 portent les notes 70 et 71
#define BELLOW_SERVO_ANGLE_CLOSED 20   // Soufflet ferme
#define BELLOW_SERVO_ANGLE_OPEN 160    // Soufflet ouvert (peut etre < CLOSED si montage inverse)
#define BELLOW_SERVO_MIN_SPEED_DPS 5.0f    // Vitesse de balayage a debit minimal (deg/s)
#define BELLOW_SERVO_MAX_SPEED_DPS 120.0f  // Vitesse de balayage a debit maximal (deg/s)
#define BELLOW_SERVO_UPDATE_MS 20          // Periode d'envoi de la consigne (50 Hz)
// Marges avant les angles extremes, en fraction de la course : equivalent des seuils 30/70 %
// du soufflet pas a pas.
#define BELLOW_SERVO_REVERSE_OPEN 0.9f
#define BELLOW_SERVO_REVERSE_CLOSE 0.1f
// Demande d'air atteignant BELLOW_SERVO_MAX_SPEED_DPS. Une note seule vaut environ 1.0.
#define BELLOW_SERVO_DEMAND_FULL_SCALE 6.0f

//-------------------------------------------------------------------------------------------
// 5.c  TURBINE PWM  (AIR_SOURCE_BLOWER_PWM)
//-------------------------------------------------------------------------------------------
// Ventilateur centrifuge ou soufflante continue sur MOSFET. Unidirectionnel : pas de course,
// pas d'inversion, pas de calibration. Le rapport cyclique suit la demande d'air.
#define BLOWER_PWM_PIN 9    // Broche PWM materielle, hors SDA/SCL
#define BLOWER_PWM_INVERT 0        // 1 si l'etage de puissance inverse (driver low-side PNP)
#define BLOWER_DUTY_MIN 60         // 0-255 : duty produisant le debit minimal utile
#define BLOWER_DUTY_MAX 255        // 0-255 : duty a debit maximal
#define BLOWER_DUTY_IDLE 0         // Duty maintenu sans note. > 0 garde la turbine en rotation
                                   // et supprime la latence de redemarrage (au prix du bruit).
#define BLOWER_SPINUP_DUTY 200     // Impulsion de demarrage : un moteur a l'arret ne part pas
#define BLOWER_SPINUP_MS 120       // a BLOWER_DUTY_MIN.
// Demande d'air (somme des debits de notes) correspondant a BLOWER_DUTY_MAX. Au-dela, le
// duty sature : c'est la limite de polyphonie utile de la turbine.
#define BLOWER_DEMAND_FULL_SCALE 6.0f

//-------------------------------------------------------------------------------------------
// 5.d  TURBINE BRUSHLESS SUR ESC  (AIR_SOURCE_BLOWER_ESC)
//-------------------------------------------------------------------------------------------
// L'ESC attend une impulsion type servo : elle est produite par un canal libre d'un PCA9685,
// deja cadence a SERVO_PWM_FREQUENCY. Un ESC non arme ignore toute consigne : la sequence
// d'armement (impulsion minimale maintenue) est obligatoire au demarrage.
#define ESC_PCA_INDEX 3
#define ESC_PCA_PIN 11            // Canal libre du 4e PCA
#define ESC_PULSE_MIN_US 1000      // Impulsion "arret" / armement
#define ESC_PULSE_MAX_US 2000      // Impulsion pleine puissance
#define ESC_PULSE_IDLE_US 1000     // Impulsion au repos (> MIN pour garder la turbine lancee)
#define ESC_ARM_MS 2500            // Duree de maintien de l'impulsion d'armement
#define ESC_DEMAND_FULL_SCALE 6.0f // Demande d'air correspondant a ESC_PULSE_MAX_US

//-------------------------------------------------------------------------------------------
// 5.e  POMPE TOUT-OU-RIEN  (AIR_SOURCE_PUMP_ONOFF)
//-------------------------------------------------------------------------------------------
// Pompe a membrane ou a piston, typiquement avec un reservoir tampon.
//   - avec capteur de pression : regulation par hysteresis autour de la consigne
//   - sans capteur : modulation lente du rapport cyclique sur PUMP_CYCLE_MS
// Une pompe de ce type ne supporte pas la marche continue : PUMP_MAX_RUN_MS force un repos.
#define PUMP_PIN 8         // Sortie tout-ou-rien : aucun timer necessaire
#define PUMP_ACTIVE_LEVEL 1        // Niveau logique qui met la pompe en marche
#define PUMP_HYSTERESIS_KPA 0.4f   // Bande morte autour de la consigne (avec capteur)
#define PUMP_CYCLE_MS 200UL        // Periode de modulation lente (sans capteur)
#define PUMP_MIN_DUTY_PERCENT 20   // Duty a debit minimal
#define PUMP_MAX_DUTY_PERCENT 100  // Duty a debit maximal
#define PUMP_DEMAND_FULL_SCALE 6.0f
#define PUMP_MAX_RUN_MS 30000UL    // Marche continue maximale avant repos force
#define PUMP_REST_MS 5000UL        // Duree du repos force

//===========================================================================================
// 6. CAPTEUR DE PRESSION ET REGULATION
//===========================================================================================
// Sans capteur (PRESSURE_SENSOR_ENABLED 0), tout le systeme fonctionne en boucle ouverte :
// la demande d'air pilote directement la vitesse ou le duty, et la pression reelle depend de
// l'etancheite, du nombre d'anches ouvertes et de l'usure. Avec capteur, la demande devient
// une CONSIGNE de pression et le correcteur ajuste la commande.
#ifndef PRESSURE_SENSOR_ENABLED
#define PRESSURE_SENSOR_ENABLED 0
#endif

#define PRESSURE_SENSOR_PIN A0
#define PRESSURE_ADC_AT_ZERO 102        // Comptes ADC a pression nulle (0,5 V sur 5 V = 102)
#define PRESSURE_ADC_PER_KPA 40.9f      // Comptes ADC par kPa (capteur type MPX5010 : 4,5 V
                                        // pleine echelle a 10 kPa sur 1024 comptes)
#define PRESSURE_FILTER_ALPHA 0.25f     // Filtre passe-bas du premier ordre (0 = fige, 1 = brut)
#define PRESSURE_SAMPLE_MS 5UL          // Periode d'echantillonnage et de calcul du correcteur

// Consigne : pression visee pour une demande d'air de 1.0, et plafond absolu.
#define PRESSURE_TARGET_KPA 2.5f
#define PRESSURE_MAX_KPA 8.0f           // Au-dela : coupure de securite (fuite bouchee, valve
                                        // bloquee fermee, anche coincee)

#ifndef PRESSURE_CONTROL
#define PRESSURE_CONTROL PRESSURE_CONTROL_PI
#endif
#define PRESSURE_KP 0.35f
#define PRESSURE_KI 0.80f               // par seconde
#define PRESSURE_KD 0.00f               // secondes (ignore si PRESSURE_CONTROL != _PID)
#define PRESSURE_INTEGRAL_LIMIT 1.0f    // Anti-emballement de l'integrateur

// Bornes du facteur correctif applique a la commande calculee en boucle ouverte. Le
// correcteur RATTRAPE la boucle ouverte, il ne la remplace pas : le laisser aller de 0 a
// l'infini transformerait une derive de capteur en emballement mecanique.
#define PRESSURE_SCALE_MIN 0.25f
#define PRESSURE_SCALE_MAX 2.50f

//===========================================================================================
// 7. TABLES DE NOTES
//===========================================================================================
// Une ligne par anche pilotee, sous forme de X-macro :
//
//   X(midiNote, pcaAddress, pcaChannel, airFlow, closedAngle, openDirection, priority)
//
//   midiNote      numero de note MIDI, explicite : la disposition Stradella de la main
//                 gauche n'est pas chromatique, aucune continuite n'est supposee
//   pcaAddress    adresse I2C du PCA9685 qui porte l'actionneur
//   pcaChannel    canal 0-15 sur ce PCA
//   airFlow       consommation d'air relative de l'anche (les graves consomment plus)
//   closedAngle   angle de repos du servo (ignore si NOTE_ACTUATOR == ACTUATOR_SOLENOID)
//   openDirection true  = ouverture a closedAngle - SERVO_OPEN_ANGLE (montage normal)
//                 false = ouverture a closedAngle + SERVO_OPEN_ANGLE (montage miroir)
//   priority      NOTE_PRIORITY_BASS (3) > NOTE_PRIORITY_MELODY (2) > NOTE_PRIORITY_CHORD (1)
//                 Quand MAX_SIMULTANEOUS_NOTES est atteint, une note entrante ne peut voler
//                 que la place d'une note STRICTEMENT moins prioritaire, la plus ancienne.
//
// NUM_NOTES_RIGHT / NUM_NOTES_LEFT doivent valoir exactement le nombre de lignes :
// noteMapping.cpp le verifie a la compilation. Une main peut avoir 0 note (instrument
// melodie seule, ou basses seules) : la table correspondante est alors vide.

#define NUM_NOTES_RIGHT 34
#define NUM_NOTES_LEFT  24

// --- Main droite : melodie, 34 notes chromatiques de Fa#3 (54) a Re#6 (87) ---------------
// Premier bloc de 18 servos en montage normal, second bloc de 16 en montage miroir.
#define RIGHT_HAND_NOTE_LIST(X) \
    X( 54, 0x40,  0, 1.00f, SERVO_CLOSED_ANGLE,        true,  NOTE_PRIORITY_MELODY) \
    X( 55, 0x40,  1, 0.98f, SERVO_CLOSED_ANGLE,        true,  NOTE_PRIORITY_MELODY) \
    X( 56, 0x40,  2, 0.96f, SERVO_CLOSED_ANGLE,        true,  NOTE_PRIORITY_MELODY) \
    X( 57, 0x40,  3, 0.94f, SERVO_CLOSED_ANGLE,        true,  NOTE_PRIORITY_MELODY) \
    X( 58, 0x40,  4, 0.92f, SERVO_CLOSED_ANGLE,        true,  NOTE_PRIORITY_MELODY) \
    X( 59, 0x40,  5, 0.90f, SERVO_CLOSED_ANGLE,        true,  NOTE_PRIORITY_MELODY) \
    X( 60, 0x40,  6, 0.88f, SERVO_CLOSED_ANGLE,        true,  NOTE_PRIORITY_MELODY) \
    X( 61, 0x40,  7, 0.86f, SERVO_CLOSED_ANGLE,        true,  NOTE_PRIORITY_MELODY) \
    X( 62, 0x40,  8, 0.84f, SERVO_CLOSED_ANGLE,        true,  NOTE_PRIORITY_MELODY) \
    X( 63, 0x40,  9, 0.82f, SERVO_CLOSED_ANGLE,        true,  NOTE_PRIORITY_MELODY) \
    X( 64, 0x40, 10, 0.80f, SERVO_CLOSED_ANGLE,        true,  NOTE_PRIORITY_MELODY) \
    X( 65, 0x40, 11, 0.78f, SERVO_CLOSED_ANGLE,        true,  NOTE_PRIORITY_MELODY) \
    X( 66, 0x40, 12, 0.76f, SERVO_CLOSED_ANGLE,        true,  NOTE_PRIORITY_MELODY) \
    X( 67, 0x40, 13, 0.74f, SERVO_CLOSED_ANGLE,        true,  NOTE_PRIORITY_MELODY) \
    X( 68, 0x40, 14, 0.72f, SERVO_CLOSED_ANGLE,        true,  NOTE_PRIORITY_MELODY) \
    X( 69, 0x40, 15, 0.70f, SERVO_CLOSED_ANGLE,        true,  NOTE_PRIORITY_MELODY) \
    X( 70, 0x43, 13, 0.68f, SERVO_CLOSED_ANGLE,        true,  NOTE_PRIORITY_MELODY) \
    X( 71, 0x43, 14, 0.66f, SERVO_CLOSED_ANGLE,        true,  NOTE_PRIORITY_MELODY) \
    X( 72, 0x41,  0, 0.66f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_MELODY) \
    X( 73, 0x41,  1, 0.66f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_MELODY) \
    X( 74, 0x41,  2, 0.64f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_MELODY) \
    X( 75, 0x41,  3, 0.62f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_MELODY) \
    X( 76, 0x41,  4, 0.60f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_MELODY) \
    X( 77, 0x41,  5, 0.58f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_MELODY) \
    X( 78, 0x41,  6, 0.56f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_MELODY) \
    X( 79, 0x41,  7, 0.54f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_MELODY) \
    X( 80, 0x41,  8, 0.52f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_MELODY) \
    X( 81, 0x41,  9, 0.50f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_MELODY) \
    X( 82, 0x41, 10, 0.48f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_MELODY) \
    X( 83, 0x41, 11, 0.46f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_MELODY) \
    X( 84, 0x41, 12, 0.44f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_MELODY) \
    X( 85, 0x41, 13, 0.42f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_MELODY) \
    X( 86, 0x41, 14, 0.40f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_MELODY) \
    X( 87, 0x41, 15, 0.38f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_MELODY)

// --- Main gauche : basses et accords, disposition Stradella -------------------------------
// Rangee basse (grave) : 36 43 38 45 40 47 42 49 44 51 46 41
// Rangee aigue         : 48 55 50 57 52 59 54 61 56 63 58 53
// Ces notes ne sont PAS contigues : la recherche se fait par numero de note.
#define LEFT_HAND_NOTE_LIST(X) \
    X( 36, 0x42,  0, 1.00f, SERVO_CLOSED_ANGLE,        true,  NOTE_PRIORITY_BASS) \
    X( 43, 0x42,  1, 1.00f, SERVO_CLOSED_ANGLE,        true,  NOTE_PRIORITY_BASS) \
    X( 38, 0x42,  2, 1.00f, SERVO_CLOSED_ANGLE,        true,  NOTE_PRIORITY_BASS) \
    X( 45, 0x42,  3, 1.00f, SERVO_CLOSED_ANGLE,        true,  NOTE_PRIORITY_BASS) \
    X( 40, 0x42,  4, 1.00f, SERVO_CLOSED_ANGLE,        true,  NOTE_PRIORITY_BASS) \
    X( 47, 0x42,  5, 1.00f, SERVO_CLOSED_ANGLE,        true,  NOTE_PRIORITY_BASS) \
    X( 42, 0x42,  6, 1.00f, SERVO_CLOSED_ANGLE,        true,  NOTE_PRIORITY_BASS) \
    X( 49, 0x42,  7, 1.00f, SERVO_CLOSED_ANGLE,        true,  NOTE_PRIORITY_BASS) \
    X( 44, 0x42,  8, 1.00f, SERVO_CLOSED_ANGLE,        true,  NOTE_PRIORITY_BASS) \
    X( 51, 0x42,  9, 1.00f, SERVO_CLOSED_ANGLE,        true,  NOTE_PRIORITY_BASS) \
    X( 46, 0x42, 10, 1.00f, SERVO_CLOSED_ANGLE,        true,  NOTE_PRIORITY_BASS) \
    X( 41, 0x42, 11, 1.00f, SERVO_CLOSED_ANGLE,        true,  NOTE_PRIORITY_BASS) \
    X( 48, 0x42, 12, 1.00f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_CHORD) \
    X( 55, 0x42, 13, 1.00f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_CHORD) \
    X( 50, 0x42, 14, 1.00f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_CHORD) \
    X( 57, 0x42, 15, 1.00f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_CHORD) \
    X( 52, 0x43,  1, 1.00f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_CHORD) \
    X( 59, 0x43,  2, 1.00f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_CHORD) \
    X( 54, 0x43,  3, 1.00f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_CHORD) \
    X( 61, 0x43,  4, 1.00f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_CHORD) \
    X( 56, 0x43,  5, 1.00f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_CHORD) \
    X( 63, 0x43,  6, 1.00f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_CHORD) \
    X( 58, 0x43,  7, 1.00f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_CHORD) \
    X( 53, 0x43,  8, 1.00f, SERVO_CLOSED_ANGLE_MIRROR, false, NOTE_PRIORITY_CHORD)

#endif
