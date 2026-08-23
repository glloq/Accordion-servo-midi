#ifndef AIR_SOURCE_H
#define AIR_SOURCE_H

#include "settings.h"
#include "airFault.h"
#include "bellowController.h"
#include "servoBellowController.h"
#include "blowerController.h"

// =========================================================================================
// SELECTION DE LA SOURCE D'AIR
// -----------------------------------------------------------------------------------------
// `AirSource` designe la classe correspondant a AIR_SOURCE (config.h). Le choix est fait a
// la COMPILATION, pas a l'execution : les en-tetes des autres sources sont vides grace a
// leurs propres gardes, et une seule classe existe dans le binaire.
//
// Pourquoi pas une classe de base virtuelle ? Sur ATmega32U4 (2560 octets de SRAM, 32 Ko de
// FLASH), embarquer cinq pilotes pour n'en utiliser qu'un est indefendable, et aucune
// machine n'a deux sources d'air a la fois. Le typedef donne la meme modularite de code
// pour un cout nul a l'execution.
//
// INTERFACE COMMUNE que toute source doit fournir — c'est le contrat qu'Instrument utilise,
// et qu'une nouvelle source (pompe a pied, compresseur avec reservoir, orgue a soufflerie...)
// doit implementer pour se brancher sans toucher au reste du firmware :
//
//   void begin();                       initialisation materielle + mise en route
//   void update();                      a appeler a chaque tour de boucle
//   void setAirDemand(float);           somme des debits des notes, ponderee par la velocite
//   void setVolume(byte);               CC7  : 0 doit REELLEMENT couper la pression
//   void setExpression(byte);           CC11 : idem
//   void startCalibration();            (re)met la machine dans un etat connu
//   void stopAndDisable();              mise au repos, sans defaut
//   void openValve(); void closeValve();mise a l'air libre
//   bool isCalibrating() const;         vrai tant que les notes doivent etre refusees
//   bool isReady() const;               vrai quand l'instrument peut jouer
//   bool hasFault() const;
//   AirFault getFault() const;
//   void setFault(AirFault);
//   void clearFault();
//
// Toutes prennent le meme constructeur (ServoController&, AirValve&, PressureRegulator&),
// meme quand elles n'en utilisent qu'une partie : c'est ce qui permet a Instrument de les
// construire sans savoir laquelle est compilee.
// =========================================================================================

#if AIR_SOURCE == AIR_SOURCE_BELLOW_STEPPER
  typedef BellowController AirSource;
#elif AIR_SOURCE == AIR_SOURCE_BELLOW_SERVO
  typedef ServoBellowController AirSource;
#elif AIR_SOURCE == AIR_SOURCE_BLOWER_PWM || AIR_SOURCE == AIR_SOURCE_BLOWER_ESC || \
      AIR_SOURCE == AIR_SOURCE_PUMP_ONOFF
  typedef BlowerController AirSource;
#else
  #error "AIR_SOURCE ne correspond a aucune implementation (voir airSourceTypes.h)."
#endif

#endif
