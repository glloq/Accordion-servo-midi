#include "midiHandler.h"
#include <MIDI.h>

// Constantes MIDI Control Change
#define CC_VOLUME         7    // Volume principal
#define CC_SUSTAIN        64   // Pédale de sustain
#define CC_ALL_SOUND_OFF  120  // Arrêt de tous les sons
#define CC_ALL_NOTES_OFF  123  // Désactive toutes les notes

MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);

MidiHandler::MidiHandler(Instrument& instr)
    : instrument(instr), sustainActive(false) {}

void MidiHandler::begin() {
    MIDI.begin(MIDI_CHANNEL_OMNI); // Écoute tous les canaux MIDI

    #if DEBUG
    Serial.println(F("[DEBUG] MIDI Handler initialisé"));
    #endif
}

void MidiHandler::update() {
    processMIDI();
}

// Traite les messages MIDI reçus
void MidiHandler::processMIDI() {
    if (!MIDI.read()) return;

    byte type = MIDI.getType();
    byte channel = MIDI.getChannel();
    byte data1 = MIDI.getData1();
    byte data2 = MIDI.getData2();

    if (type == midi::ControlChange) {
        handleControlChange(data1, data2);
        return;
    }

    if (type == midi::NoteOn && data2 > 0) {
        instrument.noteOn(data1, data2, channel);
    } else if (type == midi::NoteOff || (type == midi::NoteOn && data2 == 0)) {
        // Ignore NoteOff si sustain actif (les notes restent tenues)
        if (!sustainActive) {
            instrument.noteOff(data1, channel);
        }
    }
}

// Gestion des Control Change MIDI
void MidiHandler::handleControlChange(byte control, byte value) {
    switch (control) {
        case CC_VOLUME:
            instrument.setVolume(value);
            break;

        case CC_SUSTAIN:
            sustainActive = (value >= 64);
            // Si sustain relâché, on pourrait libérer les notes tenues
            // (non implémenté ici car complexe - nécessite tracking des notes tenues)
            #if DEBUG
            Serial.print(F("[DEBUG] Sustain: "));
            Serial.println(sustainActive ? "ON" : "OFF");
            #endif
            break;

        case CC_ALL_SOUND_OFF:
        case CC_ALL_NOTES_OFF:
            instrument.allNotesOff();
            sustainActive = false;
            #if DEBUG
            Serial.println(F("[DEBUG] MIDI Panic reçu"));
            #endif
            break;
    }
}
