#pragma once
// Text layout for a round screen. Done once when the text changes, never per frame.
// Lines are centred and wrapped to the chord of the circle at that height, long words
// break at / - _ or hard, and a block either fits whole or says so. Nothing is cut
// silently: callers choose to shrink the font, show "...", or refuse.
#include <math.h>
#include <stdint.h>
#include "ui_gfx.h"

namespace ui {

constexpr int TEXT_R = 104;  // text stays inside this radius; the rim lives at 111..117
constexpr int MAX_LINES = 16;

struct Line {
  uint16_t start, len;
  bool dots;  // draw "..." after this line (text was cut)
};

struct Block {
  const char *text = nullptr;
  uint8_t size = 0, n = 0;
  bool fits = false, cut = false;
  int16_t y = 0, lineH = 0, dx = 0;  // dx shifts the centre line sideways
  uint16_t color = 0;
  Line line[MAX_LINES];
};

inline int lineHeight(int size) { return 8 * size + 4; }

// How many characters fit on a row whose glyphs span yTop..yTop+8*size.
inline int chordChars(int yTop, int size) {
  int top = CX - yTop, bottom = yTop + 8 * size - CX;
  int dy = top > bottom ? top : bottom;
  if (dy < 0) dy = 0;
  if (dy >= TEXT_R) return 0;
  float half = sqrtf((float)(TEXT_R * TEXT_R - dy * dy));
  return (int)(2 * half) / (6 * size);
}

inline bool isBreakAfter(char c) { return c == '/' || c == '-' || c == '_' || c == ',' || c == ':' || c == ';'; }

// Lays text out from `top`. Without `ellipsis`, text that does not reach `bottom` sets
// fits=false. With it, the last line that fits ends in "...".
inline void layoutAt(Block &b, const char *text, int size, int top, int bottom, bool ellipsis) {
  const int len = (int)strlen(text);
  b.text = text;
  b.size = size;
  b.n = 0;
  b.fits = true;
  b.cut = false;
  b.lineH = lineHeight(size);
  b.y = top;
  int y = top, pos = 0;
  while (pos < len && text[pos] == ' ') pos++;
  while (pos < len) {
    bool overflow = y + 8 * size > bottom || b.n >= MAX_LINES;
    int room = overflow ? 0 : chordChars(y, size);
    if (!overflow && room < 2) { y += b.lineH; continue; }  // too narrow near the rim
    if (overflow) {
      b.fits = false;
      if (ellipsis && b.n > 0) {
        Line &last = b.line[b.n - 1];
        int r = chordChars(b.y + (b.n - 1) * b.lineH, size) - 3;
        if (r < 1) r = 1;
        if (last.len > r) last.len = r;
        while (last.len > 1 && text[last.start + last.len - 1] == ' ') last.len--;
        last.dots = true;
        b.cut = true;
      }
      return;
    }
    int end;
    if (len - pos <= room) {
      end = len;
    } else {
      end = pos + room;
      int brk = -1;
      for (int k = end; k > pos; k--) {  // prefer a space
        if (text[k] == ' ') { brk = k; break; }
      }
      if (brk > pos) {
        end = brk;
      } else {
        for (int pass = 0; pass < 2 && end == pos + room && brk < 0; pass++) {  // then a path-like separator
          for (int k = pos + room; k > pos + 3; k--) {
            char c = text[k - 1];
            if (pass == 0 ? isBreakAfter(c) : c == '.') { end = k; brk = -2; break; }
          }
        }
      }
    }
    int l = end - pos;
    while (l > 1 && text[pos + l - 1] == ' ') l--;
    if (b.n == 0) b.y = y;
    b.line[b.n++] = {(uint16_t)pos, (uint16_t)l, false};
    pos = end;
    while (pos < len && text[pos] == ' ') pos++;
    y += b.lineH;
  }
}

// Picks the first size (largest first) at which the whole text fits between top and
// bottom. If none does: with `ellipsis` the smallest size is used and cut; without, the
// block reports fits=false. When `centre`, the block is moved to the middle of the band.
inline void fitText(Block &b, const char *text, const uint8_t *sizes, int count, int top, int bottom,
                    bool ellipsis, bool centre) {
  for (int i = 0; i < count; i++) {
    layoutAt(b, text, sizes[i], top, bottom, false);
    if (b.fits) break;
  }
  if (!b.fits && ellipsis) layoutAt(b, text, sizes[count - 1], top, bottom, true);
  if (!b.fits && !b.cut) return;
  if (!centre || b.n == 0) return;
  int used = b.n * b.lineH - 4;
  int slack = (bottom - top - used) / 2;
  if (slack <= 0) return;
  Block moved;
  for (int shift = slack; shift > 0; shift /= 2) {
    layoutAt(moved, text, b.size, top + shift, bottom, false);
    if (moved.fits && moved.n == b.n) { moved.color = b.color; moved.dx = b.dx; b = moved; return; }
  }
}

}  // namespace ui
