#ifndef INSTRUMENT_H
#define INSTRUMENT_H

// L'ordre des includes suit l'ordre de construction des membres.
#include "servoController.h"
#include "airValve.h"
#include "pressureRegulator.h"
#include "airSource.h"
#include "handController.h"

// Etats globaux de l'instrument.
// Tant que l'etat n'est pas SYS_READY, les NoteOn sont refusees : accepter des notes
// pendant la mise en route rendait le systeme incoherent (valve fermee, compteur de notes
// incremente, mais source d'air encore en calibration).
enum SystemState {
    SYS_BOOT,       // Avant begin()
    SYS_SERVO_INIT, // Detection des PCA9685 et fermeture initiale des actionneurs
    SYS_HOMING,     // Mise en route de la source d'air (recherche du zero, armement d'ESC...)
    SYS_READY,      // Jeu autorise
    SYS_FAULT       // Defaut verrouille
};

// Origine du defaut. Les defauts de la source d'air gardent leur code detaille dans AirFault.
enum InstrumentFault {
    INST_FAULT_NONE = 0,
    INST_FAULT_PCA_MISSING, // Un PCA9685 n'a pas repondu au demarrage
    INST_FAULT_PCA_BUS,     // Trop d'erreurs I2C consecutives en fonctionnement
    INST_FAULT_BELLOW       // Voir getFault() pour le detail
};

class Instrument {
public:
    Instrument();

    void begin();                                        // Initialise les controleurs
    void noteOn(byte note, byte velocity, byte channel); // Gestion d'une note ON
    void noteOff(byte note, byte channel);               // Gestion d'une note OFF
    void setVolume(byte volume);                         // CC7
    void setExpression(byte expression);                 // CC11
    void setSustain(bool active);                        // CC64
    void allNotesOff();                                  // Desactive toutes les notes (MIDI Panic)
    void update();                                       // Met a jour l'etat general

    bool isReady() const { return state == SYS_READY; }
    SystemState getState() const { return state; }
    InstrumentFault getInstrumentFault() const { return instrumentFault; }
    AirFault getFault() const { return airSource.getFault(); }
    byte getActiveNoteCount() const;
    uint8_t getMissingPcaMask() const { return servoController.getMissingMask(); }

    // Acces diagnostic pour les tests : la source compilee expose, en plus de l'interface
    // commune, ses propres grandeurs (position du soufflet, rapport cyclique, angle...).
    AirSource &getAirSource() { return airSource; }
    const PressureRegulator &getPressure() const { return pressureRegulator; }

private:
    // ORDRE DE CONSTRUCTION : servoController doit exister avant les objets qui en
    // conservent une reference, et la valve avant la source d'air qui s'en sert.
    ServoController servoController;
    AirValve airValve;
    PressureRegulator pressureRegulator;
    AirSource airSource;
    HandController leftHand;
    HandController rightHand;

    SystemState state;
    InstrumentFault instrumentFault;

    uint16_t noteSequence; // Compteur d'activation, pour departager les vols de voix

    // Attaque : supplement de debit applique a la SEULE derniere note declenchee, et non
    // a l'ensemble des notes tenues.
    HandController *attackHand;
    byte attackIndex;
    uint32_t lastAttackTime;
    bool attackActive;

    bool sustainActive;  // Pedale de sustain (CC64)
    bool airIdle;        // Mise au repos deja effectuee (evite de re-commander en boucle)
    uint32_t lastActivityTime; // Derniere date ou une note etait ouverte

    // Attribution d'une note a une main, selon MIDI_ROUTING.
    HandController *handForNote(byte note, byte channel);

    float attackBonus() const;      // Supplement de debit de la note en cours d'attaque
    void refreshAirDemand();        // Recalcule et pousse la demande d'air vers la source
    bool stealVoice(byte incomingPriority); // Libere une voix moins prioritaire
    void clearAttack();
    void enterFault(InstrumentFault reason);

    void manageInactivity(); // Mise au repos de la source d'air apres inactivite
    void managePCA();        // Coupe l'OE des PCA quand plus rien ne bouge
};

#endif
