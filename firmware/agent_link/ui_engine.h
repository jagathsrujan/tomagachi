#pragma once
// Scheduler for the screen. Nothing is drawn unless something visible changed.
//
//   still parts   painted once when the scene changes (text, rim, icons)
//   face          redrawn only when its quantised pose changes; wakes at the next blink or
//                 glance, and runs frames only while the eyes are moving
//   waiting dots  one tile row, 3 steps a second
//   hold ring     only new pieces are drawn, 30 fps while BOOT is held
//
// All timing uses the clock passed to tick(), never frame counts, so a slow frame does
// not change how fast anything moves.
#include "ui_screens.h"

namespace ui {

constexpr uint32_t RING_DELAY_MS = 150;   // a tap never shows the ring
constexpr uint32_t RING_FRAME_MS = 33;    // 30 fps while held
constexpr uint32_t BLINK_MS = 170;
constexpr uint32_t EAR_FLICK_MS = 350, SNIFF_MS = 800;
constexpr uint32_t FIDGET_MIN_MS = 4000, FIDGET_MAX_MS = 9000;
constexpr uint32_t DOTS_STEP_MS = 320;
constexpr uint32_t NEVER = 0xFFFFFFFFu;

struct EngineStats {
  uint32_t paints = 0, faceFrames = 0, ringFrames = 0, dotFrames = 0;
  uint32_t drawUsMax = 0;
};

class Engine {
 public:
  Model m;
  Scene sc;
  Gfx g;
  EngineStats stats;

  void begin(uint16_t *fb, const Hooks &h) {
    hooks = h;
    g.begin(fb, h);
    needBuild = true;
  }

  void invalidate() { needBuild = true; }

  // Draws whatever is due into the frame buffer and marks it dirty. Never sends.
  void tick(uint32_t now) {
    const uint32_t t0 = hooks.micros();
    if (needBuild || pickScreen(m, now) != sc.id) rebuild(now);
    if (due(now, faceAt)) stepFace(now);
    if (sc.dots && due(now, dotsAt)) stepDots(now);
    if (ringDrawn || (m.pressing && sc.holdMs)) stepRing(now);
    uint32_t us = hooks.micros() - t0;
    if (us > stats.drawUsMax) stats.drawUsMax = us;
  }

  bool flush(uint32_t budgetUs) { return g.flush(budgetUs); }

  // Milliseconds until the next scheduled draw (0 = something is already pending).
  uint32_t wakeIn(uint32_t now) const {
    if (g.pending() || needBuild) return 0;
    uint32_t best = NEVER;
    consider(best, now, faceAt);
    if (sc.dots) consider(best, now, dotsAt);
    if (ringDrawn && !(m.pressing && sc.holdMs)) return 0;
    if (m.pressing && sc.holdMs) consider(best, now, ringAt);
    if (sc.id == SC_NOTICE) consider(best, now, m.noticeUntil);
    return best;
  }

 private:
  Hooks hooks{};
  bool needBuild = true;

  // face
  float gx = 0, gy = 0, tx = 0, ty = 0;
  uint32_t faceAt = 0, lastFace = 0, nextBlink = 0, nextGlance = NEVER, blinkStart = 0;
  bool blinking = false, faceDrawn = false;
  Pose drawn{};

  // Small habits from the kangaroo sketch: flick an ear or sniff, one at a time.
  enum Fidget : uint8_t { F_EAR_L, F_EAR_R, F_SNIFF, F_NONE };
  Fidget fidget = F_NONE;
  uint32_t nextFidget = NEVER, fidgetStart = 0, fidgetLen = 0;

  // dots, ring
  int8_t dotPhase = -1;
  uint32_t dotsAt = 0, ringAt = 0;
  int ringDrawn = 0;

  static bool due(uint32_t now, uint32_t at) { return at != NEVER && (int32_t)(now - at) >= 0; }
  static void consider(uint32_t &best, uint32_t now, uint32_t at) {
    if (at == NEVER) return;
    int32_t d = (int32_t)(at - now);
    uint32_t ms = d < 0 ? 0 : (uint32_t)d;
    if (ms < best) best = ms;
  }
  uint32_t rnd(uint32_t lo, uint32_t hi) { return hi <= lo ? lo : lo + hooks.random(hi - lo); }

  void rebuild(uint32_t now) {
    Screen before = sc.id;
    bool first = needBuild && stats.paints == 0;
    buildScene(sc, m, now);
    needBuild = false;
    paintScene(g, sc);
    stats.paints++;
    if (first || sc.id != before) {  // a new screen starts with a settled, fresh face
      gx = gy = tx = ty = 0;
      blinking = false;
      nextBlink = now + rnd(sc.motion.blinkMin, sc.motion.blinkMax);
      nextGlance = sc.motion.glanceMin ? now + rnd(sc.motion.glanceMin, sc.motion.glanceMax) : NEVER;
      lastFace = now;
      fidget = F_NONE;
      nextFidget = sc.motion.glanceMin ? now + rnd(FIDGET_MIN_MS, FIDGET_MAX_MS) : NEVER;
    }
    faceDrawn = false;
    faceAt = now;
    dotPhase = -1;
    dotsAt = now;
    ringDrawn = 0;
    ringAt = now;
  }

