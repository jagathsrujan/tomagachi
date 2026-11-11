# AGENTS.md

Single source of truth for any AI coding tool working in this repo (Codex, Cursor, Gemini CLI, Claude Code, Command Code, others). You have no memory of past conversations, so read this file and `STATUS.md` before doing anything. Keep both accurate: update them when you finish work.

## What this is
A small round ESP32-S3 voice companion for UVCE alumni (UVCE is a Bangalore engineering college; the founder is a student there). The buyer uses it by voice, away from a laptop, to:
1. Delegate admin tasks (calendar, reminders, search, answers).
2. Talk to running coding-agent sessions: start them, hear results, approve or deny permission requests.

Targets: build cost about 4,000 rupees, sale price about 5,000 rupees *(as of 2026-11-06)*. The founder does not want to run models, tokens or agent orchestration; the buyer brings their own Claude Code login. The device holds no agent tokens and makes no internet calls of its own.

**Version 1 decision (2026-11-07):** the board connects straight to the laptop over Wi-Fi, no phone. The laptop bridge creates Claude Code sessions directly through the Agent SDK (default) or through T3 Code (`--t3`). The phone route in `FORK_PLAN.md` comes later.

## Repo map
There is no git repo yet (see STATUS.md). Paths are relative to the repo root.

| Path | What it is |
|---|---|
| `AGENTS.md`, `STATUS.md` | This file and the live status. Start here. |
| `PROJECT_BRIEF.md` | Founder's vision, decisions, hardware on hand, known limits. |
| `FEATURES.md` | Feature list and build order for the breadboard prototype. |
| `SOFTWARE_SPEC.md` | Full software spec: approval rules, risk table, protocol. Parts are older than the Wi-Fi decision (see Docs below). |
| `OLDER_ALUMNI_PLAN.md`, `FORK_PLAN.md` | Go-to-market choice (older alumni, admin by voice) and the later phone-app plan. |
| `M0_RESULTS.md`, `M1_RESULTS.md` | What was tested with real agents, and what was not. |
| `BACKEND_STRATEGY.md`, `FEASIBILITY.md` | Desk research on agent products and routes. Not tested. |
| `HARDWARE_BUDGET.md`, `STAGE1_ORDER.md`, `PITCH_PROTOTYPE.md`, `OUTREACH_DRAFTS.md`, `PLAN.md` | Costs, shopping, pitch video, outreach drafts, and an old superseded plan. |
| `m1/` | **The laptop bridge (Node, ES modules).** The code that matters. |
| `m1/src/bridge.mjs` | `DeviceBridge`: turns agent events into device messages and enforces every approval rule. |
| `m1/src/risk.mjs` | Risk rules and `canApproveOnDevice`. |
| `m1/src/claudeBackend.mjs` | Direct Claude Code backend (Agent SDK). |
| `m1/src/t3Backend.mjs`, `m1/src/localT3.mjs` | T3 Code backend and local T3 server launcher. |
| `m1/src/deviceLink.mjs` | TCP link to the board (one JSON message per line, HMAC pairing). |
| `m1/src/server.mjs`, `m1/public/` | Laptop approvals page and device simulator (http://127.0.0.1:3918). |
| `m1/scripts/link.mjs` | Entry point for the real board (`npm run link`). |
| `m1/scripts/fake-device.mjs` | Stands in for the board on loopback. |
| `m1/scripts/dev.mjs`, `m1/scripts/soak.mjs` | Browser device simulator over T3; approval-lifetime soak test. |
| `m1/test/` | Unit tests (`node --test`). |
| `m1/profiles.json` | Profiles and presets shown on the device. Sessions run in `m1/sandbox-repo`. |
| `m1/data/` | Local runtime data: `device-token` (pairing secret), logs. Not committed. |
| `firmware/agent_link/` | **Version 1 board firmware (Arduino).** Another agent may be editing the UI here: read the files fresh, and see its `README.md`. |
| `firmware/face_animation/`, `firmware/kangaroo_animation/` | Earlier display experiments. |
| `m0/` | Throwaway first test of T3 (script, logs, test data). Safe to ignore. |
| `video_dl/` | One downloaded reference video (`video.mp4`, about 7 MB). Not source. |
| `.claude/launch.json` | Claude Code desktop launch configs; the commands below work without it. |

## Commands
Node 26 is installed locally. Run bridge commands from `m1/`.

```bash
cd m1
npm test                 # unit tests (node --test test/). 53 pass as of 2026-11-07
npm run link             # bridge for the real board, Claude Code direct (Agent SDK)
npm run link:t3          # same, but sessions go through T3 Code
npm run dev              # browser device simulator over T3 (node scripts/dev.mjs)
node scripts/fake-device.mjs [preset] [approve|deny]   # fake board against a running bridge; default preset "Create note"
```
- `npm run link` starts: the board link on TCP port 3919 (`LINK_PORT`), the approvals page on http://127.0.0.1:3918 (`PORT`), T3 on 3927 with `--t3` (`T3_PORT`). `APPROVAL_TTL_MS` (default 120000) is how long the board gets to answer before the request moves to the laptop page.
- First run creates `m1/data/device-token` and writes `firmware/agent_link/secrets.h` (Wi-Fi name and password placeholders, laptop address, bridge port, token). Edit the Wi-Fi lines by hand.
- Real sessions need Claude Code logged in on this laptop, and they cost the founder's usage. Prefer unit tests; run real sessions only when needed.

Firmware (board on USB; replace the port, find it with `arduino-cli board list`):
```bash
cd firmware/agent_link
arduino-cli compile --fqbn esp32:esp32:esp32s3:CDCOnBoot=cdc .
arduino-cli compile --upload -p /dev/cu.usbmodemXXXX --fqbn esp32:esp32:esp32s3:CDCOnBoot=cdc .
```
Installed here: esp32 core 3.3.12, GFX Library for Arduino 1.6.9, ArduinoJson 7.4.3. Serial monitor is 115200 baud; typing a line and Enter starts a session with that text. An Arduino MCP server (`arduino`) is also registered in Claude Code, which tools other than Claude cannot use.

## Safety rules (do not break)
These are the product's trust. Sources: `SOFTWARE_SPEC.md` (Approval section), `m1/src/bridge.mjs`, `m1/src/risk.mjs`. Any change to approval logic needs tests in `m1/test/` and an explicit note in STATUS.md.
1. **No answer means no approval.** Unanswered requests expire; silence is never consent. On the board they expire and move to the laptop page; on the direct route they are denied after 10 minutes.
2. **High risk is never approved on the device.** Delete, push, deploy, send, secrets, payments, paths outside the project, complex shell syntax and unknown commands are high risk. Only the laptop page (with a stronger check) can approve them. The device can still deny.
3. **Never truncate approval text.** If the full text does not fit (limit 140 characters, plain printable ASCII), it is not sent to the device, and the device cannot approve it.
4. **Unknown means high risk.** Anything not clearly understood is classified high.
5. **First answer wins; stale answers are rejected** (id and nonce checked). The device is never trusted: the bridge enforces every rule.
6. **Tokens stay off the device.** The board stores only the bridge pairing token (an HMAC proof, never sent over the network), no Claude, Codex or other agent credentials.
7. **Approve is deliberately hard on the board:** hold BOOT 1.5 s to approve, tap to deny, so a slip can only deny.
8. Every session must be created in an approval-required mode. T3's default is full access, so always set `runtimeMode: "approval-required"` when using T3.
9. v1 is drafts only for email and messages: no sending, paying or deleting.
10. Never claim every command is reviewed. Claude Code decides what asks for approval and runs some harmless commands without asking.

Secrets handling: never print, copy or commit `firmware/agent_link/secrets.h` (Wi-Fi password, bridge address, pairing token) or `m1/data/device-token`. Never put emails, passwords or tokens in documents or logs.

## Hardware quirks
Board: ESP32-S3 N16R8 dev board (16 MB flash, 8 MB PSRAM), 1.28" GC9A01 240x240 round display (no touch, no backlight pin). Wiring in `firmware/agent_link/pins.h`: SCL=8, SDA=7, CS=6, DC=4, RST=5, VCC=3V3. Planned later board: Waveshare ESP32-S3-Touch-LCD-1.46B (not bought).
- **SPI must stay at 20 MHz or lower** on the breadboard. 40 MHz gave a white screen.
- **Do not call `pinMode` on display pins before `gfx->begin()`.** An uninitialised panel looks white with faint vertical lines; that is not a power fault. A test that did GPIO checks before init stayed white for reasons not fully explained.
- FQBN is `esp32:esp32:esp32s3:CDCOnBoot=cdc`.
- Flashing: with this firmware on, `arduino-cli upload` resets the board by itself. The factory firmware needed BOOT and RST pressed by hand. If the app does not start after flashing, tap RST, or use esptool with `--before no-reset --after watchdog-reset` (esptool is not on PATH here; it ships inside the esp32 Arduino core).
- Wi-Fi is 2.4 GHz only. On an iPhone hotspot turn on Maximize Compatibility.
- Audio parts owned: MAX98357A I2S amp, INMP441 I2S mic, speaker (model and rating unconfirmed; do not run at full volume until confirmed). I2S pins are not chosen. Avoid GPIO 4-8 (display), and 35-37 (octal PSRAM on N16R8). Also avoid GPIO 0 (BOOT button, used by the firmware) and the USB pins 19-20 *(interp.)*. Use 3.3 V for the INMP441, not 5 V.
- No battery, buttons (only BOOT), vibration motor or IMU yet.

## Electronics and firmware habits
- Derate parts; prefer parts the assembler stocks (JLCPCB/LCSC basic first), with a datasheet you have read. Record MPN and supplier on every part.
- Add test points, an LED per main rail, and a debug header on any real board. Keep SWD/UART access.
- Add pull-ups or pull-downs on reset, boot, enable and open-drain lines; never leave inputs floating.
- Keep logic levels matched across every link. Check the datasheet before wiring a new module.
- Make changes in small steps; compile and test after each. Never claim a check passed without reading its real output.

## Working style
- The founder dictates by voice, so product and company names may be misheard (for example "Grogbot"). Confirm a name before relying on it.
- Write plain, clear English. Short sentences. No hype.
- Mark guesses and assumptions as *(interp.)*, and say what is unverified.
- **Do not commit, push, delete files or install packages unless asked.** Do not start long-running servers or open network access (such as Tailscale) without asking.
- Verify with real output (test run, compile result, serial log) before saying something works. Say what you could not verify.
- Do not buy from ElectroPi (non-delivery complaints, no pickup). The founder was calling Tenettech in Bangalore about the 1.46B board.
- Do not edit `firmware/agent_link/` without checking whether another agent is working there; read it fresh first.
- Do not rename the product around Claude's name. Buyers sign in to unmodified Claude Code themselves.
- Update `STATUS.md` when you finish work (date at the top).

## Docs: which to trust
Newest wins. `SOFTWARE_SPEC.md` says the phone app and Bluetooth are v1, and `FEATURES.md` Tier 2 assumes a phone link; both predate the 2026-11-07 Wi-Fi-to-laptop decision. Their approval rules and risk table still apply. `PLAN.md` is superseded. `M1_RESULTS.md` says 35 tests; there are now 53.
