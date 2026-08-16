#ifndef MIDI_H_STUB
#define MIDI_H_STUB
#include <stdint.h>
#include <Arduino.h>
namespace midi {
  enum MidiType { InvalidType=0x00, NoteOff=0x80, NoteOn=0x90, ControlChange=0xB0 };
}
#define MIDI_CHANNEL_OMNI 0
// File de messages injectables par les tests, pour mesurer l'effet du trafic MIDI
// sur la generation de pas.
struct StubMidiMessage { byte type, channel, data1, data2; };
class MidiInterfaceStub {
public:
  MidiInterfaceStub() : head(0), count(0) {}
  void begin(int) {}
  void turnThruOff() {}
  bool read();
  byte getType() { return cur.type; }
  byte getChannel() { return cur.channel; }
  byte getData1() { return cur.data1; }
  byte getData2() { return cur.data2; }
  void push(byte type, byte channel, byte d1, byte d2);
  StubMidiMessage queue[64];
  StubMidiMessage cur;
  int head, count;
};
#define MIDI_CREATE_INSTANCE(T, S, N) MidiInterfaceStub N
#define USBMIDI_CREATE_INSTANCE(C, N) MidiInterfaceStub N
#endif
