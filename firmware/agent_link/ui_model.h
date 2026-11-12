#pragma once
// What the screen shows, as plain fixed-size data. The sketch fills this in when a message
// arrives; the renderer only reads it. No heap use, so drawing never allocates.
#include <stdint.h>
#include <string.h>

namespace ui {

enum Risk : uint8_t { RISK_LOW, RISK_MED, RISK_HIGH };
enum TaskState : uint8_t { TS_NONE, TS_IDLE, TS_WORKING, TS_NEEDS_YOU, TS_DONE, TS_FAILED };
enum NoticeKind : uint8_t { NK_NONE, NK_STARTING, NK_SENDING, NK_APPROVED, NK_DENIED, NK_LAPTOP, NK_ERROR };

constexpr int MAX_PRESETS = 8;
constexpr int MAX_APPROVALS = 4;
constexpr int LABEL_CAP = 64;      // longer preset labels are not offered on the device
constexpr int APPROVAL_TEXT_CAP = 192;
constexpr int SUMMARY_CAP = 512;   // 160 characters, up to 3 bytes each

struct Preset {
  char profile[40];
  char label[LABEL_CAP];
};

struct Approval {
  char id[640];  // T3 request ids run past 400 characters
  char nonce[24];
  char kind[24];
  char reason[64];
  char text[APPROVAL_TEXT_CAP];  // plain ASCII, shown in full; empty unless approvable
  Risk risk;
  bool approvable;               // may be answered with a hold on the device
};

struct Model {
  bool wifi = false, linked = false;
  char ssid[34] = "";

  Preset presets[MAX_PRESETS];
  uint8_t presetCount = 0, presetIndex = 0;
  bool hasDial = false;  // a thumbwheel picks the preset, so hints say "Turn" instead of "Tap"

  TaskState taskState = TS_NONE;
  bool needsLaptop = false;  // the task is waiting on something the device cannot answer
  char taskTitle[64] = "";
  char taskSummary[SUMMARY_CAP] = "";

  Approval approvals[MAX_APPROVALS];
  uint8_t approvalCount = 0;

  NoticeKind noticeKind = NK_NONE;
  char noticeTitle[24] = "";
  char noticeDetail[96] = "";
  uint32_t noticeUntil = 0;

  bool pressing = false;  // BOOT is down and its hold can still fire
  uint32_t pressedAt = 0;
};

// Copies src into dst (always terminated). Returns true if dst changed.
inline bool put(char *dst, size_t cap, const char *src) {
  if (!src) src = "";
  size_t n = strlen(src);
  if (n >= cap) n = cap - 1;
  if (strlen(dst) == n && memcmp(dst, src, n) == 0) return false;
  memcpy(dst, src, n);
  dst[n] = 0;
  return true;
}

// Turns UTF-8 into text the built-in ASCII font can draw. Returns the length written.
// "..." and common quotes keep their meaning; anything else becomes '?'.
inline size_t asciiFold(char *dst, size_t cap, const char *src) {
  size_t o = 0;
  const uint8_t *s = (const uint8_t *)src;
  while (*s && o + 1 < cap) {
    uint8_t c = *s;
    if (c >= 0x20 && c < 0x7F) { dst[o++] = c; s++; continue; }
    if (c == '\t' || c == '\n' || c == '\r') { dst[o++] = ' '; s++; continue; }
    if (c < 0x20 || c == 0x7F) { s++; continue; }
    if (c == 0xE2 && s[1] == 0x80) {
      uint8_t d = s[2];
      s += 3;
      if (d == 0xA6) { for (int k = 0; k < 3 && o + 1 < cap; k++) dst[o++] = '.'; }
      else if (d == 0x98 || d == 0x99) dst[o++] = '\'';
      else if (d == 0x9C || d == 0x9D) dst[o++] = '"';
      else if (d == 0x93 || d == 0x94) dst[o++] = '-';
      else dst[o++] = '?';
      continue;
    }
    // Any other multi-byte character: one '?' for the whole sequence.
    s++;
    while ((*s & 0xC0) == 0x80) s++;
    dst[o++] = '?';
  }
  dst[o] = 0;
  return o;
}

inline bool isPlainAscii(const char *s) {
  for (; *s; s++) {
    if ((uint8_t)*s < 0x20 || (uint8_t)*s > 0x7E) return false;
  }
  return true;
}

}  // namespace ui
