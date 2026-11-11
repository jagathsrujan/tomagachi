#pragma once
// Frame buffer drawing with dirty-tile tracking. No Arduino dependency, so the same code
// runs on the board and in the host preview (preview/host.cpp).
//
// Every drawing call marks the 16x8 tiles it touches. flush() then hashes only those
// tiles, and sends only the ones whose pixels differ from what the panel already shows.
// A tile hash is 4 bytes, so no second 115 KB frame buffer is needed.
#include <math.h>
#include <stdint.h>
#include <string.h>
#include "font5x7.h"

namespace ui {

constexpr int W = 240;
constexpr int CX = W / 2;
constexpr int TILE_W = 16, TILE_H = 8;
constexpr int TILES_X = W / TILE_W, TILES_Y = W / TILE_H, TILES = TILES_X * TILES_Y;

constexpr uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

struct Hooks {
  uint32_t (*micros)();
  uint32_t (*random)(uint32_t n);                                    // 0..n-1
  void (*send)(int x, int y, int w, int h, uint16_t *pixels);        // blocking SPI write
};

struct FlushStats {
  uint32_t bytes = 0, runs = 0, tiles = 0, sendUs = 0;
};

class Gfx {
 public:
  uint16_t *fb = nullptr;
  FlushStats stats;

  void begin(uint16_t *buffer, const Hooks &h) {
    fb = buffer;
    hooks = h;
    memset(hash, 0, sizeof hash);  // 0 never matches a real hash (low bit is forced to 1)
    for (int ty = 0; ty < TILES_Y; ty++) {
      for (int tx = 0; tx < TILES_X; tx++) {
        // The glass is a circle, so tiles wholly outside it are never sent.
        float nx = fminf(fmaxf(119.5f, tx * TILE_W), tx * TILE_W + TILE_W - 1);
        float ny = fminf(fmaxf(119.5f, ty * TILE_H), ty * TILE_H + TILE_H - 1);
        float dx = nx - 119.5f, dy = ny - 119.5f;
        visible[ty * TILES_X + tx] = dx * dx + dy * dy <= 121.0f * 121.0f;
      }
    }
    memset(dirty, 0, sizeof dirty);
    dirtyCount = 0;
    for (int y = 0; y < W; y++) {
      float dy = y + 0.5f - CX;
      clipHalf[y] = dy * dy < CLIP_R * CLIP_R ? (uint8_t)sqrtf(CLIP_R * CLIP_R - dy * dy) : 0;
    }
  }

  // Limits drawing to the disc inside the rim, above y1. Used for the kangaroo and its grass.
  void setClip(int y1) { clipY1 = y1; }
  void clearClip() { clipY1 = 0; }

  bool pending() const { return dirtyCount != 0; }

  // ---- dirty tracking ----
  void mark(int x0, int y0, int x1, int y1) {  // x1, y1 exclusive, already clipped
    int tx0 = x0 / TILE_W, tx1 = (x1 - 1) / TILE_W, ty0 = y0 / TILE_H, ty1 = (y1 - 1) / TILE_H;
    for (int ty = ty0; ty <= ty1; ty++) {
      uint8_t *row = dirty + ty * TILES_X;
      for (int tx = tx0; tx <= tx1; tx++) {
        if (!row[tx]) { row[tx] = 1; dirtyCount++; }
      }
    }
  }
  void markAll() { mark(0, 0, W, W); }

