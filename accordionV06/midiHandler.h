#ifndef MIDI_HANDLER_H
#define MIDI_HANDLER_H

#include <Arduino.h>
#include "instrument.h"

// Routeur MIDI multi-transports.
// Les transports actifs sont choisis dans settings.h (MIDI_TRANSPORT_DIN / _USB) ou
// depuis la ligne de compilation. Les messages des differents transports sont fusionnes
// vers le meme instrument.
//
// ORDONNANCEMENT : update() ne traite qu'UN SEUL message par appel. FlexyStepper ne
// produit qu'un pas par appel a processMovement() ; enchainer plusieurs messages MIDI,
// et donc plusieurs ecritures I2C de ~110 us, avant de rendre la main au generateur de
// pas ferait chuter la frequence de pas et decrocher le moteur.
// Un message par tour de boucle reste tres au-dessus du debit maximal du MIDI
// (~1000 messages/s) des lors que la boucle tourne a quelques dizaines de microsecondes.
class MidiHandler {
public:
    MidiHandler(Instrument& instr);

    void begin();  // Initialise les transports MIDI
    void update(); // Traite au plus un message MIDI

private:
    Instrument& instrument;
#if MIDI_TRANSPORT_DIN && MIDI_TRANSPORT_USB
    bool preferUsb; // Alternance entre transports, pour qu'aucun ne soit affame
#endif

    bool readDin();
    bool readUsb();
    void dispatch(byte type, byte channel, byte data1, byte data2);
    void handleControlChange(byte channel, byte control, byte value);
};

#endif
