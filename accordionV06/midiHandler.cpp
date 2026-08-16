#include "midiHandler.h"
#include <MIDI.h>

// Constantes MIDI Control Change
#define CC_VOLUME         7    // Volume principal
#define CC_EXPRESSION     11   // Expression
#define CC_SUSTAIN        64   // Pedale de sustain
#define CC_ALL_SOUND_OFF  120  // Arret de tous les sons
#define CC_ALL_NOTES_OFF  123  // Desactive toutes les notes

#if !MIDI_TRANSPORT_DIN && !MIDI_TRANSPORT_USB
#error "Aucun transport MIDI actif : definir MIDI_TRANSPORT_DIN et/ou MIDI_TRANSPORT_USB."
#endif

// === TRANSPORT DIN / UART (Serial1) ===
#if MIDI_TRANSPORT_DIN
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI_DIN);
#endif

// === TRANSPORT USB NATIF (Leonardo / Micro) ===
// Necessite la bibliotheque "USB-MIDI" (lathoub). Active par -DMIDI_TRANSPORT_USB=1.
#if MIDI_TRANSPORT_USB
#include <USB-MIDI.h>
USBMIDI_CREATE_INSTANCE(0, MIDI_USB);
#endif

MidiHandler::MidiHandler(Instrument& instr)
    : instrument(instr)
#if MIDI_TRANSPORT_DIN && MIDI_TRANSPORT_USB
    , preferUsb(false)
#endif
{}

void MidiHandler::begin() {
#if MIDI_TRANSPORT_DIN
    MIDI_DIN.begin(MIDI_CHANNEL_OMNI); // Ecoute tous les canaux MIDI
    MIDI_DIN.turnThruOff();            // Evite de re-emettre l'entree sur la sortie
#endif

#if MIDI_TRANSPORT_USB
    MIDI_USB.begin(MIDI_CHANNEL_OMNI);
    MIDI_USB.turnThruOff();
#endif

    #if DEBUG
    Serial.println(F("[DEBUG] MIDI Handler initialise"));
    #endif
}

// Lit un message sur le transport DIN. Retourne true si un message a ete traite.
bool MidiHandler::readDin() {
#if MIDI_TRANSPORT_DIN
    if (!MIDI_DIN.read()) return false;
    dispatch(MIDI_DIN.getType(), MIDI_DIN.getChannel(), MIDI_DIN.getData1(), MIDI_DIN.getData2());
    return true;
#else
    return false;
#endif
}

// Lit un message sur le transport USB. Retourne true si un message a ete traite.
bool MidiHandler::readUsb() {
#if MIDI_TRANSPORT_USB
    if (!MIDI_USB.read()) return false;
    dispatch(MIDI_USB.getType(), MIDI_USB.getChannel(), MIDI_USB.getData1(), MIDI_USB.getData2());
    return true;
#else
    return false;
#endif
}

// Traite AU PLUS un message par appel : le generateur de pas reprend la main entre chaque.
void MidiHandler::update() {
#if MIDI_TRANSPORT_DIN && MIDI_TRANSPORT_USB
    // Alternance stricte : une rafale sur un transport ne peut pas affamer l'autre.
    if (preferUsb) {
        if (!readUsb()) readDin();
    } else {
        if (!readDin()) readUsb();
    }
    preferUsb = !preferUsb;
#elif MIDI_TRANSPORT_DIN
    readDin();
#else
    readUsb();
#endif
}

// Traitement commun a tous les transports
void MidiHandler::dispatch(byte type, byte channel, byte data1, byte data2) {
    switch (type) {
        case midi::NoteOn:
            // Une NoteOn de velocite 0 equivaut a une NoteOff
            if (data2 > 0) instrument.noteOn(data1, data2, channel);
            else instrument.noteOff(data1, channel);
            break;

        case midi::NoteOff:
            instrument.noteOff(data1, channel);
            break;

        case midi::ControlChange:
            handleControlChange(channel, data1, data2);
            break;

        default:
            break;
    }
}

// Gestion des Control Change MIDI
void MidiHandler::handleControlChange(byte channel, byte control, byte value) {
    (void)channel; // Volume, expression et sustain s'appliquent a tout l'instrument

    switch (control) {
        case CC_VOLUME:
            instrument.setVolume(value);
            break;

        case CC_EXPRESSION:
            instrument.setExpression(value);
            break;

        case CC_SUSTAIN:
            // Le suivi des notes retenues est fait par Instrument, qui referme
            // reellement les notes relachees au relachement de la pedale.
            instrument.setSustain(value >= 64);
            #if DEBUG
            Serial.print(F("[DEBUG] Sustain: "));
            Serial.println(value >= 64 ? "ON" : "OFF");
            #endif
            break;

        case CC_ALL_SOUND_OFF:
        case CC_ALL_NOTES_OFF:
            // Autorise dans tous les etats, y compris pendant le homing et en defaut.
            instrument.allNotesOff();
            #if DEBUG
            Serial.println(F("[DEBUG] MIDI Panic recu"));
            #endif
            break;

        default:
            break;
    }
}
