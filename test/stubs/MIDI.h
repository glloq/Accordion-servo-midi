#ifndef MIDI_H_STUB
#define MIDI_H_STUB
#include <stdint.h>
#include <Arduino.h>
namespace midi {
  enum MidiType { InvalidType=0x00, NoteOff=0x80, NoteOn=0x90, ControlChange=0xB0 };
}
#define MIDI_CHANNEL_OMNI 0
class MidiInterfaceStub {
public:
  void begin(int) {}
  void turnThruOff() {}
  bool read() { return false; }
  byte getType() { return 0; }
  byte getChannel() { return 1; }
  byte getData1() { return 0; }
  byte getData2() { return 0; }
};
#define MIDI_CREATE_INSTANCE(T, S, N) MidiInterfaceStub N
#define USBMIDI_CREATE_INSTANCE(C, N) MidiInterfaceStub N
#endif
