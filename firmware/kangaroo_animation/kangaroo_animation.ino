// Animated kangaroo portrait for a 1.28" GC9A01 round display (240x240) on an ESP32-S3.
// Frames are drawn off-screen, then only the tiles that changed since the last frame
// are sent, because the SPI link is the bottleneck.
#include <Arduino_GFX_Library.h>
#include "pins.h"
#include "kangaroo_scene.h"

constexpr int W = roo::W;
constexpr uint32_t SPI_HZ = 20000000;  // 40 MHz is too fast for breadboard jumper wires

Arduino_DataBus *bus = new Arduino_ESP32SPI(PIN_DC, PIN_CS, PIN_SCK, PIN_MOSI, GFX_NOT_DEFINED);
Arduino_GFX *panel = new Arduino_GC9A01(bus, PIN_RST, 0, true);
Arduino_Canvas *gfx = new Arduino_Canvas(W, W, panel);

constexpr int TILE_W = 16;
constexpr int TILE_H = 8;
uint16_t *prevFrame = nullptr;
uint16_t runBuf[W * TILE_H];
uint32_t pixelsSent = 0;

bool tileChanged(const uint16_t *fb, int tx, int ty) {
  for (int y = ty; y < ty + TILE_H; y++) {
    if (memcmp(fb + y * W + tx, prevFrame + y * W + tx, TILE_W * 2)) return true;
  }
  return false;
}

// Sends each horizontal run of changed tiles as one block.
void pushChanged() {
  uint16_t *fb = gfx->getFramebuffer();
  for (int ty = 0; ty < W; ty += TILE_H) {
    int tx = 0;
    while (tx < W) {
      if (!tileChanged(fb, tx, ty)) {
        tx += TILE_W;
        continue;
      }
      int end = tx + TILE_W;
      while (end < W && tileChanged(fb, end, ty)) end += TILE_W;
      int w = end - tx;
      for (int y = 0; y < TILE_H; y++) {
        const uint16_t *row = fb + (ty + y) * W + tx;
        memcpy(runBuf + y * w, row, w * 2);
        memcpy(prevFrame + (ty + y) * W + tx, row, w * 2);
      }
      panel->draw16bitRGBBitmap(tx, ty, runBuf, w, TILE_H);
      pixelsSent += w * TILE_H;
      tx = end;
    }
  }
}

constexpr int TOUCH_PIN = 1;  // bare header pin, or a wire or foil pad attached to it
constexpr int PET_BUTTON_PIN = 0;   // the BOOT button also pets the kangaroo
constexpr float TOUCH_ON = 1.15f;   // ratio of reading to resting level
constexpr float TOUCH_OFF = 1.07f;
constexpr uint32_t TOUCH_POLL_MS = 40;
constexpr uint32_t PET_HOLD_MS = 900;  // keeps a quick tap visible

float touchBase = 0, touchNow = 0;
bool touching = false;
uint32_t pettedUntil = 0;

bool updatePetted(uint32_t now) {
  static uint32_t lastPoll = 0;
  if (now - lastPoll >= TOUCH_POLL_MS) {
    lastPoll = now;
    touchNow = touchRead(TOUCH_PIN);
    // Early readings after boot run high, so the resting level drops at once and only
    // rises slowly. A touch raises the reading, so it can't drag the level up.
    if (touchNow < touchBase) touchBase = touchNow;
    touching = touchNow > touchBase * (touching ? TOUCH_OFF : TOUCH_ON);
    if (!touching) touchBase += (touchNow - touchBase) * 0.01f;
  }
  if (touching || digitalRead(PET_BUTTON_PIN) == LOW) pettedUntil = now + PET_HOLD_MS;
  return now < pettedUntil;
}

void setup() {
  Serial.begin(115200);
  pinMode(PET_BUTTON_PIN, INPUT_PULLUP);
  for (int i = 0; i < 16; i++) {
    touchBase += touchRead(TOUCH_PIN) / 16.0f;
    delay(5);
  }
  if (!gfx->begin(SPI_HZ)) {
    while (true) {
      Serial.println("ROO display init failed");
      delay(1000);
    }
  }
  prevFrame = (uint16_t *)heap_caps_malloc(W * W * 2, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  if (prevFrame) {
    gfx->fillScreen(0);
    gfx->flush();
    memcpy(prevFrame, gfx->getFramebuffer(), W * W * 2);
  }
  Serial.printf("ROO started partial=%d\n", prevFrame != nullptr);
}

void loop() {
  static uint32_t frames = 0, lastReport = 0, lastFrame = 0;
  uint32_t now = millis();
  float dt = (now - lastFrame) / 1000.0f;
  lastFrame = now;

  bool petted = updatePetted(now);
  roo::drawScene(*gfx, now / 1000.0f, dt, petted);
  if (prevFrame) {
    pushChanged();
  } else {
    gfx->flush();
    pixelsSent += W * W;
  }

  frames++;
  if (now - lastReport >= 1000) {
    Serial.printf("ROO fps=%lu sent=%lu%% touch=%.0f base=%.0f petted=%d\n", (unsigned long)frames,
                  (unsigned long)(pixelsSent * 100 / (frames * W * W)), touchNow, touchBase, petted);
    frames = 0;
    pixelsSent = 0;
    lastReport = now;
  }
}
