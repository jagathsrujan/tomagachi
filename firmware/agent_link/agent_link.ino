// Version 1 device: links the ESP32-S3 to the laptop bridge (m1/scripts/link.mjs) over
// Wi-Fi, starts Claude Code sessions and answers approvals. Holds no agent credentials.
//
// BOOT button, home screen:  tap = next preset, hold 0.8 s = start that session.
// BOOT button, approval:     tap = deny, hold 1.5 s = approve. A slip can only ever deny.
// USB serial:                type a line and press Enter to start a session with that text.
//
// Drawing lives in ui_*.h (see ui_engine.h). It sends only the screen tiles that changed and
// draws only when something moves, so idle costs almost no SPI traffic.
#include <Arduino_GFX_Library.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include "mbedtls/md.h"
#include "pins.h"
#include "secrets.h"
#include "ui_engine.h"
#include "ui_dial.h"

// Set to 1 to print frame and SPI figures once a second over Serial (for measuring on the board).
#ifndef UI_STATS
#define UI_STATS 0
#endif

constexpr uint32_t SPI_HZ = 20000000;  // 40 MHz is too fast for breadboard jumper wires
constexpr int PIN_BOOT = 0;
constexpr uint32_t SILENCE_MS = 15000;
constexpr uint32_t RETRY_MS = 3000;
constexpr size_t MAX_LINE = 4096;
constexpr uint32_t FLUSH_SLICE_US = 6000;  // longest the main loop spends sending pixels at a time

using ui::Approval;
using ui::Model;

Arduino_DataBus *bus = new Arduino_ESP32SPI(PIN_DC, PIN_CS, PIN_SCK, PIN_MOSI, GFX_NOT_DEFINED);
Arduino_GFX *panel = new Arduino_GC9A01(bus, PIN_RST, 0, true);
// The canvas only supplies the frame buffer and starts the panel; ui::Gfx draws into it.
Arduino_Canvas *canvas = new Arduino_Canvas(ui::W, ui::W, panel);

ui::Engine engine;
Model &M = engine.m;

WiFiClient client;
char lineBuffer[MAX_LINE + 1];
size_t lineLength = 0;
String serialBuffer;
bool linked = false;
uint32_t lastHeard = 0, lastAttempt = 0;
int refCounter = 0;

ui::Dial dial;
bool buttonDown = false, holdFired = false;
uint32_t pressedAt = 0;
bool dirty = true;

// ---------- Link ----------

void sendJson(JsonDocument &doc) {
  if (!client.connected()) return;
  String out;
  serializeJson(doc, out);
  out += '\n';
  client.print(out);
}

String authCode(const char *nonce) {
  uint8_t mac[32];
  mbedtls_md_hmac(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256),
                  (const uint8_t *)DEVICE_TOKEN, strlen(DEVICE_TOKEN),
                  (const uint8_t *)nonce, strlen(nonce), mac);
  char hex[65];
  for (int i = 0; i < 32; i++) sprintf(hex + i * 2, "%02x", mac[i]);
  return String(hex);
}

void showNotice(ui::NoticeKind kind, const char *title = "", const char *detail = "") {
  M.noticeKind = kind;
  ui::put(M.noticeTitle, sizeof M.noticeTitle, title);
  ui::put(M.noticeDetail, sizeof M.noticeDetail, detail);
  M.noticeUntil = millis() + ui::NOTICE_MS;
  dirty = true;
}

void removeApproval(const char *id) {
  for (int i = 0; i < M.approvalCount; i++) {
    if (strcmp(M.approvals[i].id, id)) continue;
    for (int j = i; j + 1 < M.approvalCount; j++) M.approvals[j] = M.approvals[j + 1];
    M.approvalCount--;
    dirty = true;
    return;
  }
}

// No link means nobody can answer, so everything on screen is withdrawn.
void dropLink() {
  client.stop();
  linked = false;
  M.linked = false;
  M.approvalCount = 0;
  lineLength = 0;
  dirty = true;
}

ui::TaskState taskStateFrom(const char *s) {
  if (!strcmp(s, "working")) return ui::TS_WORKING;
  if (!strcmp(s, "needs_you")) return ui::TS_NEEDS_YOU;
  if (!strcmp(s, "done")) return ui::TS_DONE;
  if (!strcmp(s, "failed")) return ui::TS_FAILED;
  return s[0] ? ui::TS_IDLE : ui::TS_NONE;
}