  // ---- primitives ----
  void fillRaw(int x, int y, int w, int h, uint16_t c) {  // clipped, no marking
    int x1 = x + w, y1 = y + h;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x1 > W) x1 = W;
    if (y1 > W) y1 = W;
    if (x >= x1 || y >= y1) return;
    for (int yy = y; yy < y1; yy++) {
      uint16_t *p = fb + yy * W + x;
      for (int xx = x; xx < x1; xx++) *p++ = c;
    }
  }
  void fillRect(int x, int y, int w, int h, uint16_t c) {
    int x1 = x + w, y1 = y + h;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x1 > W) x1 = W;
    if (y1 > W) y1 = W;
    if (clipY1) {
      if (y1 > clipY1) y1 = clipY1;
      for (int yy = y; yy < y1; yy++) {
        int a = x > CX - clipHalf[yy] ? x : CX - clipHalf[yy];
        int b = x1 < CX + clipHalf[yy] ? x1 : CX + clipHalf[yy];
        if (a >= b) continue;
        fillRaw(a, yy, b - a, 1, c);
        mark(a, yy, b, yy + 1);
      }
      return;
    }
    if (x >= x1 || y >= y1) return;
    fillRaw(x, y, x1 - x, y1 - y, c);
    mark(x, y, x1, y1);
  }
  // Like fillRect, but only inside the circle of radius r, so clearing a box near the top
  // never wipes the rim.
  void fillRectInCircle(int x, int y, int w, int h, int r, uint16_t c) {
    for (int yy = y; yy < y + h; yy++) {
      float dy = yy + 0.5f - CX;
      if (dy * dy >= (float)r * r) continue;
      int half = (int)sqrtf((float)r * r - dy * dy);
      int x0 = x > CX - half ? x : CX - half, x1 = x + w < CX + half ? x + w : CX + half;
      if (x1 > x0) fillRect(x0, yy, x1 - x0, 1, c);
    }
  }
  void clear(uint16_t c) { fillRect(0, 0, W, W, c); }

  void fillCircle(int cx, int cy, int r, uint16_t c) {
    float rr = (r + 0.5f) * (r + 0.5f);
    for (int dy = -r; dy <= r; dy++) {
      int dx = (int)sqrtf(rr - dy * dy);
      fillRect(cx - dx, cy + dy, 2 * dx + 1, 1, c);
    }
  }

  void fillEllipse(int cx, int cy, int rx, int ry, uint16_t c) {
    for (int dy = -ry; dy <= ry; dy++) {
      float k = (float)dy / (ry + 0.5f);
      int dx = (int)(rx * sqrtf(1.0f - k * k) + 0.5f);
      fillRect(cx - dx, cy + dy, 2 * dx + 1, 1, c);
    }
  }

  void fillRoundRect(int x, int y, int w, int h, int r, uint16_t c) {
    if (r > w / 2) r = w / 2;
    if (r > h / 2) r = h / 2;
    if (r <= 0) return fillRect(x, y, w, h, c);
    for (int i = 0; i < r; i++) {
      float d = r - i - 0.5f;
      int inset = r - (int)(sqrtf((float)r * r - d * d) + 0.5f);
      fillRect(x + inset, y + i, w - 2 * inset, 1, c);
      fillRect(x + inset, y + h - 1 - i, w - 2 * inset, 1, c);
    }
    fillRect(x, y + r, w, h - 2 * r, c);
  }

  // Round-ended thick line, built from discs.
  void thickLine(int x0, int y0, int x1, int y1, int r, uint16_t c) {
    int dx = x1 - x0 < 0 ? x0 - x1 : x1 - x0, dy = y1 - y0 < 0 ? y0 - y1 : y1 - y0;
    int steps = dx > dy ? dx : dy;
    if (steps == 0) steps = 1;
    for (int i = 0; i <= steps; i++) fillCircle(x0 + (x1 - x0) * i / steps, y0 + (y1 - y0) * i / steps, r, c);
  }

  // One of n equal pieces of the ring between radii r0 and r1, clockwise from 12 o'clock.
  void ringSegment(int i, int n, int r0, int r1, uint16_t c) {
    const float a0 = i * (2 * (float)M_PI / n) - (float)M_PI / 2;
    const float a1 = (i + 1) * (2 * (float)M_PI / n) - (float)M_PI / 2;
    const float c0 = cosf(a0), s0 = sinf(a0), c1 = cosf(a1), s1 = sinf(a1);
    const float am = (a0 + a1) / 2, cm = cosf(am), sm = sinf(am);
    const float xs[6] = {r0 * c0, r1 * c0, r0 * c1, r1 * c1, r0 * cm, r1 * cm};
    const float ys[6] = {r0 * s0, r1 * s0, r0 * s1, r1 * s1, r0 * sm, r1 * sm};
    float minx = xs[0], maxx = xs[0], miny = ys[0], maxy = ys[0];
    for (int k = 1; k < 6; k++) {
      minx = fminf(minx, xs[k]); maxx = fmaxf(maxx, xs[k]);
      miny = fminf(miny, ys[k]); maxy = fmaxf(maxy, ys[k]);
    }
    int bx0 = CX + (int)floorf(minx) - 1, bx1 = CX + (int)ceilf(maxx) + 2;
    int by0 = CX + (int)floorf(miny) - 1, by1 = CX + (int)ceilf(maxy) + 2;
    if (bx0 < 0) bx0 = 0;
    if (by0 < 0) by0 = 0;
    if (bx1 > W) bx1 = W;
    if (by1 > W) by1 = W;
    const float lo = (float)r0 * r0, hi = (float)r1 * r1;
    for (int y = by0; y < by1; y++) {
      float py = y + 0.5f - CX;
      for (int x = bx0; x < bx1; x++) {
        float px = x + 0.5f - CX;
        float d2 = px * px + py * py;
        if (d2 < lo || d2 >= hi) continue;
        if (c0 * py - s0 * px < 0) continue;   // before the start edge
        if (c1 * py - s1 * px >= 0) continue;  // at or past the end edge
        fb[y * W + x] = c;
      }
    }
    mark(bx0, by0, bx1, by1);
  }

  // ---- text (built-in 5x7 font, scaled by an integer) ----
  static int textWidth(int chars, int size) { return chars * 6 * size; }

  void drawText(int x, int y, const char *s, int n, int size, uint16_t c) {
    int x0 = x < 0 ? 0 : x, y0 = y < 0 ? 0 : y;
    int x1 = x + n * 6 * size, y1 = y + 8 * size;
    if (x1 > W) x1 = W;
    if (y1 > W) y1 = W;
    if (x0 >= x1 || y0 >= y1) return;
    for (int i = 0; i < n; i++) {
      const uint8_t *glyph = FONT5X7 + (uint8_t)s[i] * 5;
      for (int col = 0; col < 5; col++) {
        uint8_t bits = glyph[col];
        int row = 0;
        while (row < 8) {
          if (!(bits & (1 << row))) { row++; continue; }
          int start = row;
          while (row < 8 && (bits & (1 << row))) row++;
          fillRaw(x + (i * 6 + col) * size, y + start * size, size, (row - start) * size, c);
        }
      }
    }
    mark(x0, y0, x1, y1);
  }

  // ---- flushing ----
  // Sends dirty tiles that really changed, top to bottom, and stops once the time budget
  // is spent so the main loop keeps polling the link and button. Returns true if more is
  // pending.
  bool flush(uint32_t budgetUs) {
    if (!dirtyCount) return false;
    const uint32_t start = hooks.micros();
    for (int ty = 0; ty < TILES_Y; ty++) {
      int tx = 0;
      while (tx < TILES_X) {
        int i = ty * TILES_X + tx;
        if (!dirty[i]) { tx++; continue; }
        int n = 0;
        uint32_t fresh[TILES_X];
        while (tx + n < TILES_X) {
          int j = i + n;
          if (!dirty[j]) break;
          if (!visible[j]) { settle(j); break; }
          uint32_t h = tileHash(tx + n, ty);
          if (h == hash[j]) { settle(j); break; }
          fresh[n++] = h;
        }
        if (n == 0) { tx++; continue; }
        int w = n * TILE_W;
        for (int r = 0; r < TILE_H; r++) {
          memcpy(runBuf + r * w, fb + (ty * TILE_H + r) * W + tx * TILE_W, w * 2);
        }
        uint32_t t0 = hooks.micros();
        hooks.send(tx * TILE_W, ty * TILE_H, w, TILE_H, runBuf);
        stats.sendUs += hooks.micros() - t0;
        stats.bytes += w * TILE_H * 2;
        stats.runs++;
        stats.tiles += n;
        for (int k = 0; k < n; k++) { hash[i + k] = fresh[k]; settle(i + k); }
        tx += n;
        if (hooks.micros() - start >= budgetUs) return dirtyCount != 0;
      }
    }
    return dirtyCount != 0;
  }

 private:
  static constexpr float CLIP_R = 105;
  int clipY1 = 0;  // 0 = no clip
  uint8_t clipHalf[W];
  Hooks hooks{};
  uint8_t dirty[TILES];
  bool visible[TILES];
  uint32_t hash[TILES];
  uint16_t dirtyCount = 0;
  uint16_t runBuf[TILES_X * TILE_W * TILE_H];

  void settle(int i) {
    if (dirty[i]) { dirty[i] = 0; dirtyCount--; }
  }

  uint32_t tileHash(int tx, int ty) const {
    uint32_t h = 2166136261u;
    for (int r = 0; r < TILE_H; r++) {
      const uint32_t *p = (const uint32_t *)(fb + (ty * TILE_H + r) * W + tx * TILE_W);
      for (int k = 0; k < TILE_W / 2; k++) {
        h = (h ^ p[k]) * 16777619u;
        h ^= h >> 15;
      }
    }
    return h | 1u;
  }
};

}  // namespace ui
