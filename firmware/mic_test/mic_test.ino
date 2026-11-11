// Mic check for the INMP441: shows a live level bar on the round display and prints the
// level over serial. Wiring: SCK=12, WS=13, SD=14, L/R=GND, VDD=3V3.
#include <Arduino_GFX_Library.h>
#include <ESP_I2S.h>
#include "pins.h"

constexpr int MIC_SCK = 12, MIC_WS = 13, MIC_SD = 14;
constexpr uint32_t SAMPLE_RATE = 16000;
constexpr int SAMPLES = 512;
constexpr uint32_t SPI_HZ = 20000000;

Arduino_DataBus *bus = new Arduino_ESP32SPI(PIN_DC, PIN_CS, PIN_SCK, PIN_MOSI, GFX_NOT_DEFINED);
Arduino_GFX *gfx = new Arduino_GC9A01(bus, PIN_RST, 0, true);
I2SClass mic;
int32_t samples[SAMPLES];
int lastBar = -1;
const char *lastStatus = "";

void showStatus(const char *text, uint16_t colour) {
  if (!strcmp(text, lastStatus)) return;
  lastStatus = text;
  gfx->fillRect(30, 70, 180, 24, RGB565_BLACK);
  gfx->setTextSize(3);
  gfx->setTextColor(colour);
  gfx->setCursor(120 - strlen(text) * 9, 72);
  gfx->print(text);
}

void showBar(int bar) {  // 0..180 px
  if (bar == lastBar) return;
  if (bar > lastBar) gfx->fillRect(30 + max(lastBar, 0), 110, bar - max(lastBar, 0), 30, RGB565_GREEN);
  else gfx->fillRect(30 + bar, 110, lastBar - bar, 30, RGB565_DARKGREY);
  lastBar = bar;
}

void setup() {
  Serial.begin(115200);
  gfx->begin(SPI_HZ);
  gfx->fillScreen(RGB565_BLACK);
  gfx->fillRect(30, 110, 180, 30, RGB565_DARKGREY);
  mic.setPins(MIC_SCK, MIC_WS, -1, MIC_SD);
  if (!mic.begin(I2S_MODE_STD, SAMPLE_RATE, I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO, I2S_STD_SLOT_BOTH)) {
    showStatus("I2S FAIL", RGB565_RED);
    while (true) { Serial.println("MIC i2s begin failed"); delay(1000); }
  }
}

// Two one-pole high-pass stages at about 200 Hz remove rumble and mains hum, keeping speech.
struct HighPass {
  static constexpr float A = 0.9244f;  // exp(-2*pi*200/16000)
  float x1 = 0, y1 = 0, x2 = 0, y2 = 0;
  float step(float x) {
    y1 = A * (y1 + x - x1);
    x1 = x;
    y2 = A * (y2 + y1 - x2);
    x2 = y1;
    return y2;
  }
} filter;

void loop() {
  static bool lastTalk = false;
  static double sumRaw = 0, sumVoice = 0;
  static uint32_t phaseN = 0;
  size_t frames = mic.readBytes((char *)samples, sizeof samples) / 8;
  if (!frames) return;

  double raw = 0, voice = 0, mean = 0;
  for (size_t i = 0; i < frames; i++) mean += samples[i * 2] >> 8;
  mean /= frames;
  for (size_t i = 0; i < frames; i++) {
    float v = samples[i * 2] >> 8;
    float f = filter.step(v);
    raw += (v - mean) * (v - mean);
    voice += (double)f * f;
  }
  raw = sqrt(raw / frames);
  voice = sqrt(voice / frames);
  showBar(constrain((int)((20 * log10(voice + 1) - 45) * 180 / 50), 0, 180));

  // Alternates so quiet and speech can be compared: 6 s each.
  bool talk = (millis() / 6000) % 2;
  showStatus(talk ? "TALK" : "QUIET", talk ? RGB565_GREEN : RGB565_WHITE);
  if (talk != lastTalk) {
    if (phaseN) {
      Serial.printf("PHASE %s voice_db=%.1f raw_db=%.1f\n", lastTalk ? "TALK" : "QUIET",
                    20 * log10(sumVoice / phaseN + 1), 20 * log10(sumRaw / phaseN + 1));
    }
    lastTalk = talk;
    sumRaw = sumVoice = 0;
    phaseN = 0;
  }
  sumRaw += raw;
  sumVoice += voice;
  phaseN++;
}
