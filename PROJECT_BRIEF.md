# Project brief – voice agent companion

> **Start here:** read [AGENTS.md](AGENTS.md) (rules, repo map, commands) and [STATUS.md](STATUS.md) (current state, next steps) first. To run the bridge without Claude Code's launch config, run `npm run link` in `m1/` (or `npm run link:t3` to go through T3).

Handoff for new sessions. Written 2026-11-06. Read this first, then the linked files as needed.

## Product vision (founder's words, summarised)
A small round wearable or pocket device for UVCE alumni. Using voice, on the go, without a laptop or phone tapping, the buyer can:
1. **Delegate admin tasks:** calendar, reminders, search, information, answers and queries.
2. **Talk to running agent sessions:** Claude, ChatGPT, Claude Code, Codex, Grok Build and Cursor. Get results, give prompts, approve permissions.

**Must be very user friendly** for the buyer. **Intended moat:** smoother integration across many environments, open, and not tied to one company.

## Decided
- **Hardware:** ESP32-S3 with round display, mic, speaker and a few buttons, in a customisable 3D-printed case. First-choice board is Waveshare ESP32-S3-Touch-LCD-1.46B.
- **Price target:** build about ₹4,000, sell about ₹5,000.
- **Go to market:** working prototype, 60-second honest video, then pitch about 20 alumni before building more.
- **Founder's position:** does not want to run models, tokens or agent orchestration.
- **Version 1 link, decided 2026-11-07:** the ESP32-S3 connects straight to the laptop over Wi-Fi with no phone, and creates Claude Code sessions with or without T3. See [firmware/agent_link/README.md](firmware/agent_link/README.md).

## Open question from the founder
Which backend gives the easiest buyer experience across these agent products? The founder sees these as most used by alumni, and these claims are **not yet verified**:
- "Grogbot" is the dictated name, and it was a first mover with the largest share. **Confirm what this means:** possibly Clawdbot/OpenClaw, possibly a Grok product.
- Muse is free and Meta reportedly released an open-source charm SDK on GitHub. Muse is expected to be unavailable in India.
- Dots and Claude Duet have not open sourced anything similar.
- Someone on X built a Tamagotchi-style charm for "Grogbot".
- T3 Code, tested below, is a candidate backbone.

## Hardware on hand (breadboard prototype)
Added 2026-11-07. The founder already owns these; nothing here needs buying.
| Part | Details | Notes |
|---|---|---|
| ESP32-S3 N16R8 dev board | 16 MB flash, 8 MB PSRAM | Working, see memory `esp32-breadboard-setup` |
| 1.28" GC9A01 round display (oceanlabz) | 240x240 IPS, SPI. Pins: RST, CS, DC, SDA, SCL, GND, VCC | Wired and animating; keep SPI at 20 MHz or below. No touch, no backlight pin |
| MAX98357A I2S amplifier module (DELLZO) | I2S mono Class D, 3 W into 4-8 ohm, Vin 2.5-5.5 V, 9 dB default gain, outputs (L+R)/2. Pins: LRC, BCLK, DIN, GAIN, SD, GND, Vin | Drives the speaker. SD pin sets shutdown and channel select; GAIN pin changes gain |
| INMP441 I2S MEMS microphone module | Omnidirectional digital mic, 3.3 V. Pins: SCK, WS, SD, L/R, VDD, GND | L/R to GND gives the left channel, to VDD the right. Use 3.3 V, not 5 V |
| Speaker module | Model and impedance not yet confirmed *(interp.: 4-8 ohm, up to 3 W to suit the amp)* | Confirm the rating before running the amp at full volume |

Wiring plan *(interp.)*: mic and amp both use I2S, but they are separate buses or a shared clock with two data lines. The display uses GPIO 4-8, so I2S pins must avoid those. Pins are not yet chosen.

## What is proven
- **M0:** an outside client started Claude Code and Codex threads through T3 Code, and approved and denied real commands. See [M0_RESULTS.md](M0_RESULTS.md).
- **M1:** a web device simulator ran the full flow with both harnesses. See [M1_RESULTS.md](M1_RESULTS.md); code is in `m1/`.
- **Laptop link, 2026-11-07:** a fake device on loopback paired with the bridge and ran real Claude Code sessions, directly through the Agent SDK and through T3. The real board then passed pairing, starting sessions, deny, approve and refusing high-risk approvals on the direct route. See [M2_RESULTS.md](M2_RESULTS.md).

## Known limits
- T3 only sees threads started in T3, not terminal sessions.
- Supervised mode does not send every command for approval.
- T3's `/mcp` route cannot approve; the RPC token is effectively shell access to the laptop.
- T3 Connect has no documented route for third-party apps. Remote access is untested (Tailscale was off).
- The laptop must stay on for T3.
- Duet was a US-only preview; Dots reportedly needed a high-tier plan.
- Bluetooth standby current is unmeasured and decides battery life.
- Similar devices exist: Anthropic `claude-desktop-buddy`, Agent Buddy, Codex Micro, Meta Muse Charm.

## Key files
- [SOFTWARE_SPEC.md](SOFTWARE_SPEC.md): software v1 spec, may change with this question
- [FEASIBILITY.md](FEASIBILITY.md): research on agents and routes
- [HARDWARE_BUDGET.md](HARDWARE_BUDGET.md), [STAGE1_ORDER.md](STAGE1_ORDER.md): hardware and costs
- [PITCH_PROTOTYPE.md](PITCH_PROTOTYPE.md): video plan

## Buying note
Do not buy from ElectroPi: non-delivery complaints, Gurugram address, no pickup. The founder is calling Tenettech in Bangalore about the 1.46B.
