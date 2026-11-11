// Host-side preview and load estimate. Compiles the sketch's real UI code (../ui_*.h) with a
// fake panel, renders every screen to PPM, checks that the "panel" ends up identical to the
// frame buffer, checks nothing is drawn outside the safe circle, and simulates how many
// bytes each state sends. Build and run with preview/run.sh.
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include "../ui_engine.h"

using namespace ui;

static uint16_t fbMem[W * W] __attribute__((aligned(16)));
static uint16_t panelMem[W * W];
static uint64_t simUs = 0;
static uint64_t totalBytes = 0, totalRuns = 0;
static uint32_t rngState = 12345;

static uint32_t hMicros() { return (uint32_t)simUs; }
static uint32_t hRandom(uint32_t n) {
  rngState ^= rngState << 13; rngState ^= rngState >> 17; rngState ^= rngState << 5;
  return rngState % n;
}
// 20 MHz SPI: 8 bits per byte, plus about 30 us of command and driver overhead per block.
static void hSend(int x, int y, int w, int h, uint16_t *px) {
  for (int r = 0; r < h; r++) memcpy(panelMem + (y + r) * W + x, px + r * w, w * 2);
  totalBytes += (uint64_t)w * h * 2;
  totalRuns++;
  simUs += (uint64_t)w * h * 2 * 8 / 20 + 30;  // 20 bits per microsecond
}

static Engine eng;
static uint32_t nowMs() { return (uint32_t)(simUs / 1000); }

static void settle() {  // run the loop until nothing is pending
  for (int i = 0; i < 1000; i++) {
    eng.tick(nowMs());
    if (!eng.flush(6000)) {
      if (eng.wakeIn(nowMs()) != 0) return;
    }
  }
}

static void resetEngine() {
  memset(fbMem, 0, sizeof fbMem);
  for (int i = 0; i < W * W; i++) panelMem[i] = 0xF81F;  // magenta = never sent
  Hooks h = {hMicros, hRandom, hSend};
  new (&eng) Engine();
  eng.begin(fbMem, h);
  simUs = 1000000;
  totalBytes = totalRuns = 0;
}

static void savePPM(const char *path) {
  FILE *f = fopen(path, "wb");
  fprintf(f, "P6\n%d %d\n255\n", W, W);
  for (int i = 0; i < W * W; i++) {
    uint16_t c = panelMem[i];
    uint8_t rgb[3] = {(uint8_t)(((c >> 11) << 3) | (c >> 13)), (uint8_t)((((c >> 5) & 63) << 2) | ((c >> 9) & 3)),
                      (uint8_t)(((c & 31) << 3) | ((c >> 2) & 7))};
    fwrite(rgb, 1, 3, f);
  }
  fclose(f);
}

// Checks: panel equals frame buffer on every visible pixel, and nothing but the rim
// reaches the edge of the circle.
static int checkImage(const char *name) {
  int bad = 0, stale = 0, clipped = 0, rimGap = 0;
  for (int y = 0; y < W; y++) {
    for (int x = 0; x < W; x++) {
      float dx = x + 0.5f - 120, dy = y + 0.5f - 120, r = sqrtf(dx * dx + dy * dy);
      if (r > 119) continue;
      uint16_t p = panelMem[y * W + x], f = fbMem[y * W + x];
      if (p == 0xF81F && f != 0xF81F) stale++;
      else if (p != f) bad++;
      if (f != BG && r > 118.0f) clipped++;
      if (f != BG && r > 105.5f && r < 109.5f) rimGap++;
    }
  }
  if (bad || stale || clipped || rimGap)
    printf("  CHECK %-28s mismatch=%d unsent=%d outside=%d intoRimGap=%d\n", name, bad, stale, clipped, rimGap);
  return bad + stale + clipped + rimGap;
}

// ---------- scenarios ----------
static void fillPresets(Model &m, bool longLabel) {
  m.presetCount = 3;
  strcpy(m.presets[0].label, "List files");
  strcpy(m.presets[1].label, "Create note");
  strcpy(m.presets[2].label, longLabel ? "Summarise yesterday's unread email threads and draft replies" : "Delete note");
  for (int i = 0; i < 3; i++) strcpy(m.presets[i].profile, "claude-sandbox");
  m.presetIndex = longLabel ? 2 : 0;
}
static void linked(Model &m) {
  m.wifi = m.linked = true;
  strcpy(m.ssid, "Home-WiFi-2G");
}
static void addApproval(Model &m, const char *kind, Risk risk, const char *reason, const char *text, bool approvable) {
  Approval &a = m.approvals[m.approvalCount++];
  strcpy(a.id, "id1"); strcpy(a.nonce, "n1");
  strcpy(a.kind, kind); strcpy(a.reason, reason);
  strcpy(a.text, approvable ? text : "");
  a.risk = risk;
  a.approvable = approvable;
}
static const char *TEXT140 =
    "git commit -m \"Refactor the payment retry logic and update the unit tests for the new timeout handling in the checkout service module\"";
