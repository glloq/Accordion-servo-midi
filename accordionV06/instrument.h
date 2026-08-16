#ifndef INSTRUMENT_H
#define INSTRUMENT_H

// L'ordre des includes suit l'ordre de construction des membres.
#include "servoController.h"
#include "bellowController.h"
#include "handController.h"

// Etats globaux de l'instrument.
// Tant que l'etat n'est pas SYS_READY, les NoteOn sont refusees : accepter des notes
// pendant le homing rendait le systeme incoherent (valve fermee, compteur de notes
// incremente, mais soufflet immobile car en calibration).
enum SystemState {
    SYS_BOOT,       // Avant begin()
    SYS_SERVO_INIT, // Detection des PCA9685 et fermeture initiale des servos
    SYS_HOMING,     // Recherche du zero du soufflet
    SYS_READY,      // Jeu autorise
    SYS_FAULT       // Defaut verrouille
};

// Origine du defaut. Les defauts du soufflet gardent leur code detaille dans BellowFault.
enum InstrumentFault {
    INST_FAULT_NONE = 0,
    INST_FAULT_PCA_MISSING, // Un PCA9685 n'a pas repondu au demarrage
    INST_FAULT_PCA_BUS,     // Trop d'erreurs I2C consecutives en fonctionnement
    INST_FAULT_BELLOW       // Voir getBellowFault() pour le detail
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
    void update();                                       // Met a jour l'etat general de l'instrument

    bool isReady() const { return state == SYS_READY; }
    SystemState getState() const { return state; }
    InstrumentFault getInstrumentFault() const { return instrumentFault; }
    BellowFault getFault() const { return bellowController.getFault(); }
    byte getActiveNoteCount() const;
    uint8_t getMissingPcaMask() const { return servoController.getMissingMask(); }

private:
    // ORDRE DE CONSTRUCTION : servoController doit exister avant les objets qui en
    // conservent une reference.
    ServoController servoController;
    BellowController bellowController;
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
    bool bellowIdle;     // Mise au repos deja effectuee (evite de re-commander en boucle)
    uint32_t lastActivityTime; // Derniere date ou une note etait ouverte

    HandController *handForChannel(byte channel);
    float attackBonus() const;      // Supplement de debit de la note en cours d'attaque
    void refreshAirDemand();        // Recalcule et pousse la demande d'air vers le soufflet
    bool stealVoice(byte incomingPriority); // Libere une voix moins prioritaire
    void clearAttack();
    void enterFault(InstrumentFault reason);

    void manageInactivity(); // Mise au repos du soufflet apres inactivite
    void managePCA();        // Coupe l'OE des PCA quand plus rien ne bouge
};

#endif
