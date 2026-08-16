// Implementations des stubs I2C et MIDI, separees pour rester lisibles.
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <MIDI.h>
#include "harness.h"

uint8_t TwoWire::endTransmission() {
    for (int i = 0; i < NUM_PCA_TOTAL; i++) {
        if (PCA_TAB[i] == pending && (stubMissingPcaMask & (1 << i))) return 2; // NACK
    }
    return 0;
}

uint8_t Adafruit_PWMServoDriver::setPWM(uint8_t, uint16_t, uint16_t) {
    stubI2cWrites++;
    stubMicros += STUB_I2C_WRITE_US; // Le bus I2C coute du temps CPU reel
    if (stubI2cErrors > 0) { stubI2cErrors--; return 2; }
    return 0;
}

bool MidiInterfaceStub::read() {
    if (count == 0) return false;
    cur = queue[head];
    head = (head + 1) % 64;
    count--;
    stubMicros += 15; // Cout d'analyse d'un message
    return true;
}

void MidiInterfaceStub::push(byte type, byte channel, byte d1, byte d2) {
    if (count >= 64) return;
    int tail = (head + count) % 64;
    queue[tail].type = type; queue[tail].channel = channel;
    queue[tail].data1 = d1;  queue[tail].data2 = d2;
    count++;
}
