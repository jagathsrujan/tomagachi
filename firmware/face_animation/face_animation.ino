// Animated companion face for a 1.28" GC9A01 round display (240x240) on an ESP32-S3.
// Frames are drawn off-screen, then only the tiles that changed since the last frame
// are sent, because the SPI link is the bottleneck.
#include <Arduino_GFX_Library.h>
#include "pins.h"

constexpr int W = 240;
constexpr int CX = W / 2;
constexpr int CY = W / 2;
constexpr uint32_t SPI_HZ = 20000000;  // 40 MHz is too fast for breadboard jumper wires

Arduino_DataBus *bus = new Arduino_ESP32SPI(PIN_DC, PIN_CS, PIN_SCK, PIN_MOSI, GFX_NOT_DEFINED);
Arduino_GFX *panel = new Arduino_GC9A01(bus, PIN_RST, 0, true);
Arduino_Canvas *gfx = new Arduino_Canvas(W, W, panel);

constexpr uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

constexpr uint16_t BG = rgb(6, 8, 20);
constexpr uint16_t EYE = rgb(90, 235, 255);
constexpr uint16_t MOUTH = rgb(255, 150, 190);

uint16_t hue(float h) {
  h = fmodf(h, 1.0f) * 6.0f;
  float f = h - floorf(h);
  uint8_t up = f * 255, down = (1 - f) * 255;
  switch ((int)h) {
    case 0: return rgb(255, up, 0);
    case 1: return rgb(down, 255, 0);
    case 2: return rgb(0, 255, up);
    case 3: return rgb(0, down, 255);
    case 4: return rgb(up, 0, 255);
    default: return rgb(255, 0, down);
  }
}

float lookX = 0, lookY = 0, targetX = 0, targetY = 0;
uint32_t nextLook = 0, nextBlink = 1500, blinkStart = 0;
constexpr uint32_t BLINK_MS = 180;

void drawRing(float t) {
  for (int i = 0; i < 24; i++) {
    float a = i * (TWO_PI / 24) + t * 0.6f;
    int r = 3 + (int)(2 * (1 + sinf(t * 3 + i * 0.5f)) / 2);
    gfx->fillCircle(CX + cosf(a) * 110, CY + sinf(a) * 110, r, hue(i / 24.0f + t * 0.08f));
  }
}

void drawEye(int x, int y, float open) {
  int h = max(6, (int)(64 * open));
  gfx->fillRoundRect(x - 20, y - h / 2, 40, h, min(18, h / 2), EYE);
  if (open > 0.5f) gfx->fillCircle(x + 8, y - h / 4, 6, rgb(255, 255, 255));  // catchlight
}

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

void drawFace(float t, uint32_t now, float dt) {
  if (now >= nextLook) {
    targetX = random(-22, 23);
    targetY = random(-12, 13);
    nextLook = now + random(900, 2600);
  }
  float ease = 1 - expf(-2.3f * dt);  // same glance speed at any frame rate
  lookX += (targetX - lookX) * ease;
  lookY += (targetY - lookY) * ease;

  if (now >= nextBlink) {
    blinkStart = now;
    nextBlink = now + random(2000, 5000);
  }
  float open = 1;
  uint32_t since = now - blinkStart;
  if (blinkStart && since < BLINK_MS) open = fabsf(since / (float)(BLINK_MS / 2) - 1);

  int bob = sinf(t * 1.6f) * 4;
  int ex = CX + lookX, ey = CY - 18 + lookY + bob;
  drawEye(ex - 42, ey, open);
  drawEye(ex + 42, ey, open);

  int mx = CX + lookX * 0.6f, my = CY + 34 + lookY * 0.6f + bob;
  gfx->fillArc(mx, my, 26, 20, 25, 155, MOUTH);
}

void setup() {
  Serial.begin(115200);
  randomSeed(esp_random());
  if (!gfx->begin(SPI_HZ)) {
    while (true) {
      Serial.println("FACE display init failed");
      delay(1000);
    }
  }
  prevFrame = (uint16_t *)heap_caps_malloc(W * W * 2, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  if (prevFrame) {
    gfx->fillScreen(BG);
    gfx->flush();
    memcpy(prevFrame, gfx->getFramebuffer(), W * W * 2);
  }
  Serial.printf("FACE started partial=%d\n", prevFrame != nullptr);
}

void loop() {
  static uint32_t frames = 0, lastReport = 0, lastFrame = 0;
  uint32_t now = millis();
  float t = now / 1000.0f;
  float dt = (now - lastFrame) / 1000.0f;
  lastFrame = now;

  gfx->fillScreen(BG);
  drawRing(t);
  drawFace(t, now, dt);
  if (prevFrame) {
    pushChanged();
  } else {
    gfx->flush();
    pixelsSent += W * W;
  }

  frames++;
  if (now - lastReport >= 1000) {
    Serial.printf("FACE fps=%lu sent=%lu%% heap=%lu\n", (unsigned long)frames,
                  (unsigned long)(pixelsSent * 100 / (frames * W * W)), (unsigned long)ESP.getFreeHeap());
    frames = 0;
    pixelsSent = 0;
    lastReport = now;
  }
}
