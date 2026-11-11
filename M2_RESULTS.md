# M2 results – board to laptop over Wi-Fi

**Date:** 2026-11-07 · **Result: passed for the tests below** · Board: ESP32-S3 N16R8, 1.28" GC9A01 · Claude Code direct through the Agent SDK · iPhone hotspot · model `claude-haiku-5-5`

The real board created Claude Code sessions over Wi-Fi with no phone and no T3. Results come from the board's serial log, `m1/data/activity.jsonl` and the sandbox folder, not only from what was seen on screen.

| Test | Result |
|---|---|
| Board joins the hotspot and pairs with the bridge | Pass, from 172.20.10.5 |
| Tap changes the preset | Pass |
| Hold starts a session | Pass; `List files` ran with no approval, because Claude Code allows `ls` |
| Tap denies a medium-risk command | Pass; `touch` did not run |
| Hold approves a medium-risk command | Pass; `note-from-device.txt` created at 14:42 |
| Hold on a high-risk `rm` | Pass; the board logged `approvable=0` and nothing was sent |
| Tap denies a high-risk `rm` | Pass; the file stayed |

**Speed:** `requested` came about 3 s after `task_started` on warm runs. One earlier direct run on loopback took about 25 s, so this may be a first-session cost. The first session after a bridge start has not been timed.

**Display load:** frames took about 5 ms to draw and send, about 15 ms for the first full paint. Screen traffic was about 1–16 ms per second depending on movement.

## Not tested on hardware
- Approving a high-risk request on the laptop page while the board is connected (passed earlier with the fake device).
- A request nobody answers: expiry on the board after 2 minutes, then denial after 10 minutes.
- Wi-Fi or laptop dropping out while an approval is on screen.
- The T3 route from the board (`npm run link:t3`).
- Long approval text on the real glass, and whether the small font is readable.

## Open observations
- The link dropped once and reconnected by itself after the 14:42 approval, near two long BOOT presses of 6 s and 3 s. Cause unknown.
- The screen background looked bright blue in a photo. It may be the camera; not confirmed by eye.
- Two early `List files` runs happened when Create note was intended. The likely cause is that the hold began before the label changed, but this was not proven. The logged taps all worked.