void setTask(JsonObjectConst task) {
  ui::TaskState state = taskStateFrom(task["state"] | "");
  bool changed = state != M.taskState;
  M.taskState = state;
  changed |= ui::put(M.taskTitle, sizeof M.taskTitle, task["title"] | "");
  changed |= ui::put(M.taskSummary, sizeof M.taskSummary, task["summary"] | "");
  bool phone = task["needsPhone"] | false;
  changed |= phone != M.needsLaptop;
  M.needsLaptop = phone;
  // The session is now visible on the home screen, so "Starting" has done its job.
  if (M.noticeKind == ui::NK_STARTING && state != ui::TS_NONE && state != ui::TS_IDLE) {
    M.noticeKind = ui::NK_NONE;
    changed = true;
  }
  if (changed) dirty = true;
}

void addApproval(JsonDocument &m) {
  const char *id = m["id"] | "";
  const char *nonce = m["nonce"] | "";
  for (int i = 0; i < M.approvalCount; i++) if (!strcmp(M.approvals[i].id, id)) return;
  if (M.approvalCount == ui::MAX_APPROVALS) return;  // It stays open on the laptop page.
  Approval &a = M.approvals[M.approvalCount];
  memset(&a, 0, sizeof a);
  // An id that does not fit could not be answered correctly, so it is left to the laptop page.
  if (strlen(id) >= sizeof a.id || strlen(nonce) >= sizeof a.nonce) return;
  strcpy(a.id, id);
  strcpy(a.nonce, nonce);
  ui::put(a.kind, sizeof a.kind, m["kind"] | "");
  ui::put(a.reason, sizeof a.reason, m["reason"] | "");
  const char *risk = m["risk"] | "high";
  a.risk = !strcmp(risk, "low") ? ui::RISK_LOW : !strcmp(risk, "medium") ? ui::RISK_MED : ui::RISK_HIGH;
  // Approvable only if the bridge says so AND the whole text is plain ASCII that fits on screen.
  // High risk is never approvable here. Text is never shortened: if it can't be shown, it
  // can't be approved here.
  const char *text = m["text"] | "";
  a.approvable = (m["deviceApprovable"] | false) && a.risk != ui::RISK_HIGH && text[0] &&
                 strlen(text) < sizeof a.text && ui::isPlainAscii(text) && ui::approvalTextFits(text);
  if (a.approvable) strcpy(a.text, text);
  M.approvalCount++;
  if (buttonDown) holdFired = true;  // A press from before this request must not answer it.
  dirty = true;
}

void loadProfiles(JsonDocument &m) {
  M.presetCount = 0;
  bool full = false;
  for (JsonObjectConst profile : m["profiles"].as<JsonArrayConst>()) {
    for (JsonVariantConst label : profile["presets"].as<JsonArrayConst>()) {
      if (M.presetCount == ui::MAX_PRESETS) { full = true; break; }
      const char *name = label.as<const char *>();
      const char *id = profile["id"] | "";
      // The label is sent back as written, so one that does not fit is skipped, not shortened.
      if (!name || strlen(name) >= ui::LABEL_CAP || strlen(id) >= sizeof M.presets[0].profile) continue;
      strcpy(M.presets[M.presetCount].profile, id);
      strcpy(M.presets[M.presetCount].label, name);
      M.presetCount++;
    }
    if (full) break;
  }
  M.presetIndex = 0;
  dirty = true;
}

const char *resultText(const char *status) {
  if (!strcmp(status, "expired")) return "The request ran out of time";
  if (!strcmp(status, "stale")) return "That request is no longer open";
  if (!strcmp(status, "failed")) return "Could not send the answer";
  return "The answer was not accepted";
}

