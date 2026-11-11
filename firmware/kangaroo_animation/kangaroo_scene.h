#pragma once
// Close-up kangaroo portrait: blinks, glances, flicks its ears and sniffs.
// When petted it closes its eyes happily, blushes, smiles and floats hearts.
// Only needs fillRect, fillCircle and fillEllipse, so the same code runs on the
// device and in a desktop preview.
#include <math.h>
#include <stdint.h>

namespace roo {

constexpr int W = 240;

constexpr uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

constexpr uint16_t GRASS = rgb(84, 138, 58);
constexpr uint16_t GRASS_DARK = rgb(58, 108, 46);
constexpr uint16_t GRASS_LIGHT = rgb(124, 172, 76);
constexpr uint16_t FUR = rgb(164, 150, 140);
constexpr uint16_t FUR_LIGHT = rgb(192, 180, 170);
constexpr uint16_t FUR_SOFT = rgb(174, 161, 151);
constexpr uint16_t FUR_DARK = rgb(116, 100, 92);
constexpr uint16_t EAR_BACK = rgb(120, 96, 84);
constexpr uint16_t EAR_INNER = rgb(232, 218, 208);
constexpr uint16_t MUZZLE = rgb(240, 234, 226);
constexpr uint16_t CHEST = rgb(232, 224, 214);
constexpr uint16_t NOSE = rgb(66, 52, 52);
constexpr uint16_t NOSE_LIGHT = rgb(122, 106, 104);
constexpr uint16_t NOSTRIL = rgb(176, 96, 110);
constexpr uint16_t EYE_RIM = rgb(52, 40, 40);
constexpr uint16_t EYE = rgb(14, 14, 22);
constexpr uint16_t EYE_SHINE = rgb(96, 150, 220);
constexpr uint16_t WHITE_ = rgb(255, 255, 255);
constexpr uint16_t BLUSH = rgb(236, 150, 150);
constexpr uint16_t HEART = rgb(240, 70, 110);
constexpr uint16_t TONGUE = rgb(232, 120, 130);

inline float mix(float a, float b, float k) { return a + (b - a) * k; }

inline float hash01(int n) {
  uint32_t h = (uint32_t)n * 2654435761u;
  h ^= h >> 15;
  h *= 2246822519u;
  h ^= h >> 13;
  return (h & 0xFFFF) / 65535.0f;
}

// 0 outside a burst, rising to 1 and back during the first `len` seconds of each period.
inline float burst(float t, float period, float len) {
  float k = fmodf(t, period) / len;
  return k < 1 ? sinf(k * (float)M_PI) : 0;
}

// A tapered rounded stroke, used for ears and mouth.
template <class G>
void limb(G &g, float x0, float y0, float x1, float y1, float r0, float r1, uint16_t c) {
  int steps = (int)(hypotf(x1 - x0, y1 - y0) / 1.5f) + 1;
  for (int i = 0; i <= steps; i++) {
    float k = i / (float)steps;
    g.fillCircle(lroundf(mix(x0, x1, k)), lroundf(mix(y0, y1, k)), lroundf(mix(r0, r1, k)), c);
  }
}

template <class G>
void drawGrass(G &g) {
  g.fillRect(0, 0, W, W, GRASS);
  for (int n = 0; n < 150; n++) {
    int x = hash01(n) * W, y = hash01(n + 500) * W;
    g.fillEllipse(x, y, 3 + hash01(n + 900) * 9, 2 + hash01(n + 1300) * 4, n % 3 ? GRASS_DARK : GRASS_LIGHT);
  }
}

template <class G>
void drawEar(G &g, float bx, float by, float tx, float ty, float flick) {
  // Flicking swings the tip around the base.
  float dx = tx - bx, dy = ty - by, cs = cosf(flick), sn = sinf(flick);
  float ex = bx + dx * cs - dy * sn, ey = by + dx * sn + dy * cs;
  float mx = mix(bx, ex, 0.5f), my = mix(by, ey, 0.5f);
  limb(g, bx, by, mx, my, 24, 31, EAR_BACK);
  limb(g, mx, my, ex, ey, 31, 4, EAR_BACK);
  float ix = mix(bx, ex, 0.3f), iy = mix(by, ey, 0.3f);
  limb(g, ix, iy, mx, my, 15, 21, EAR_INNER);
  limb(g, mx, my, mix(bx, ex, 0.88f), mix(by, ey, 0.88f), 21, 2, EAR_INNER);
}

template <class G>
void drawEye(G &g, float x, float y, float lookX, float lookY, float open) {
  int ry = 15 * open;
  if (ry < 2) {
    limb(g, x - 15, y + 1, x + 15, y + 1, 2, 2, EYE_RIM);
    return;
  }
  g.fillEllipse(x, y, 18, ry + 3, EYE_RIM);
  g.fillEllipse(x + lookX * 3, y + lookY * 2, 14, ry, EYE);
  if (open > 0.5f) {
    g.fillEllipse(x + lookX * 3 + 3, y + lookY * 2 + 6, 7, 4, EYE_SHINE);
    g.fillCircle(x + lookX * 3 - 5, y + lookY * 2 - 6, 4, WHITE_);
    g.fillCircle(x + lookX * 3 + 6, y + lookY * 2 - 3, 1, WHITE_);
  }
}

template <class G>
void drawHappyEye(G &g, float x, float y) {
  limb(g, x - 16, y + 5, x, y - 7, 3, 3, EYE_RIM);
  limb(g, x, y - 7, x + 16, y + 5, 3, 3, EYE_RIM);
}

template <class G>
void drawHeart(G &g, float x, float y, float s) {
  g.fillCircle(x - s * 0.5f, y, s * 0.6f, HEART);
  g.fillCircle(x + s * 0.5f, y, s * 0.6f, HEART);
  limb(g, x - s * 0.55f, y + s * 0.25f, x, y + s * 1.2f, s * 0.5f, 1, HEART);
  limb(g, x + s * 0.55f, y + s * 0.25f, x, y + s * 1.2f, s * 0.5f, 1, HEART);
}

struct Heart {
  float x, y, age;
  bool alive;
};

// Spawns hearts while happy and floats them up with a sideways drift.
template <class G>
void drawHearts(G &g, float t, float dt, float joy) {
  constexpr float LIFE = 1.6f;
  static Heart hearts[8] = {};
  static float nextSpawn = 0;
  static int spawned = 0;
  if (joy > 0.4f && t >= nextSpawn) {
    for (Heart &h : hearts) {
      if (h.alive) continue;
      h = {40 + hash01(spawned) * 160, 120 + hash01(spawned + 300) * 40, 0, true};
      spawned++;
      nextSpawn = t + 0.28f;
      break;
    }
  }
  for (Heart &h : hearts) {
    if (!h.alive) continue;
    h.age += dt;
    h.y -= 58 * dt;
    if (h.age > LIFE) {
      h.alive = false;
      continue;
    }
    float k = h.age / LIFE;
    float size = 9 * fminf(k * 5, 1) * (k > 0.8f ? (1 - k) * 5 : 1);
    if (size >= 2) drawHeart(g, h.x + sinf(h.age * 5 + h.x) * 7, h.y, size);
  }
}

template <class G>
void drawScene(G &g, float t, float dt, bool petted) {
  static float lookX = 0, lookY = 0, joy = 0;
  joy += ((petted ? 1.0f : 0.0f) - joy) * (1 - expf(-(petted ? 9.0f : 2.5f) * dt));

  // Glance somewhere new every so often, often straight back at the viewer.
  int slot = (int)(t / 1.7f);
  float targetX = 0, targetY = 0;
  if (hash01(slot + 5) > 0.35f && joy < 0.3f) {
    targetX = hash01(slot) * 2 - 1;
    targetY = hash01(slot + 77) * 1.4f - 0.7f;
  }
  float ease = 1 - expf(-7 * dt);
  lookX += (targetX - lookX) * ease;
  lookY += (targetY - lookY) * ease;

  float tilt = sinf(t * 0.7f) * 0.05f + lookX * 0.05f + sinf(t * 5) * 0.08f * joy;
  float offX = lookX * 7, offY = sinf(t * 1.6f) * 2 + lookY * 3 + sinf(t * 10) * 3 * joy;
  float cs = cosf(tilt), sn = sinf(tilt);
  auto X = [&](float x, float y) { return 120 + (x - 120) * cs - (y - 210) * sn + offX; };
  auto Y = [&](float x, float y) { return 210 + (x - 120) * sn + (y - 210) * cs + offY; };

  float sniff = fabsf(sinf(t * 22)) * burst(t, 5.3f, 0.8f);
  float blink = burst(t, 3.1f, 0.2f) + burst(t + 0.34f, 9.3f, 0.2f);
  float open = 1 - fminf(blink, 1.0f);
  float wiggle = sinf(t * 13) * 0.14f * joy;
  float flickL = burst(t, 4.3f, 0.35f) * 0.4f + wiggle;
  float flickR = -burst(t + 1.9f, 6.1f, 0.35f) * 0.45f - wiggle;

  drawGrass(g);

  // Shoulders and chest stay mostly still under the head.
  g.fillEllipse(120 + offX * 0.3f, 250, 92, 66, FUR_DARK);
  g.fillEllipse(120 + offX * 0.4f, 256, 56, 58, CHEST);

  drawEar(g, X(76, 74), Y(76, 74), X(44, 12), Y(44, 12), flickL);
  drawEar(g, X(164, 74), Y(164, 74), X(214, 30), Y(214, 30), flickR);

  // Head, cheeks and jaw.
  g.fillEllipse(X(120, 108), Y(120, 108), 72, 60, FUR);
  g.fillEllipse(X(120, 160), Y(120, 160), 54, 46, FUR);
  g.fillEllipse(X(120, 80), Y(120, 80), 34, 22, FUR_SOFT);
  g.fillEllipse(X(72, 142), Y(72, 142), 16, 22, FUR_SOFT);
  g.fillEllipse(X(168, 142), Y(168, 142), 16, 22, FUR_SOFT);

  // Soft fur speckle.
  for (int n = 0; n < 70; n++) {
    float a = hash01(n + 40) * 6.283f, r = sqrtf(hash01(n + 240));
    float x = 120 + cosf(a) * r * 66, y = 104 + sinf(a) * r * 52;
    g.fillCircle(X(x, y), Y(x, y), 1, n & 1 ? FUR_DARK : FUR_LIGHT);
  }

  // Muzzle.
  g.fillEllipse(X(98, 172), Y(98, 172), 24, 19, MUZZLE);
  g.fillEllipse(X(142, 172), Y(142, 172), 24, 19, MUZZLE);
  g.fillEllipse(X(120, 184), Y(120, 184), 26, 14, MUZZLE);

  // Nose, lifted a little while sniffing.
  float ny = 150 - sniff * 3;
  g.fillEllipse(X(120, ny), Y(120, ny), 25 + sniff, 15, NOSE);
  limb(g, X(120, ny + 4), Y(120, ny + 4), X(120, ny + 19), Y(120, ny + 19), 11, 2, NOSE);
  g.fillEllipse(X(120, ny - 9), Y(120, ny - 9), 12, 3, NOSE_LIGHT);
  g.fillEllipse(X(106, ny + 1), Y(106, ny + 1), 7, 5 + sniff, NOSTRIL);
  g.fillEllipse(X(134, ny + 1), Y(134, ny + 1), 7, 5 + sniff, NOSTRIL);

  // Mouth.
  limb(g, X(120, 170), Y(120, 170), X(120, 184), Y(120, 184), 1, 1, NOSE);
  float smile = 181 - joy * 6;
  limb(g, X(120, 185), Y(120, 185), X(104 - joy * 6, smile), Y(104 - joy * 6, smile), 1, 1, NOSE);
  limb(g, X(120, 185), Y(120, 185), X(136 + joy * 6, smile), Y(136 + joy * 6, smile), 1, 1, NOSE);
  if (joy > 0.3f) {
    g.fillEllipse(X(120, 191), Y(120, 191), 10, 6 * joy, NOSE);
    g.fillEllipse(X(120, 194), Y(120, 194), 7, 3 * joy, TONGUE);
    g.fillEllipse(X(68, 150), Y(68, 150), 13, 8, BLUSH);
    g.fillEllipse(X(172, 150), Y(172, 150), 13, 8, BLUSH);
  }

  if (joy > 0.5f) {
    drawHappyEye(g, X(78, 116), Y(78, 116));
    drawHappyEye(g, X(162, 116), Y(162, 116));
  } else {
    drawEye(g, X(78, 116), Y(78, 116), lookX, lookY, open);
    drawEye(g, X(162, 116), Y(162, 116), lookX, lookY, open);
  }

  drawHearts(g, t, dt, joy);
}

}  // namespace roo
