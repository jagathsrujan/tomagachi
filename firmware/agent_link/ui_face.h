#pragma once
// The companion: the kangaroo from firmware/kangaroo_animation, peeking over the text panel
// from a patch of grass. The head stays still and only its eyes, ears and nose move, so a
// blink or an ear flick sends a few tiles instead of the whole picture.
#include "ui_gfx.h"

namespace ui {

constexpr uint16_t BG = rgb(6, 8, 20);
constexpr uint16_t TEXT = rgb(238, 242, 252);
constexpr uint16_t SUB = rgb(176, 188, 214);
constexpr uint16_t TRACK = rgb(34, 44, 78);
constexpr uint16_t CYAN = rgb(90, 235, 255);
constexpr uint16_t AMBER = rgb(255, 200, 80);
constexpr uint16_t GREEN = rgb(90, 230, 130);
constexpr uint16_t RED = rgb(255, 110, 110);
constexpr uint16_t SLATE = rgb(150, 165, 205);
constexpr uint16_t WHITE = rgb(255, 255, 255);

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
constexpr uint16_t NOSE = rgb(66, 52, 52);
constexpr uint16_t NOSE_LIGHT = rgb(122, 106, 104);
constexpr uint16_t NOSTRIL = rgb(176, 96, 110);
constexpr uint16_t EYE_RIM = rgb(52, 40, 40);
constexpr uint16_t EYE = rgb(14, 14, 22);
constexpr uint16_t BLUSH = rgb(236, 150, 150);
constexpr uint16_t HEART = rgb(240, 70, 110);
constexpr uint16_t TONGUE = rgb(232, 120, 130);

enum Expr : uint8_t {
  X_NEUTRAL, X_SLEEPY, X_SEARCH, X_WIDE, X_THINK, X_WORRY, X_HAPPY, X_SAD, X_FLAT
};

struct FaceStyle {
  int16_t cx, cy;  // between the eyes
  float s;         // size against the full-screen portrait
  int16_t y1;      // the grass ends here and the text panel begins
};

// Four sizes: the big one owns the top half, the small ones leave room for text.
constexpr FaceStyle FACE_BIG = {120, 70, 0.55f, 114};
constexpr FaceStyle FACE_MED = {120, 56, 0.42f, 94};
constexpr FaceStyle FACE_SMALL = {120, 42, 0.30f, 70};
constexpr FaceStyle FACE_TINY = {120, 31, 0.20f, 50};

struct Pose {  // quantised so an unchanged pose costs nothing to draw or send
  int8_t gx, gy;
  uint8_t open;              // 0 closed .. 4 open
  uint8_t earL, earR, sniff;  // 0 .. 3
  bool operator==(const Pose &o) const {
    return gx == o.gx && gy == o.gy && open == o.open && earL == o.earL && earR == o.earR && sniff == o.sniff;
  }
};

inline float hash01(int n) {
  uint32_t h = (uint32_t)n * 2654435761u;
  h ^= h >> 15;
  h *= 2246822519u;
  h ^= h >> 13;
  return (h & 0xFFFF) / 65535.0f;
}

// Draws in the coordinates of the original 240 px portrait, scaled and moved into place.
struct Head {
  Gfx &g;
  const FaceStyle &f;
  int X(float x) const { return lroundf(f.cx + (x - 120) * f.s); }
  int Y(float y) const { return lroundf(f.cy + (y - 116) * f.s); }
  int R(float r) const { int v = lroundf(r * f.s); return v < 1 ? 1 : v; }
  void ellipse(float x, float y, float rx, float ry, uint16_t c) { g.fillEllipse(X(x), Y(y), R(rx), R(ry), c); }
  void disc(float x, float y, float r, uint16_t c) { g.fillCircle(X(x), Y(y), R(r), c); }
  // A tapered rounded stroke, used for ears and mouth.
  void limb(float x0, float y0, float x1, float y1, float r0, float r1, uint16_t c) {
    int steps = (int)(hypotf(x1 - x0, y1 - y0) * f.s / 1.5f) + 1;
    for (int i = 0; i <= steps; i++) {
      float k = i / (float)steps;
      disc(x0 + (x1 - x0) * k, y0 + (y1 - y0) * k, r0 + (r1 - r0) * k, c);
    }
  }
};

inline void drawGrass(Gfx &g, int y1) {
  g.fillRect(0, 0, W, y1, GRASS);
  for (int n = 0; n < 150; n++) {
    int y = hash01(n + 500) * W;
    if (y > y1 + 6) continue;
    g.fillEllipse(hash01(n) * W, y, 3 + hash01(n + 900) * 9, 2 + hash01(n + 1300) * 4, n % 3 ? GRASS_DARK : GRASS_LIGHT);
  }
}

inline void drawEar(Head &h, float bx, float by, float tx, float ty, float flick) {
  // Flicking swings the tip around the base.
  float dx = tx - bx, dy = ty - by, cs = cosf(flick), sn = sinf(flick);
  float ex = bx + dx * cs - dy * sn, ey = by + dx * sn + dy * cs;
  float mx = (bx + ex) / 2, my = (by + ey) / 2;
  h.limb(bx, by, mx, my, 24, 31, EAR_BACK);
  h.limb(mx, my, ex, ey, 31, 4, EAR_BACK);
  float ix = bx + (ex - bx) * 0.3f, iy = by + (ey - by) * 0.3f;
  h.limb(ix, iy, mx, my, 15, 21, EAR_INNER);
  h.limb(mx, my, bx + (ex - bx) * 0.88f, by + (ey - by) * 0.88f, 21, 2, EAR_INNER);
}

inline void drawEye(Head &h, float x, float y, float lookX, float lookY, float open, uint16_t shine) {
  float ry = 15 * open;
  if (ry < 2) {
    h.limb(x - 15, y + 1, x + 15, y + 1, 2, 2, EYE_RIM);
    return;
  }
  h.ellipse(x, y, 18, ry + 3, EYE_RIM);
  h.ellipse(x + lookX * 5, y + lookY * 3, 14, ry, EYE);
  if (open >= 0.45f) {
    h.ellipse(x + lookX * 5 + 3, y + lookY * 3 + 6, 7, 4, shine);
    h.disc(x + lookX * 5 - 5, y + lookY * 3 - 6, 4, WHITE);
    if (h.f.s > 0.35f) h.disc(x + lookX * 5 + 6, y + lookY * 3 - 3, 1, WHITE);
  }
}

inline void drawHappyEye(Head &h, float x, float y) {
  h.limb(x - 16, y + 5, x, y - 7, 3, 3, EYE_RIM);
  h.limb(x, y - 7, x + 16, y + 5, 3, 3, EYE_RIM);
}

inline void drawHeart(Head &h, float x, float y, float s) {
  h.disc(x - s * 0.5f, y, s * 0.6f, HEART);
  h.disc(x + s * 0.5f, y, s * 0.6f, HEART);
  h.limb(x - s * 0.55f, y + s * 0.25f, x, y + s * 1.2f, s * 0.5f, 1, HEART);
  h.limb(x + s * 0.55f, y + s * 0.25f, x, y + s * 1.2f, s * 0.5f, 1, HEART);
}

// `shine` is the state colour, reflected in the eyes.
inline void drawFace(Gfx &g, const FaceStyle &f, Expr e, uint16_t shine, Pose p) {
  g.setClip(f.y1);
  drawGrass(g, f.y1);
  Head h{g, f};

  // Ears carry the mood: pricked when alert, swung out and down when low.
  float mood = 0;
  switch (e) {
    case X_WIDE: case X_SEARCH: mood = 0.15f; break;
    case X_SLEEPY: case X_FLAT: mood = -0.35f; break;
    case X_WORRY: mood = -0.5f; break;
    case X_SAD: mood = -0.75f; break;
    default: break;
  }
  drawEar(h, 76, 74, 44, 12, mood + p.earL * 0.13f);
  drawEar(h, 164, 74, 214, 30, -mood - p.earR * 0.15f);

  // Head, cheeks and jaw.
  h.ellipse(120, 108, 72, 60, FUR);
  h.ellipse(120, 160, 54, 46, FUR);
  h.ellipse(120, 80, 34, 22, FUR_SOFT);
  h.ellipse(72, 142, 16, 22, FUR_SOFT);
  h.ellipse(168, 142, 16, 22, FUR_SOFT);

  // Soft fur speckle, only where it is large enough to read.
  if (f.s >= 0.4f) {
    for (int n = 0; n < 70; n++) {
      float a = hash01(n + 40) * 6.283f, r = sqrtf(hash01(n + 240));
      g.fillRect(h.X(120 + cosf(a) * r * 66), h.Y(104 + sinf(a) * r * 52), 1, 1, n & 1 ? FUR_DARK : FUR_LIGHT);
    }
  }

  // Muzzle.
  h.ellipse(98, 172, 24, 19, MUZZLE);
  h.ellipse(142, 172, 24, 19, MUZZLE);
  h.ellipse(120, 184, 26, 14, MUZZLE);

  // Nose, lifted a little while sniffing.
  float sniff = p.sniff / 3.0f, ny = 150 - sniff * 3;
  h.ellipse(120, ny, 25 + sniff, 15, NOSE);
  h.limb(120, ny + 4, 120, ny + 19, 11, 2, NOSE);
  h.ellipse(120, ny - 9, 12, 3, NOSE_LIGHT);
  h.ellipse(106, ny + 1, 7, 5 + sniff, NOSTRIL);
  h.ellipse(134, ny + 1, 7, 5 + sniff, NOSTRIL);

  // Mouth: the corners rise for a smile and fall for a frown.
  const float corner = e == X_HAPPY ? 175 : e == X_SAD ? 192 : e == X_WORRY ? 189 : e == X_FLAT ? 185 : 181;
  const float width = e == X_HAPPY ? 22 : 16;
  h.limb(120, 170, 120, 184, 1, 1, NOSE);
  h.limb(120, 185, 120 - width, corner, 1, 1, NOSE);
  h.limb(120, 185, 120 + width, corner, 1, 1, NOSE);
  if (e == X_HAPPY) {
    h.ellipse(120, 191, 10, 6, NOSE);
    h.ellipse(120, 194, 7, 3, TONGUE);
    h.ellipse(68, 150, 13, 8, BLUSH);
    h.ellipse(172, 150, 13, 8, BLUSH);
  } else if (e == X_WIDE) {
    h.ellipse(120, 192, 8, 7, NOSE);
  }

  // Eyes.
  if (e == X_HAPPY) {
    drawHappyEye(h, 78, 116);
    drawHappyEye(h, 162, 116);
    if (f.s >= 0.5f) {
      drawHeart(h, 16, 72, 12);
      drawHeart(h, 224, 92, 10);
    }
  } else {
    float lid = e == X_SLEEPY ? 0.5f : e == X_FLAT ? 0.6f : e == X_SAD ? 0.85f : e == X_WIDE ? 1.15f : 1.0f;
    float open = lid * p.open / 4.0f;
    float lookX = fmaxf(-1.5f, fminf(1.5f, p.gx / 10.0f)), lookY = fmaxf(-1.0f, fminf(1.0f, p.gy / 6.0f));
    if (e == X_THINK) lookY -= 0.6f;
    float y = 116 + (lid < 1 ? (1 - lid) * 8 : 0);
    drawEye(h, 78, y, lookX, lookY, open, shine);
    drawEye(h, 162, y, lookX, lookY, open, shine);
  }

  // Brows slope up toward the nose when worried or sad.
  if (e == X_WORRY || e == X_SAD) {
    float rise = e == X_SAD ? 16 : 11;
    h.limb(56, 92, 96, 92 - rise, 4, 4, EYE_RIM);
    h.limb(184, 92, 144, 92 - rise, 4, 4, EYE_RIM);
  }
  g.clearClip();
}

}  // namespace ui