void onMessage(const char *line) {
  JsonDocument m;
  if (deserializeJson(m, line)) return;
  const char *type = m["type"] | "";

  if (!strcmp(type, "hello")) {
    JsonDocument out;
    out["type"] = "auth";
    out["mac"] = authCode(m["nonce"] | "");
    sendJson(out);
  } else if (!strcmp(type, "auth.ok")) {
    linked = true;
    M.linked = true;
    dirty = true;
    Serial.println("Linked to the laptop");
  } else if (!strcmp(type, "ping")) {
    JsonDocument out;
    out["type"] = "pong";
    sendJson(out);
  } else if (!strcmp(type, "state.snapshot")) {
    JsonArrayConst tasks = m["tasks"].as<JsonArrayConst>();
    if (tasks.size()) setTask(tasks[0]);
  } else if (!strcmp(type, "profiles.sync")) {
    loadProfiles(m);
  } else if (!strcmp(type, "task.update")) {
    setTask(m["task"]);
    Serial.printf("%s: %s\n", M.taskState == ui::TS_WORKING ? "working" : "update", M.taskSummary);
  } else if (!strcmp(type, "approval.request")) {
    addApproval(m);
  } else if (!strcmp(type, "approval.cancel")) {
    removeApproval(m["id"] | "");
  } else if (!strcmp(type, "approval.result")) {
    const char *status = m["status"] | "";
    removeApproval(m["id"] | "");
    // "Approved" appears only here, after the bridge confirms it passed the answer on.
    if (!strcmp(status, "accepted")) {
      showNotice(!strcmp(m["decision"] | "", "approve") ? ui::NK_APPROVED : ui::NK_DENIED);
    } else if (!strcmp(status, "needs_phone")) {
      showNotice(ui::NK_LAPTOP);
    } else {
      showNotice(ui::NK_ERROR, "Not sent", resultText(status));
    }
  } else if (!strcmp(type, "task.error")) {
    showNotice(ui::NK_ERROR, "Problem", m["reason"] | "Something failed");
  }
}

void pollLink() {
  bool wifi = WiFi.status() == WL_CONNECTED;
  if (wifi != M.wifi) { M.wifi = wifi; dirty = true; }
  if (!wifi) {
    if (client.connected() || linked) dropLink();
    return;
  }
  if (!client.connected()) {
    if (linked) dropLink();
    if (millis() - lastAttempt < RETRY_MS) return;
    lastAttempt = millis();
    if (client.connect(BRIDGE_HOST, BRIDGE_PORT, 2000)) {
      client.setNoDelay(true);
      lastHeard = millis();
    }
    return;
  }
  while (client.available()) {
    char c = client.read();
    lastHeard = millis();
    if (c == '\n') {
      lineBuffer[lineLength] = 0;
      onMessage(lineBuffer);
      lineLength = 0;
    } else if (lineLength < MAX_LINE) {
      lineBuffer[lineLength++] = c;
    } else {
      return dropLink();
    }
  }
  if (millis() - lastHeard > SILENCE_MS) dropLink();
}

// ---------- Input ----------

void answerApproval(const char *decision) {
  JsonDocument out;
  out["type"] = "approval.response";
  out["id"] = M.approvals[0].id;
  out["nonce"] = M.approvals[0].nonce;
  out["decision"] = decision;
  sendJson(out);
  // Nothing shows as approved until the bridge confirms it with approval.result.
  removeApproval(M.approvals[0].id);
  showNotice(ui::NK_SENDING);
}

void startTask(const char *profile, const char *preset, const char *text) {
  JsonDocument out;
  out["type"] = "task.start";
  out["ref"] = String("d") + (++refCounter);
  out["profile"] = profile;
  if (preset) out["preset"] = preset;
  if (text) out["text"] = text;
  sendJson(out);
  showNotice(ui::NK_STARTING, "", preset ? preset : "Your request");
}

void onTap() {
  if (!linked) return;
  if (M.approvalCount) return answerApproval("deny");
  if (M.presetCount) {
    M.presetIndex = (M.presetIndex + 1) % M.presetCount;
    dirty = true;
  }
}

void onHold() {
  if (!linked) return;
  if (M.approvalCount) {
    if (M.approvals[0].approvable) answerApproval("approve");
    return;
  }
  if (M.presetCount) {
    const ui::Preset &p = M.presets[M.presetIndex];
    startTask(p.profile, p.label, nullptr);
  }
}

void pollButton() {
  bool down = digitalRead(PIN_BOOT) == LOW;
  uint32_t now = millis();
  if (down && !buttonDown) {
    buttonDown = true;
    holdFired = false;
    pressedAt = now;
  }
  if (down && !holdFired) {
    uint32_t need = M.approvalCount ? ui::APPROVE_HOLD_MS : ui::START_HOLD_MS;
    if (now - pressedAt >= need) {
      holdFired = true;
      onHold();
    }
  }
  if (!down && buttonDown) {
    buttonDown = false;
    if (!holdFired && now - pressedAt > 30) onTap();
  }
  // The ring is drawn by the engine from these two values.
  M.pressing = buttonDown && !holdFired;
  M.pressedAt = pressedAt;
}

