#pragma once
// GC9A01 wiring. Display label -> ESP32-S3 GPIO.
// VCC goes to 3V3 and GND to GND.
#define PIN_SCK  8  // SCL
#define PIN_MOSI 7  // SDA
#define PIN_CS   6  // CS
#define PIN_DC   4  // DC
#define PIN_RST  5  // RST

// Thumbwheel: 10k pot, outer legs to 3V3 and GND, middle leg (wiper) to this pin (ADC1).
// Set DIAL_ENABLED to 0 while no pot is wired: a floating pin reads noise and the
// selection would jump about. Set DIAL_REVERSED to 1 if the wheel runs backwards.
#define PIN_DIAL 1
#define DIAL_ENABLED 1
#define DIAL_REVERSED 0
