#ifndef MIDI_HANDLER_H
#define MIDI_HANDLER_H

#include <Arduino.h>
#include "instrument.h"

// Nombre maximum de messages MIDI traites par tour de boucle.
// Borne le temps passe dans update() : FlexyStepper a besoin d'etre rappele tres
// regulierement pour generer les pas sans a-coups.
#define MIDI_MAX_MESSAGES_PER_LOOP 8

// Routeur MIDI multi-transports.
// Les transports actifs sont choisis dans settings.h (MIDI_TRANSPORT_DIN / _USB) ou
// depuis la ligne de compilation. Les messages des differents transports sont fusionnes
// vers le meme instrument.
class MidiHandler {
public:
    MidiHandler(Instrument& instr);

    void begin();  // Initialise les transports MIDI
    void update(); // Verifie les messages MIDI

private:
    Instrument& instrument;

    void processMIDI(); // Interne, lit les transports
    void dispatch(byte type, byte channel, byte data1, byte data2);
    void handleControlChange(byte channel, byte control, byte value);
};

#endif
