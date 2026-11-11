# Feature list: breadboard prototype

Written 2026-11-07. Built from the parts in "Hardware on hand" in [PROJECT_BRIEF.md](PROJECT_BRIEF.md) and the intent in [OLDER_ALUMNI_PLAN.md](OLDER_ALUMNI_PLAN.md) (admin tasks by voice, large text, spoken replies, one button, no setup burden), [SOFTWARE_SPEC.md](SOFTWARE_SPEC.md) (device holds no tokens, BLE to phone) and [PITCH_PROTOTYPE.md](PITCH_PROTOTYPE.md) (honest 60-second video). Items marked *(interp.)* are my choices.

## Hardware I can use
ESP32-S3 N16R8, 240x240 round GC9A01 (no touch), INMP441 mic, MAX98357A amp, one speaker. **Missing:** buttons, battery, vibration motor, IMU. Buttons are cheap and I'll add them to the wiring plan; the rest are out of scope for now.

## Tier 1: build now, device only (no phone, no internet)
| # | Feature | Uses | Done when |
|---|---|---|---|
| 1 | **Face with states**: idle, listening, thinking, speaking, error, offline. Large readable text overlay for older eyes | Display | Each state shown on the round screen at 18+ fps |
| 2 | **Speaker output**: play tones and chimes (ready, done, needs you) and a stored test phrase | Amp, speaker | Clear sound, no hum or clipping, volume set on the gain pin and in code |
| 3 | **Microphone capture**: 16 kHz mono from the INMP441, with a live level meter on the face | Mic, display | Speaking makes the meter move; silence reads flat |
| 4 | **Push to talk**: hold a button to record, release to send. The face shows listening while held | Button, mic, display | Works 20 times in a row with no stuck state |
| 5 | **Voice activity detection** *(interp.)*: stop recording after a pause as a backup to the button | Mic | Trims silence without cutting off words |
| 6 | **Audio loopback**: record, then play it back | Mic, amp, speaker | Heard clearly. This proves both halves of the audio chain |
| 7 | **Approve and deny buttons**: a two-button yes/no for requests, with the face and a chime confirming | Buttons, display, speaker | Demo of the approval screen from the spec, with local fake requests |
| 8 | **Large-text reply screen**: short answers wrapped for the round screen, scrolling by button | Display | A 140-character reply shows in full without truncation |

## Tier 2: needs a phone link
| # | Feature | Notes |
|---|---|---|
| 9 | **BLE link to phone** with bonding and "Offline" state | Spec protocol: `state.snapshot`, `task.update`, `device.status` |
| 10 | **Stream mic audio to the phone** as compressed frames over BLE | Phone turns speech into text, per the spec. Needs a codec on the ESP32 *(interp.: Opus or ADPCM)* |
| 11 | **Spoken replies**: the phone sends text or compressed audio, the device plays it | Older alumni likely need this; stays an open decision per the plan |
| 12 | **Approvals from a real agent**: `approval.request` and `approval.response` with nonce, expiry, and no answer meaning no approval | High-risk actions are sent to the phone, never approved on the device |
| 13 | **Task status screen**: working, waiting, done, failed | Reuses the face states |
| 14 | **Reminder and calendar answers**: "what's on tomorrow?" shown and read aloud | Depends on the answer-return route (options A-D in the older alumni plan) |

## Safety rules built into every feature
- The device stores no agent tokens and makes no internet calls.
- Unanswered requests expire and count as denied.
- Text that doesn't fit the screen goes to the phone; I never show a truncated approval.
- Mic is active only while the button is held (or during an on-screen listening state), and the face always shows when it is.
- Email and messages stay drafts; no sending, paying or deleting in v1.

## Not building now
Vibration alerts, battery and charging, touch input, wake word, always-on listening, pet or game features, the iOS app, and any custom board or shell.

## Build order
1. Tier 1 items 2, 3 and 6 (audio chain test), then 1, 4 and 8.
2. Items 7 and 5.
3. Phone link (9), then voice to phone (10), then spoken replies (11).
4. Real approvals and tasks (12-14) once the phone app and answer route exist.

## Open choices for you
- Add two buttons to the breadboard (I suggest one push-to-talk and one approve/deny pair).
- Keep the Arduino framework for the breadboard; the spec's ESP-IDF and LVGL move can wait for the real board.
- Confirm the speaker rating before testing loud playback.
