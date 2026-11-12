# STATUS

Last updated: 2026-11-07. **Any tool that finishes work must update this file (date, what changed, what you verified) before stopping.** Rules and commands are in `AGENTS.md`.

## Where we are
Version 1 = ESP32-S3 board links straight to the laptop over Wi-Fi (no phone). The laptop bridge creates Claude Code sessions through the Agent SDK by default, or through T3 Code with `--t3`. The link passed its first tests on the real board on 2026-11-07.

## Verified (saw real output)
- Real board over Wi-Fi: pairing, starting sessions, tap to deny, hold to approve, and refusing a high-risk approval. See `M2_RESULTS.md`.
- `npm test` in `m1/`: 53 of 53 pass (re-run 2026-11-07).
- A fake device (`m1/scripts/fake-device.mjs`) on loopback ran real Claude Code sessions both directly and through T3: start a task, approve, a refused high-risk approval from the device, and a laptop-page approval. (Founder's earlier session; not re-run in this setup pass.)
- M0 and M1: approve and deny worked for Claude Code and Codex through T3 on this laptop. See `M0_RESULTS.md`, `M1_RESULTS.md`.
- Breadboard display: face animation runs on the GC9A01 at 20 MHz SPI, 18 fps *(2026-11-06)*.

## Unverified
- Hardware gaps listed in `M2_RESULTS.md`: unanswered-request expiry, link loss during an approval, the T3 route from the board, small-font readability.
- Remote access (Tailscale) and T3 Connect from a third-party client.
- Longest time an unanswered T3 approval stays valid (at least 6 minutes).
- Real voice, audio hardware, battery life.

## Next steps, in order
1. Finish the hardware gaps in `M2_RESULTS.md`.
2. Fix the INMP441 noise, then wire the MAX98357A amp on GPIO 15-17 *(interp.)*. Mic wiring: SCK=12, WS=13, SD=14, L/R=GND, VDD=3V3. Test tones, mic level, loopback. Then voice, following `FEATURES.md` order. Confirm the speaker rating first.

## Known issues
- Mic is unproven: `firmware/mic_test` reads clean left-channel data, but QUIET and TALK levels were both about 100 dB on its scale (a silent room should be about 40 lower). A 200 Hz high-pass barely helped. Check soldering on the mic pins first, then redo the QUIET/TALK test in a quiet room. The board currently has `mic_test` on it, not `agent_link`; reflash `agent_link` to use the link.
- The direct route took about 25 s from start to first approval. Cause unknown.
- Direct sessions load no personal settings (`settingSources: []` in `m1/src/claudeBackend.mjs`), so connectors, allow rules and CLAUDE.md are not loaded. Calendar and email tasks need this changed, with the safety effect thought through.
- Link to the board is not encrypted (token itself never crosses the network). Trusted Wi-Fi only.
- Speaker model unconfirmed.
- Claude Code decides what asks for approval; some harmless commands run unasked.
- File-change approvals show the path, not the change.
- Using a personal Claude login through the Agent SDK needs a terms check before selling.
- The laptop must stay on and be on the same network.
- Docs partly out of date: see "Docs: which to trust" in `AGENTS.md`.

## Open decisions (founder)
- Who the first buyers are: older alumni (admin by voice) or younger coders. Deposits decide (`OLDER_ALUMNI_PLAN.md`).
- Do older alumni have a computer that stays on?
- What "Grogbot" means (dictated; unconfirmed: possibly Clawdbot/OpenClaw or a Grok product).
- Whether to switch direct sessions to load personal settings for calendar tasks.
- Spoken replies, speaker, buttons to add, pocket vs worn, final board (Waveshare 1.46B vs this breadboard).
- Whether to start a git repo (recommended, see report; nested repos `m0/sandbox-repo` and `m1/sandbox-repo` exist).

## Log
- 2026-11-12: thumbwheel added. CAD: 9 mm pot in the back shell at the neck, 21 mm wheel through the right side wall (`cad/README.md`). Firmware `agent_link`: new `ui_dial.h`, `pollDial()` reads the pot on GPIO 1 and sets the preset selection; hints read "Turn: pick" when `DIAL_ENABLED` is 1 (`pins.h`). The wheel never answers approvals. **Not compiled and not run**: no arduino-cli or g++ on this Windows machine. Only a JavaScript copy of the zone logic was simulated. With no pot wired, set `DIAL_ENABLED 0` or the floating pin will move the selection. The preview check (`preview/run.sh`) was not re-run.
- 2026-11-12: handheld shell v1 modelled in FreeCAD (`cad/`, see `cad/README.md` for wiring and assembly). Round head, 3 Outemu keys, pinned DevKitC, USB only, M3 inserts. The script's own check reports valid solids and 0 clashes against stand-in parts. Not printed; all part sizes are unmeasured guesses. Key pins 9/10/11 and amp pins 15/16/17 are proposals, not in firmware.
- 2026-11-12: keycaps added (`cad/keycap.py`, Talk bar / Approve dome / Deny dish, 17.6 mm square, socket from the measured stem +0.08 per side). `remote_v1.py` places them; its check reports 0 clashes at rest and at 4 mm full press against stand-ins, and against the real switch STEP (tightest gap 0.10 mm pressed). STLs in `cad/out/`. Not printed; fit unverified.
- 2026-11-07: mic wired and tested; testing stopped with the noise unresolved.
- 2026-11-07: first hardware run of the Wi-Fi link passed; see `M2_RESULTS.md`.
- 2026-11-07: the board character is now the kangaroo from `firmware/kangaroo_animation`, drawn in `firmware/agent_link/ui_face.h`; compiles and passes the preview checks, not yet run on hardware.
- 2026-11-07: board UI rewritten with dirty-tile updates and state-based frame rates; compiles, not yet run on hardware. Check size-1 text legibility on the real glass.
- 2026-11-07: workspace set up for multi-tool handoff (`AGENTS.md`, `STATUS.md`, pointer files, `.gitignore`).
