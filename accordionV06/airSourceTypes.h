#ifndef AIR_SOURCE_TYPES_H
#define AIR_SOURCE_TYPES_H

// =========================================================================================
// Identifiants des systemes selectionnables dans config.h.
// -----------------------------------------------------------------------------------------
// Ce fichier ne contient QUE des constantes de selection : aucune dependance, ni Arduino,
// ni stdint. Il est inclus par config.h, lui-meme inclus par noteMapping.h qui doit rester
// compilable sur PC pour les tests natifs.
//
// Regle : un identifiant ne disparait jamais et ne change jamais de valeur. Les fichiers
// config.h generes par l'UI (tools/configurator) referencent ces noms ; les renumeroter
// invaliderait silencieusement des configurations existantes.
// =========================================================================================

// === SOURCES D'AIR (AIR_SOURCE) ===========================================================
// Le choix est fait a la COMPILATION : une seule implementation est compilee. Sur
// ATmega32U4 (2560 octets de SRAM) embarquer les cinq pilotes et choisir a l'execution
// n'etait pas tenable, et aucune machine n'a deux sources d'air a la fois.

// Soufflet acoustique entraine par un moteur pas a pas sur vis/courroie.
// Bidirectionnel : la pression est produite en poussant ET en tirant, avec inversion de
// sens avant les fins de course. C'est le systeme decrit dans le README.
#define AIR_SOURCE_BELLOW_STEPPER 0

// Soufflet acoustique entraine par un servomoteur (ou un verin servo) de grande course.
// Meme principe bidirectionnel, mais la position est commandee en angle : pas de fin de
// course, pas de homing, la butee est l'angle lui-meme. Nettement plus simple a construire,
// course et force limitees.
#define AIR_SOURCE_BELLOW_SERVO 1

// Turbine / ventilateur centrifuge continu pilote en PWM sur un MOSFET.
// Unidirectionnel : pression (soufflage) ou depression (aspiration) selon le montage,
// jamais les deux. Pas de course, donc pas d'inversion ni de fin de course.
#define AIR_SOURCE_BLOWER_PWM 2

// Turbine brushless pilotee par un ESC. L'ESC attend une impulsion type servo : elle est
// produite par un canal libre d'un PCA9685, deja present pour les anches. Sequence
// d'armement obligatoire au demarrage.
#define AIR_SOURCE_BLOWER_ESC 3

// Pompe a membrane / piston tout-ou-rien, typiquement avec un reservoir tampon.
// Regulation par hysteresis sur le capteur de pression si present, sinon par modulation
// lente du rapport cyclique. Protection anti-surchauffe par duree de marche continue.
#define AIR_SOURCE_PUMP_ONOFF 4

// === TRANSMISSION DU SOUFFLET PAS A PAS (BELLOW_TRANSMISSION) =============================
#define TRANSMISSION_SCREW 0 // Tige filetee : avance = SCREW_LEAD_MM par tour de vis
#define TRANSMISSION_BELT  1 // Courroie : avance = BELT_PITCH_MM * BELT_PULLEY_TEETH par tour

// === SENS DE L'AIR (AIR_DIRECTION) ========================================================
// N'a de sens que pour les sources unidirectionnelles (turbine, pompe) : indique si la
// machine souffle dans les anches ou aspire au travers. Change le signe attendu du capteur
// de pression, et donc celui de la regulation.
#define AIR_DIRECTION_BLOW 0 // Surpression
#define AIR_DIRECTION_DRAW 1 // Depression

// === VALVE GENERALE (AIR_VALVE_TYPE) ======================================================
// La valve generale met l'instrument a l'air libre : ouverte, le soufflet bouge sans
// produire de son et la pression residuelle s'echappe.
#define AIR_VALVE_NONE   0 // Aucune valve (machine a turbine sans mise a l'air libre)
#define AIR_VALVE_SERVO  1 // Servo sur un canal PCA9685 (montage decrit dans le README)
#define AIR_VALVE_SOLENOID 2 // Electrovanne tout-ou-rien sur une broche du MCU

// === ACTIONNEUR DES ANCHES (NOTE_ACTUATOR) ================================================
#define ACTUATOR_SERVO    0 // Servomoteur : angle ferme / angle ouvert
#define ACTUATOR_SOLENOID 1 // Electroaimant : sortie PCA tout-ou-rien, avec maintien reduit

// === PILOTAGE DE L'OE DES PCA9685 (PCA_OE_MODE) ===========================================
#define PCA_OE_SHARED  0 // Une seule broche pour tous les PCA (cablage le plus simple)
#define PCA_OE_PER_PCA 1 // Une broche par PCA : permet de couper un banc a la fois
#define PCA_OE_NONE    2 // OE cable en dur a la masse : les sorties ne sont jamais coupees

// === ROUTAGE DES NOTES MIDI (MIDI_ROUTING) ================================================
#define MIDI_ROUTING_CHANNEL 0 // Un canal MIDI par main (comportement historique)
#define MIDI_ROUTING_SPLIT   1 // Point de partage sur le numero de note, tous canaux
#define MIDI_ROUTING_MERGE   2 // Tous canaux, main droite prioritaire puis main gauche

// === REGULATION DE PRESSION (PRESSURE_CONTROL) ============================================
#define PRESSURE_CONTROL_OPEN_LOOP 0 // Boucle ouverte : la demande pilote directement
#define PRESSURE_CONTROL_PI        1 // Correcteur proportionnel + integral
#define PRESSURE_CONTROL_PID       2 // Correcteur proportionnel + integral + derive

#endif
