#pragma once
// Turns the model into a Scene (what to show and how it moves) and paints the still
// parts of it. Every screen is the same character: a face that shows the state, a short
// plain title, big text, and a hint of what tap and hold do. Colour is never the only
// signal: each state also has its own face, its own words and its own rim pattern.
#include <stdio.h>
#include "ui_face.h"
#include "ui_layout.h"
#include "ui_model.h"

namespace ui {

constexpr uint16_t START_HOLD_MS = 800;
constexpr uint16_t APPROVE_HOLD_MS = 1500;
constexpr uint32_t NOTICE_MS = 3000;

constexpr int RING_N = 60, RING_R0 = 110, RING_R1 = 117;
constexpr uint16_t RIM_DIM = rgb(70, 84, 128);

enum Screen : uint8_t {
  SC_WIFI, SC_OFFLINE, SC_HOME, SC_WORKING, SC_NEEDS_YOU, SC_DONE, SC_FAILED,
  SC_APPROVAL, SC_LAPTOP, SC_NOTICE
};

struct Motion {
  uint16_t blinkMin, blinkMax;
  uint16_t glanceMin, glanceMax;  // 0 = never look around
  uint8_t gazeX, gazeY;
  uint8_t frameMs;                // frame interval while the face is moving
};
// Idle is mostly stillness: blink every few seconds, glance now and then. Frames are only
// produced while an eyelid or the gaze is actually moving.
constexpr Motion MOTION_IDLE = {3000, 7000, 6000, 14000, 12, 6, 40};     // 25 fps in bursts
constexpr Motion MOTION_SLEEPY = {3500, 7000, 2500, 4500, 14, 3, 40};
constexpr Motion MOTION_SEARCH = {3000, 6000, 1000, 2000, 22, 4, 40};
constexpr Motion MOTION_WORKING = {3500, 7000, 500, 1300, 14, 8, 66};    // 15 fps in bursts
constexpr Motion MOTION_ALERT = {2500, 6000, 0, 0, 0, 0, 40};            // blink only

struct Rim {
  uint8_t period, on;  // of RING_N pieces, `on` in every `period` are drawn; on == 0 is none
  uint16_t color;
};

struct Scene {
  Screen id = SC_HOME;
  const FaceStyle *face = &FACE_BIG;
  Expr expr = X_NEUTRAL;
  uint16_t eyeColor = CYAN;
  Motion motion = MOTION_IDLE;
  Rim rim = {1, 0, 0};

  Block tb[6];
  uint8_t ntb = 0;

  bool pager = false;
  uint8_t pagerN = 0, pagerI = 0;
  int16_t pagerY = 0;

  bool pips = false;  // risk bars, drawn left of block `pipBlock`
  uint8_t pipLevel = 0, pipBlock = 0;
  uint16_t pipColor = 0;

  uint8_t icon = 0;  // 1 check, 2 cross
  int16_t iconY = 0;
  uint16_t iconColor = 0;

  bool dots = false;  // "waiting" dots
  int16_t dotsY = 0;
  uint16_t dotsColor = 0;

  uint16_t holdMs = 0;  // 0 = holding does nothing here, so no ring
  uint16_t holdColor = 0;