static const char *NOSPACE140 =
    "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
static const char *LONGWORD =
    "touch src/components/dashboard/widgets/analytics/RevenueByRegionChart.component.test.tsx";
static const char *ANSWER160 =
    "The repo has four files: README.md, package.json, an index.js entry point and a tests folder with two passing unit tests for the date parsing helpers.";

struct Scn {
  const char *name;
  void (*setup)(Model &);
  uint32_t pressMs;  // >0: pretend BOOT has been held this long
};

static std::vector<Scn> scenarios() {
  return {
    {"01_wifi", [](Model &m) { m.wifi = false; strcpy(m.ssid, "Home-WiFi-2G"); }, 0},
    {"02_offline", [](Model &m) { m.wifi = true; strcpy(m.ssid, "x"); }, 0},
    {"03_home", [](Model &m) { linked(m); fillPresets(m, false); }, 0},
    {"03b_home_longlabel", [](Model &m) { linked(m); fillPresets(m, true); }, 0},
    {"03c_home_nopresets", [](Model &m) { linked(m); }, 0},
    {"04_home_hold_start", [](Model &m) { linked(m); fillPresets(m, false); }, 500},
    {"05_starting", [](Model &m) { linked(m); fillPresets(m, false); m.noticeKind = NK_STARTING; strcpy(m.noticeDetail, "List files"); m.noticeUntil = 1000000 + 3000; }, 0},
    {"06_working", [](Model &m) { linked(m); fillPresets(m, false); m.taskState = TS_WORKING; strcpy(m.taskSummary, "Reading the project files and checking what changed since the last commit"); }, 0},
    {"06b_working_long", [](Model &m) { linked(m); fillPresets(m, false); m.taskState = TS_WORKING; strcpy(m.taskSummary, ANSWER160); }, 0},
    {"07_approval_low", [](Model &m) { linked(m); fillPresets(m, false); addApproval(m, "command", RISK_LOW, "Read or test only", "ls -la", true); }, 0},
    {"07b_approval_medium", [](Model &m) { linked(m); addApproval(m, "command", RISK_MED, "Local change", "touch note-from-device.txt", true); }, 0},
    {"07c_approval_140", [](Model &m) { linked(m); addApproval(m, "command", RISK_MED, "Local change", TEXT140, true); }, 0},
    {"07d_approval_nospace140", [](Model &m) { linked(m); addApproval(m, "file-change", RISK_MED, "Edit file", NOSPACE140, true); }, 0},
    {"07e_approval_longword", [](Model &m) { linked(m); addApproval(m, "file-read", RISK_LOW, "Read file", LONGWORD, true); }, 0},
    {"07f_approval_queue", [](Model &m) { linked(m); addApproval(m, "file-change", RISK_MED, "Edit file", "src/index.js", true); addApproval(m, "command", RISK_LOW, "", "ls", true); addApproval(m, "command", RISK_LOW, "", "pwd", true); }, 0},
    {"08_approval_hold_50", [](Model &m) { linked(m); addApproval(m, "command", RISK_MED, "Local change", "touch note-from-device.txt", true); }, 150 + 600},
    {"08b_approval_hold_full", [](Model &m) { linked(m); addApproval(m, "command", RISK_MED, "Local change", "touch note-from-device.txt", true); }, 1499},
    {"09_laptop_high", [](Model &m) { linked(m); addApproval(m, "command", RISK_HIGH, "Complex shell command", "", false); }, 0},
    {"09b_laptop_secrets", [](Model &m) { linked(m); addApproval(m, "file-read", RISK_HIGH, "Touches secrets", "", false); }, 0},
    {"09c_laptop_odd_kind", [](Model &m) { linked(m); addApproval(m, "mcp-tool-call", RISK_HIGH, "Unknown request type", "", false); }, 0},
    {"10_notice_sending", [](Model &m) { linked(m); m.noticeKind = NK_SENDING; m.noticeUntil = 1000000 + 3000; }, 0},
    {"11_notice_approved", [](Model &m) { linked(m); m.noticeKind = NK_APPROVED; m.noticeUntil = 1000000 + 3000; }, 0},
    {"12_notice_denied", [](Model &m) { linked(m); m.noticeKind = NK_DENIED; m.noticeUntil = 1000000 + 3000; }, 0},
    {"13_notice_laptop", [](Model &m) { linked(m); m.noticeKind = NK_LAPTOP; m.noticeUntil = 1000000 + 3000; }, 0},
    {"14_notice_error", [](Model &m) { linked(m); m.noticeKind = NK_ERROR; strcpy(m.noticeTitle, "Not sent"); strcpy(m.noticeDetail, "The request ran out of time"); m.noticeUntil = 1000000 + 3000; }, 0},
    {"14b_notice_error_long", [](Model &m) { linked(m); m.noticeKind = NK_ERROR; strcpy(m.noticeTitle, "Problem"); strcpy(m.noticeDetail, "Could not reach the project folder because it was moved or renamed"); m.noticeUntil = 1000000 + 3000; }, 0},
    {"15_needs_you", [](Model &m) { linked(m); fillPresets(m, false); m.taskState = TS_NEEDS_YOU; }, 0},
    {"16_done_short", [](Model &m) { linked(m); fillPresets(m, false); m.taskState = TS_DONE; strcpy(m.taskSummary, "Created note-from-device.txt"); }, 0},
    {"16b_done_160", [](Model &m) { linked(m); fillPresets(m, false); m.taskState = TS_DONE; strcpy(m.taskSummary, ANSWER160); }, 0},
    {"16c_done_nospace160", [](Model &m) { linked(m); fillPresets(m, false); m.taskState = TS_DONE; strncpy(m.taskSummary, NOSPACE140, 160); }, 0},
    {"17_failed", [](Model &m) { linked(m); fillPresets(m, false); m.taskState = TS_FAILED; strcpy(m.taskSummary, "Session failed: the model could not be reached"); }, 0},
  };
}

