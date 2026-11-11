# Tomagachi – AI pet companion (dev-board prototype)

> **Superseded for software:** Phases 2 and 3 are replaced by [SOFTWARE_SPEC.md](SOFTWARE_SPEC.md). The product is now an agent remote, not an LLM-backed pet. Hardware and budget are replaced by [HARDWARE_BUDGET.md](HARDWARE_BUDGET.md).

Goal: a palm-sized handheld with a round screen showing an animated character that reacts to you (touch, shake, time of day) and chats via an LLM over WiFi. Inspired by the video's gadget. Original character, original shell.

## Phase 1 – Hardware (≈ 1 evening)
Buy one all-in-one board, no custom PCB:
- **Waveshare ESP32-S3-Touch-LCD-1.28** (or similar): ESP32-S3, 1.28" round 240×240 GC9A01 display, capacitive touch, QMI8658 IMU (shake/tilt), LiPo charger. *(interp.: verify exact board and pinout on the Waveshare wiki before ordering)*
- 3.7 V LiPo, ~300–500 mAh, JST 1.25 to match the board (check connector polarity before plugging in!)
- Optional for voice (Phase 3): I2S MEMS mic (INMP441) + MAX98357A amp + 28 mm speaker; a small piezo buzzer is the cheap first step.

Shell: ~50–60 mm blob, two halves, screen window, USB-C opening, button cutout. Measure the real board with calipers first; print in translucent PETG/resin-like filament for the glow effect. Expect 3–5 print iterations.

## Phase 2 – Firmware (PlatformIO + Arduino, or ESP-IDF)
1. Display + touch hello world (LVGL or LovyanGFX).
2. Character engine: state machine (idle, happy, sleepy, hungry, curious) with sprite/vector animations at ~30 fps.
3. Inputs: tap = pet, shake = dizzy, face-down = sleep (IMU), time of day = mood.
4. Persistent state in flash (hunger, affection, age).
5. WiFi + HTTPS client.

## Phase 3 – AI brain
- Device sends compact state + event ("user tapped 3x, mood=sleepy") to a tiny backend (Cloudflare Worker / small Python server) that holds the API key. **Never put the API key on the device.**
- Backend calls a small, fast Claude model (Haiku 5.5, `claude-haiku-5-5`) with a character system prompt; returns a short line (≤ 60 chars) plus an emotion tag the firmware maps to an animation.
- Later: voice (mic → speech-to-text → LLM → TTS), and the "agent status" mode from the video (backend polls your coding agents and sends their state).

## Phase 4 – Polish / optional custom PCB
Once the prototype is loved, design a custom board in KiCad (ESP32-S3 module, round display connector, battery management, test points, SWD/UART header) and order from JLCPCB.

## Key checks
- Boots, shows animation at ≥ 20 fps
- Touch and shake both trigger distinct reactions
- ≥ 8 h standby on the LiPo
- LLM reply round trip < 3 s on WiFi
- Assembled shell closes without pinching the battery

## Open items
- Character design (shape, colours, name)
- Voice: yes/no for v1
- Budget (dev board route ≈ $30–50 total)