// The thumbwheel only moves the preset selection, like a watch crown scrolling a list.
// It never answers an approval: approve and deny stay on the button (hold and tap).
void pollDial() {
#if DIAL_ENABLED
  static uint32_t last = 0;
  uint32_t now = millis();
  if (now - last < 20) return;
  last = now;
  int raw = analogRead(PIN_DIAL);
#if DIAL_REVERSED
  raw = 4095 - raw;
#endif
  int index = dial.update(raw, M.presetCount);
  if (index < 0 || index == M.presetIndex) return;
  M.presetIndex = index;
  dirty = true;
#endif
}

void pollSerial() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c != '\n' && c != '\r') {
      if (serialBuffer.length() < 1000) serialBuffer += c;
      continue;
    }
    serialBuffer.trim();
    if (serialBuffer.length() && linked && M.presetCount) {
      startTask(M.presets[0].profile, nullptr, serialBuffer.c_str());
    }
    serialBuffer = "";
  }
}

// ---------- Display ----------

uint32_t hookMicros() { return micros(); }
uint32_t hookRandom(uint32_t n) { return esp_random() % n; }
void hookSend(int x, int y, int w, int h, uint16_t *pixels) { panel->draw16bitRGBBitmap(x, y, pixels, w, h); }

#if UI_STATS
uint32_t statFrames = 0, statFrameUsSum = 0, statFrameUsMax = 0;

// A "frame" is one pass of the loop that sent pixels: drawing plus SPI time.
void noteFrame(uint32_t us) {
  statFrames++;
  statFrameUsSum += us;
  if (us > statFrameUsMax) statFrameUsMax = us;
}

void printStats() {
  static uint32_t last = 0, bytes0 = 0, runs0 = 0, sendUs0 = 0;
  uint32_t now = millis();
  if (now - last < 1000) return;
  const ui::FlushStats &f = engine.g.stats;
  Serial.printf("UI frames=%lu avg_us=%lu max_us=%lu bytes=%lu runs=%lu spi_us=%lu draw_max_us=%lu heap=%lu\n",
                (unsigned long)statFrames, (unsigned long)(statFrames ? statFrameUsSum / statFrames : 0),
                (unsigned long)statFrameUsMax, (unsigned long)(f.bytes - bytes0), (unsigned long)(f.runs - runs0),
                (unsigned long)(f.sendUs - sendUs0), (unsigned long)engine.stats.drawUsMax, (unsigned long)ESP.getFreeHeap());
  bytes0 = f.bytes; runs0 = f.runs; sendUs0 = f.sendUs;
  statFrames = statFrameUsSum = statFrameUsMax = 0;
  engine.stats.drawUsMax = 0;
  last = now;
}
#endif

void setup() {
  Serial.begin(115200);
  pinMode(PIN_BOOT, INPUT_PULLUP);
  if (!canvas->begin(SPI_HZ)) {
    while (true) {
      Serial.println("Display init failed");
      delay(1000);
    }
  }
  // One full frame, as the kangaroo sketch does: it also blanks the corners the UI never sends.
  canvas->fillScreen(0);
  canvas->flush();
  ui::Hooks hooks = {hookMicros, hookRandom, hookSend};
  engine.begin(canvas->getFramebuffer(), hooks);
  M.hasDial = DIAL_ENABLED;
  ui::put(M.ssid, sizeof M.ssid, WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void loop() {
  pollLink();
  pollButton();
  pollDial();
  pollSerial();
  if (dirty) {
    dirty = false;
    engine.invalidate();
  }
#if UI_STATS
  uint32_t t0 = micros(), sent0 = engine.g.stats.bytes;
#endif
  engine.tick(millis());
  bool more = engine.flush(FLUSH_SLICE_US);
#if UI_STATS
  if (engine.g.stats.bytes != sent0) noteFrame(micros() - t0);
  printStats();
#endif
  if (!more) {
    uint32_t wait = engine.wakeIn(millis());
    delay(wait < 1 ? 1 : wait > 10 ? 10 : wait);  // never sleep past a button poll
  }
}