static void render(const Scn &s, const char *dir, int &problems) {
  resetEngine();
  eng.m = Model();
  s.setup(eng.m);
  if (s.pressMs) {
    eng.m.pressing = true;
    eng.m.pressedAt = nowMs() - s.pressMs;
  }
  eng.invalidate();
  // Let the first frames play, then freeze the face at its centred pose for a stable image.
  settle();
  char path[256];
  snprintf(path, sizeof path, "%s/%s.ppm", dir, s.name);
  savePPM(path);
  problems += checkImage(s.name);
  Approval *a = eng.m.approvalCount ? &eng.m.approvals[0] : nullptr;
  if (a && a->approvable && !approvalTextFits(a->text)) {
    printf("  CHECK %s: approval text does not fit\n", s.name);
    problems++;
  }
}

// ---------- load simulation ----------
struct Load {
  double bytesPerSec, spiMsPerSec, framesPerSec;
  uint32_t maxChunkUs;
};

static Load simulate(void (*setup)(Model &), double seconds, uint32_t pressAtMs, uint32_t pressForMs, int approvals = 0) {
  resetEngine();
  eng.m = Model();
  setup(eng.m);
  eng.invalidate();
  settle();                          // initial paint is not counted
  uint64_t b0 = totalBytes, r0 = totalRuns;
  uint32_t f0 = eng.stats.faceFrames + eng.stats.ringFrames + eng.stats.dotFrames;
  uint64_t start = simUs, end = simUs + (uint64_t)(seconds * 1e6);
  uint32_t maxChunk = 0;
  uint32_t t0 = nowMs();
  while (simUs < end) {
    uint32_t now = nowMs();
    eng.m.pressing = pressForMs && now >= t0 + pressAtMs && now < t0 + pressAtMs + pressForMs;
    eng.m.pressedAt = t0 + pressAtMs;
    eng.tick(now);
    uint64_t before = simUs;
    bool pend = eng.flush(6000);
    if (simUs - before > maxChunk) maxChunk = (uint32_t)(simUs - before);
    if (!pend) {
      uint32_t w = eng.wakeIn(nowMs());
      simUs += (w < 1 ? 1 : w > 10 ? 10 : w) * 1000ull;
    }
  }
  double secs = (simUs - start) / 1e6;
  uint64_t bytes = totalBytes - b0, runs = totalRuns - r0;
  uint32_t frames = eng.stats.faceFrames + eng.stats.ringFrames + eng.stats.dotFrames - f0;
  Load l;
  l.bytesPerSec = bytes / secs;
  l.spiMsPerSec = (bytes * 8 / 20e6 * 1000 + runs * 0.030) / secs;
  l.framesPerSec = frames / secs;
  l.maxChunkUs = maxChunk;
  return l;
}

