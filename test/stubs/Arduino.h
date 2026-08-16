#ifndef ARDUINO_H_STUB
#define ARDUINO_H_STUB
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
typedef uint8_t byte;
#define HIGH 1
#define LOW 0
#define OUTPUT 1
#define INPUT_PULLUP 2
#ifndef NULL
#define NULL 0
#endif
#define F(x) (x)
#define constrain(x,a,b) ((x)<(a)?(a):((x)>(b)?(b):(x)))
unsigned long millis();
unsigned long micros();
void delay(unsigned long);
void pinMode(uint8_t, uint8_t);
void digitalWrite(uint8_t, uint8_t);
int digitalRead(uint8_t);
long map(long x, long a, long b, long c, long d);
struct SerialStub {
  void begin(unsigned long) {}
  void print(const char*) {}
  void print(int) {}
  void print(unsigned int) {}
  void print(long) {}
  void println(const char*) {}
  void println(int) {}
  void println(unsigned int) {}
  void println(long) {}
};
typedef SerialStub HardwareSerial;
extern SerialStub Serial;
extern SerialStub Serial1;
#endif