  void stepFace(uint32_t now) {
    float dt = (now - lastFace) / 1000.0f;
    lastFace = now;
    if (dt > 0.25f) dt = 0.25f;
    const Motion &mo = sc.motion;

    if (due(now, nextGlance)) {
      tx = (float)((int)rnd(0, 2 * mo.gazeX + 1) - mo.gazeX);
      ty = (float)((int)rnd(0, 2 * mo.gazeY + 1) - mo.gazeY);
      nextGlance = now + rnd(mo.glanceMin, mo.glanceMax);
    }
    if (due(now, nextBlink)) {
      blinking = true;
      blinkStart = now;
      nextBlink = now + rnd(mo.blinkMin, mo.blinkMax);
    }

    float k = 1.0f - expf(-9.0f * dt);  // same glance speed at any frame rate
    gx += (tx - gx) * k;
    gy += (ty - gy) * k;
    if (fabsf(tx - gx) < 0.6f) gx = tx;
    if (fabsf(ty - gy) < 0.6f) gy = ty;

    uint8_t open = 4;
    if (blinking) {
      uint32_t el = now - blinkStart;
      if (el >= BLINK_MS) blinking = false;
      else open = (uint8_t)(fabsf(el / (BLINK_MS / 2.0f) - 1.0f) * 4.0f + 0.5f);
    }
    if (due(now, nextFidget)) {
      fidget = (Fidget)rnd(0, 3);
      fidgetStart = now;
      fidgetLen = fidget == F_SNIFF ? SNIFF_MS : EAR_FLICK_MS;
      nextFidget = now + rnd(FIDGET_MIN_MS, FIDGET_MAX_MS);
    }
    uint8_t earL = 0, earR = 0, sniff = 0;
    if (fidget != F_NONE) {
      uint32_t el = now - fidgetStart;
      if (el >= fidgetLen) {
        fidget = F_NONE;
      } else {
        float swell = sinf((float)M_PI * el / fidgetLen);
        if (fidget == F_EAR_L) earL = (uint8_t)lroundf(swell * 3);
        else if (fidget == F_EAR_R) earR = (uint8_t)lroundf(swell * 3);
        else sniff = (uint8_t)lroundf(fabsf(sinf(el * 0.022f)) * swell * 3);
      }
    }
    Pose p = {(int8_t)lroundf(gx), (int8_t)lroundf(gy), open, earL, earR, sniff};
    if (!faceDrawn || !(p == drawn)) {
      drawFace(g, *sc.face, sc.expr, sc.eyeColor, p);
      drawn = p;
      faceDrawn = true;
      stats.faceFrames++;
    }
    bool moving = blinking || gx != tx || gy != ty || fidget != F_NONE;
    if (moving) {
      faceAt = now + mo.frameMs;
    } else {
      faceAt = nextBlink;
      if (nextGlance != NEVER && (int32_t)(nextGlance - faceAt) < 0) faceAt = nextGlance;
      if (nextFidget != NEVER && (int32_t)(nextFidget - faceAt) < 0) faceAt = nextFidget;
    }
  }

  void stepDots(uint32_t now) {
    int8_t phase = (int8_t)((now / DOTS_STEP_MS) % 3);
    dotsAt = now - now % DOTS_STEP_MS + DOTS_STEP_MS;
    if (phase == dotPhase) return;
    dotPhase = phase;
    // One tile row tall (8 px, y is a multiple of 8) and three tiles wide.
    g.fillRect(CX - 24, sc.dotsY, 48, 8, BG);
    for (int i = 0; i < 3; i++) {
      bool on = i == phase;
      g.fillCircle(CX - 16 + i * 16, sc.dotsY + 4, on ? 3 : 2, on ? sc.dotsColor : RIM_DIM);
    }
    stats.dotFrames++;
  }

  uint16_t ringBase(int i) const {
    return sc.rim.on && (i % sc.rim.period < sc.rim.on) ? sc.rim.color : BG;
  }

  void stepRing(uint32_t now) {
    const bool holding = m.pressing && sc.holdMs;
    if (holding && !due(now, ringAt)) return;  // 30 fps while held; a release is handled at once
    int want = 0;
    if (holding) {
      uint32_t held = now - m.pressedAt;
      if (held > RING_DELAY_MS) {
        uint32_t v = (held - RING_DELAY_MS) * RING_N / (sc.holdMs - RING_DELAY_MS);
        want = v > (uint32_t)RING_N ? RING_N : (int)v;
      }
    }
    if (want > ringDrawn) {
      for (int i = ringDrawn; i < want; i++) g.ringSegment(i, RING_N, RING_R0, RING_R1, sc.holdColor);
    } else if (want < ringDrawn) {
      for (int i = want; i < ringDrawn; i++) g.ringSegment(i, RING_N, RING_R0, RING_R1, ringBase(i));
    }
    if (want != ringDrawn) stats.ringFrames++;
    ringDrawn = want;
    ringAt = now + RING_FRAME_MS;
  }
};

}  // namespace ui