static void report(const char *name, Load l) {
  printf("%-34s %8.0f B/s  SPI %6.1f ms/s (%4.1f%%)  draws %5.1f/s  longest flush slice %5u us\n", name, l.bytesPerSec,
         l.spiMsPerSec, l.spiMsPerSec / 10.0, l.framesPerSec, l.maxChunkUs);
}

// Cost of switching from one screen to another (bytes and SPI time).
static void transition(const char *name, void (*from)(Model &), void (*to)(Model &)) {
  resetEngine();
  eng.m = Model();
  from(eng.m);
  eng.invalidate();
  settle();
  uint64_t b0 = totalBytes, r0 = totalRuns, t0 = simUs;
  eng.m = Model();
  to(eng.m);
  eng.invalidate();
  settle();
  uint64_t bytes = totalBytes - b0;
  printf("%-34s %7llu B  (%4.1f%% of a full frame)  SPI %5.1f ms\n", name, (unsigned long long)bytes,
         100.0 * bytes / (W * W * 2), (bytes * 8 / 20e6 * 1000) + (totalRuns - r0) * 0.030);
  (void)t0;
}

int main(int argc, char **argv) {
  const char *dir = argc > 1 ? argv[1] : ".";
  int problems = 0;
  for (const Scn &s : scenarios()) render(s, dir, problems);
  printf("Layout checks: %s\n", problems ? "PROBLEMS ABOVE" : "all clean");

  puts("\nSteady-state load at 20 MHz SPI (60 s unless noted)");
  auto home = [](Model &m) { linked(m); fillPresets(m, false); };
  report("home idle", simulate(home, 60, 0, 0));
  report("wifi joining", simulate([](Model &m) { m.wifi = false; }, 30, 0, 0));
  report("offline (searching)", simulate([](Model &m) { m.wifi = true; }, 30, 0, 0));
  report("working", simulate([](Model &m) { linked(m); fillPresets(m, false); m.taskState = TS_WORKING; strcpy(m.taskSummary, "Reading files"); }, 30, 0, 0));
  report("approval waiting (static)", simulate([](Model &m) { linked(m); addApproval(m, "command", RISK_MED, "x", "touch a.txt", true); }, 60, 0, 0));
  report("laptop-only request (static)", simulate([](Model &m) { linked(m); addApproval(m, "command", RISK_HIGH, "Complex shell command", "", false); }, 60, 0, 0));
  report("done with answer (static)", simulate([](Model &m) { linked(m); fillPresets(m, false); m.taskState = TS_DONE; strcpy(m.taskSummary, ANSWER160); }, 60, 0, 0));
  report("approval hold ring (1.5 s)", simulate([](Model &m) { linked(m); addApproval(m, "command", RISK_MED, "x", "touch a.txt", true); }, 1.5, 0, 1500));
  report("home hold ring (0.8 s)", simulate(home, 0.8, 0, 800));

  puts("\nScreen changes (one-off)");
  auto work = [](Model &m) { linked(m); fillPresets(m, false); m.taskState = TS_WORKING; strcpy(m.taskSummary, "Reading files"); };
  auto appr = [](Model &m) { linked(m); addApproval(m, "command", RISK_MED, "x", TEXT140, true); };
  auto done = [](Model &m) { linked(m); fillPresets(m, false); m.taskState = TS_DONE; strcpy(m.taskSummary, ANSWER160); };
  auto sending = [](Model &m) { linked(m); m.noticeKind = NK_SENDING; m.noticeUntil = 1000000 + 3000; };
  auto approved = [](Model &m) { linked(m); m.noticeKind = NK_APPROVED; m.noticeUntil = 1000000 + 3000; };
  transition("home -> working", home, work);
  transition("working -> approval", work, appr);
  transition("approval -> sending", appr, sending);
  transition("sending -> approved", sending, approved);
  transition("approved -> home", approved, home);
  transition("working -> done", work, done);
  transition("home -> next preset", home, [](Model &m) { linked(m); fillPresets(m, false); m.presetIndex = 1; });
  transition("offline -> home", [](Model &m) { m.wifi = true; }, home);
  return problems ? 1 : 0;
}