  char scratch[1100];
  uint16_t used = 0;
};

inline bool noticeActive(const Model &m, uint32_t now) {
  return m.noticeKind != NK_NONE && (int32_t)(now - m.noticeUntil) < 0;
}

inline Screen pickScreen(const Model &m, uint32_t now) {
  if (!m.wifi) return SC_WIFI;
  if (!m.linked) return SC_OFFLINE;
  if (m.approvalCount) return m.approvals[0].approvable ? SC_APPROVAL : SC_LAPTOP;
  if (noticeActive(m, now)) return SC_NOTICE;
  switch (m.taskState) {
    case TS_WORKING: return SC_WORKING;
    case TS_NEEDS_YOU: return SC_NEEDS_YOU;
    case TS_DONE: return SC_DONE;
    case TS_FAILED: return SC_FAILED;
    default: return SC_HOME;
  }
}

// ---- approval text region, shared with the sketch so "fits" means the same thing ----
constexpr int APPROVAL_TOP = 94, APPROVAL_BOTTOM = 178;
constexpr uint8_t SZ_3[] = {3}, SZ_321[] = {3, 2, 1}, SZ_32[] = {3, 2}, SZ_21[] = {2, 1}, SZ_2[] = {2}, SZ_1[] = {1};

// True if this request text can be shown whole, in the largest font that holds it.
inline bool approvalTextFits(const char *text) {
  Block b;
  fitText(b, text, SZ_21, 2, APPROVAL_TOP, APPROVAL_BOTTOM, false, false);
  return b.fits && !b.cut && b.n > 0;
}

// ---- building ----
inline const char *keep(Scene &s, const char *raw) {  // ASCII copy that outlives the call
  size_t room = sizeof s.scratch - s.used;
  if (room < 2) return "";
  char *dst = s.scratch + s.used;
  size_t n = asciiFold(dst, room, raw ? raw : "");
  s.used += n + 1;
  return dst;
}

inline Block &addText(Scene &s, const char *text, const uint8_t *sizes, int ns, int top, int bottom,
                      uint16_t color, bool ellipsis = false, bool centre = true) {
  Block &b = s.tb[s.ntb < 6 ? s.ntb++ : 5];
  b = Block();
  fitText(b, text, sizes, ns, top, bottom, ellipsis, centre);
  b.color = color;
  return b;
}

inline void setLook(Scene &s, const FaceStyle *f, Expr e, uint16_t eye, const Motion &mo) {
  s.face = f;
  s.expr = e;
  s.eyeColor = eye;
  s.motion = mo;
}

inline const char *kindWord(const char *kind) {
  if (!strcmp(kind, "command")) return "COMMAND";
  if (!strcmp(kind, "file-change")) return "EDIT FILE";
  if (!strcmp(kind, "file-read")) return "READ FILE";
  return nullptr;
}

inline const char *riskWord(Risk r) { return r == RISK_LOW ? "LOW RISK" : r == RISK_MED ? "MEDIUM RISK" : "HIGH RISK"; }
inline uint16_t riskColor(Risk r) { return r == RISK_LOW ? GREEN : r == RISK_MED ? AMBER : RED; }

// Kind and risk always go with a request, in words and in bars as well as colour.
inline void addKindAndRisk(Scene &s, const Approval &a) {
  const char *word = kindWord(a.kind);
  char buf[28];
  if (!word) {
    size_t n = asciiFold(buf, sizeof buf, a.kind);
    for (size_t i = 0; i < n; i++) if (buf[i] >= 'a' && buf[i] <= 'z') buf[i] -= 32;
    word = n ? buf : "REQUEST";
  }
  uint16_t c = riskColor(a.risk);
  addText(s, keep(s, word), SZ_21, 2, 52, 68, c, false, false);
  Block &r = addText(s, riskWord(a.risk), SZ_2, 1, 72, 88, c, false, false);
  r.dx = 18;  // leaves room for the bars on its left
  s.pips = true;
  s.pipBlock = s.ntb - 1;
  s.pipLevel = a.risk == RISK_LOW ? 1 : a.risk == RISK_MED ? 2 : 3;
  s.pipColor = c;
}

inline void addQueueHint(Scene &s, const Model &m, const char *tail, int y) {
  char buf[40];
  if (m.approvalCount > 1) snprintf(buf, sizeof buf, "%d/%d  %s", 1, m.approvalCount, tail);
  else snprintf(buf, sizeof buf, "%s", tail);
  addText(s, keep(s, buf), SZ_1, 1, y, y + 8, SUB, false, false);
}

inline void addCount(Scene &s, const Model &m) {
  char buf[24];
  snprintf(buf, sizeof buf, "1 of %d waiting", m.approvalCount);
  addText(s, keep(s, buf), SZ_1, 1, 198, 206, SUB, false, false);
}

inline void addFooter(Scene &s, const Model &m, int yLabel, int yHint) {
  if (!m.presetCount) return;
  char buf[LABEL_CAP + 4];
  snprintf(buf, sizeof buf, "> %s", m.presets[m.presetIndex].label);
  addText(s, keep(s, buf), SZ_1, 1, yLabel, yLabel + 8, CYAN, false, false);
  addText(s, m.presetCount > 1 ? "Tap: next   Hold: start" : "Hold: start", SZ_1, 1, yHint, yHint + 8, SUB, false, false);
}

inline void buildScene(Scene &s, const Model &m, uint32_t now) {
  Screen id = pickScreen(m, now);
  s = Scene();
  s.id = id;
  const bool canStart = m.linked && m.presetCount > 0;
  if (canStart) { s.holdMs = START_HOLD_MS; s.holdColor = CYAN; }

  switch (id) {
    case SC_WIFI:
      setLook(s, &FACE_BIG, X_SLEEPY, SLATE, MOTION_SLEEPY);
      s.rim = {2, 1, RIM_DIM};
      s.holdMs = 0;
      addText(s, "Joining Wi-Fi", SZ_2, 1, 124, 140, TEXT, false, false);
      if (m.ssid[0]) addText(s, keep(s, m.ssid), SZ_21, 2, 144, 180, SUB);
      s.dots = true; s.dotsY = 192; s.dotsColor = SLATE;
      break;

    case SC_OFFLINE:
      setLook(s, &FACE_BIG, X_SEARCH, SLATE, MOTION_SEARCH);
      s.rim = {2, 1, RIM_DIM};
      s.holdMs = 0;
      addText(s, "Offline", SZ_3, 1, 118, 146, TEXT, false, false);
      addText(s, "Looking for your laptop", SZ_21, 2, 150, 186, SUB);
      s.dots = true; s.dotsY = 192; s.dotsColor = SLATE;
      break;

    case SC_HOME:
      setLook(s, &FACE_BIG, X_NEUTRAL, CYAN, MOTION_IDLE);
      if (m.presetCount) {
        addText(s, keep(s, m.presets[m.presetIndex].label), SZ_321, 3, 116, 146, TEXT);
        if (m.presetCount > 1) {
          s.pager = true; s.pagerN = m.presetCount; s.pagerI = m.presetIndex; s.pagerY = 154;
          addText(s, "Tap: next", SZ_2, 1, 162, 178, SUB, false, false);
        }
        addText(s, "Hold: start", SZ_2, 1, m.presetCount > 1 ? 180 : 170, m.presetCount > 1 ? 196 : 186, SUB, false, false);
      } else {
        addText(s, "Ready", SZ_3, 1, 120, 148, TEXT, false, false);
      }
      break;

    case SC_WORKING:
      setLook(s, &FACE_MED, X_THINK, AMBER, MOTION_WORKING);
      s.rim = {5, 3, AMBER};
      addText(s, "Working", SZ_2, 1, 98, 114, TEXT, false, false);
      s.dots = true; s.dotsY = 120; s.dotsColor = AMBER;
      if (m.taskSummary[0]) addText(s, keep(s, m.taskSummary), SZ_21, 2, 134, 196, TEXT, true, false);
      break;

    case SC_NEEDS_YOU:
      setLook(s, &FACE_MED, X_WORRY, AMBER, MOTION_ALERT);
      s.rim = {5, 3, AMBER};
      addText(s, "Needs you", SZ_32, 2, 98, 126, TEXT, false, false);
      addText(s, "Please look at your laptop", SZ_2, 1, 130, 166, SUB);
      addFooter(s, m, 172, 186);
      break;

    case SC_DONE:
    case SC_FAILED: {
      bool ok = id == SC_DONE;
      setLook(s, &FACE_SMALL, ok ? X_HAPPY : X_SAD, ok ? GREEN : RED, MOTION_IDLE);
      addText(s, ok ? "Done" : "Failed", SZ_2, 1, 72, 88, ok ? GREEN : RED, false, false);
      if (m.taskSummary[0]) addText(s, keep(s, m.taskSummary), SZ_21, 2, 94, 166, TEXT, true, true);
      addFooter(s, m, 170, 184);
      break;
    }

    case SC_APPROVAL: {
      const Approval &a = m.approvals[0];
      setLook(s, &FACE_TINY, X_WORRY, AMBER, MOTION_ALERT);
      s.rim = {1, 1, TRACK};
      s.holdMs = APPROVE_HOLD_MS; s.holdColor = GREEN;
      addKindAndRisk(s, a);
      // Short requests get big hints; long ones keep every line for the text.
      Block &roomy = addText(s, a.text, SZ_2, 1, APPROVAL_TOP, 154, TEXT);
      if (roomy.fits) {
        addText(s, "Tap: NO", SZ_2, 1, 158, 174, SUB, false, false);
        addText(s, "Hold: YES", SZ_2, 1, 176, 192, GREEN, false, false);
        if (m.approvalCount > 1) addCount(s, m);
      } else {
        s.ntb--;
        addText(s, a.text, SZ_21, 2, APPROVAL_TOP, APPROVAL_BOTTOM, TEXT);
        addQueueHint(s, m, "Tap: NO  Hold: YES", 186);
      }
      break;
    }

    case SC_LAPTOP: {
      const Approval &a = m.approvals[0];
      setLook(s, &FACE_TINY, X_WORRY, a.risk == RISK_HIGH ? RED : AMBER, MOTION_ALERT);
      s.rim = {3, 2, RED};
      s.holdMs = 0;
      addKindAndRisk(s, a);
      const char *why = a.reason[0] ? keep(s, a.reason) : "Needs a check";
      addText(s, why, SZ_21, 2, 92, 128, riskColor(a.risk));
      addText(s, "Answer on the laptop", SZ_2, 1, 132, 168, TEXT);
      addText(s, "Tap: deny", SZ_2, 1, 176, 192, SUB, false, false);
      if (m.approvalCount > 1) addCount(s, m);
      break;
    }

    case SC_NOTICE:
      switch (m.noticeKind) {
        case NK_STARTING:
          setLook(s, &FACE_BIG, X_WIDE, CYAN, MOTION_ALERT);
          addText(s, "Starting", SZ_3, 1, 118, 146, TEXT, false, false);
          if (m.noticeDetail[0]) addText(s, keep(s, m.noticeDetail), SZ_21, 2, 150, 186, SUB);
          s.dots = true; s.dotsY = 192; s.dotsColor = CYAN;
          break;
        case NK_SENDING:
          setLook(s, &FACE_BIG, X_NEUTRAL, CYAN, MOTION_ALERT);
          addText(s, "Sending", SZ_3, 1, 118, 146, TEXT, false, false);
          addText(s, "Please wait", SZ_2, 1, 152, 168, SUB, false, false);
          s.dots = true; s.dotsY = 192; s.dotsColor = CYAN;
          break;
        case NK_APPROVED:
          setLook(s, &FACE_BIG, X_HAPPY, GREEN, MOTION_ALERT);
          addText(s, "Approved", SZ_3, 1, 118, 146, GREEN, false, false);
          s.icon = 1; s.iconY = 174; s.iconColor = GREEN;
          break;
        case NK_DENIED:
          setLook(s, &FACE_BIG, X_FLAT, SLATE, MOTION_ALERT);
          addText(s, "Denied", SZ_3, 1, 118, 146, TEXT, false, false);
          s.icon = 2; s.iconY = 174; s.iconColor = RED;
          break;
        case NK_LAPTOP:
          setLook(s, &FACE_BIG, X_WORRY, AMBER, MOTION_ALERT);
          s.rim = {5, 3, AMBER};
          addText(s, "Approve on the laptop", SZ_32, 2, 118, 180, TEXT);
          break;
        default:  // NK_ERROR
          setLook(s, &FACE_BIG, X_SAD, RED, MOTION_ALERT);
          addText(s, keep(s, m.noticeTitle[0] ? m.noticeTitle : "Problem"), SZ_32, 2, 118, 148, TEXT, false, false);
          if (m.noticeDetail[0]) addText(s, keep(s, m.noticeDetail), SZ_21, 2, 152, 190, SUB);
          break;
      }
      break;
  }
}

// ---- painting the still parts ----
inline void paintScene(Gfx &g, const Scene &s) {
  g.clear(BG);
  if (s.rim.on) {
    for (int i = 0; i < RING_N; i++) {
      if (i % s.rim.period < s.rim.on) g.ringSegment(i, RING_N, RING_R0, RING_R1, s.rim.color);
    }
  }
  for (int t = 0; t < s.ntb; t++) {
    const Block &b = s.tb[t];
    for (int i = 0; i < b.n; i++) {
      const Line &l = b.line[i];
      int chars = l.len + (l.dots ? 3 : 0);
      int x = CX + b.dx - Gfx::textWidth(chars, b.size) / 2;
      int y = b.y + i * b.lineH;
      g.drawText(x, y, b.text + l.start, l.len, b.size, b.color);
      if (l.dots) g.drawText(x + Gfx::textWidth(l.len, b.size), y, "...", 3, b.size, b.color);
    }
  }
  if (s.pager) {
    int step = 14, x0 = CX - (s.pagerN - 1) * step / 2;
    for (int i = 0; i < s.pagerN; i++) {
      if (i == s.pagerI) g.fillCircle(x0 + i * step, s.pagerY, 4, CYAN);
      else g.fillCircle(x0 + i * step, s.pagerY, 2, RIM_DIM);
    }
  }
  if (s.pips) {
    const Block &b = s.tb[s.pipBlock];
    int textW = 0;
    for (int i = 0; i < b.n; i++) textW = b.line[i].len * 6 * b.size > textW ? b.line[i].len * 6 * b.size : textW;
    int x = CX + b.dx - textW / 2 - 36;
    for (int i = 0; i < 3; i++) {
      int h = 6 + 5 * i;
      g.fillRect(x + i * 11, b.y + 14 - h, 8, h, i < s.pipLevel ? s.pipColor : TRACK);
    }
  }
  if (s.icon == 1) {
    g.thickLine(CX - 16, s.iconY, CX - 5, s.iconY + 11, 3, s.iconColor);
    g.thickLine(CX - 5, s.iconY + 11, CX + 17, s.iconY - 13, 3, s.iconColor);
  } else if (s.icon == 2) {
    g.thickLine(CX - 13, s.iconY - 13, CX + 13, s.iconY + 13, 3, s.iconColor);
    g.thickLine(CX + 13, s.iconY - 13, CX - 13, s.iconY + 13, 3, s.iconColor);
  }
}

}  // namespace ui
