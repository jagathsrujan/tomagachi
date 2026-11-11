# M1 results – web device simulator

**Date:** 2026-11-05 · **Result: passed on this laptop** · Remote access not tested

## What was built
| Part | File |
|---|---|
| Agent contract | [m1/src/backend.mjs](m1/src/backend.mjs) |
| T3 Code backend | [m1/src/t3Backend.mjs](m1/src/t3Backend.mjs) |
| Risk rules | [m1/src/risk.mjs](m1/src/risk.mjs) |
| Phone logic and approval rules | [m1/src/bridge.mjs](m1/src/bridge.mjs) |
| Local Bluetooth stand-in | [m1/src/server.mjs](m1/src/server.mjs) |
| Device and phone simulator | [m1/public/](m1/public/) |

Run it with `node m1/scripts/dev.mjs`, then open http://127.0.0.1:3918. Unit tests: `npm test` in `m1/` (35 of 35 pass).

## Tested with real agents
| Test | Harness | Result |
|---|---|---|
| Start a preset task from the device | Claude Code | Pass |
| Start a spoken task after confirming on device (typed stand-in) | Codex | Pass |
| Approve a medium-risk command on the device | Both | Pass, file created |
| High-risk `rm` is blocked on the device and sent to the phone | Claude Code | Pass |
| Deny on the phone | Claude Code | Pass, file kept |
| Approve on the phone with simulated fingerprint | Both | Pass |
| Reply to a finished task | Codex | Pass after a fix |
| Task summary reaches the device | Codex | Pass |
| Stop a task waiting for approval | Claude Code | Pass, approval withdrawn, command never ran |
| No answer for 2 minutes | Both | Device timed out, nothing ran |
| Late device answer after timeout | Both | Ignored, nothing ran |
| Reloaded device gets open approvals again | Codex | Pass |
| T3 restarted during a pending approval | Claude Code | Run cancelled, command never ran |
| Other websites posting to the simulator | n/a | Blocked (origin, host, content type) |

## Approval lifetime
- Approvals were still answerable after about 3 min 50 s (both harnesses) and 6 min 13 s (Codex).
- **The planned 10-minute check did not happen.** The test's approvals were answered through the simulator's phone panel at about 4 minutes, by someone using the pane rather than by the script. The longest lifetime is therefore still unknown.

## Bugs found and fixed
- T3's thread HTTP route did not return usable JSON, so command text was missing. The safe fallback blocked device approval. Now read over RPC.
- A project-only follow-up snapshot from T3 wiped the task list.
- Replies failed without `dispatchMode`.
- Codex's unquoted `/bin/zsh -lc ls` was rated high risk.
- The phone panel redraw could swallow taps.

## New findings
- **Supervised does not mean every command asks.** Claude Code ran `sleep 120` with no approval, and harnesses may allow other commands too. The device can only show what the harness asks about, so marketing must not promise that every command is reviewed.
- **`/mcp` cannot approve.** T3 says this route grants no approval rights, so the RPC token stays. Treat it as shell access.
- **T3 Connect needs T3's cloud sign-in** (Clerk, with client keys). A third-party app has no documented way in, so plan on Tailscale or a pairing link, or a T3 fork.
- **Risk text:** `rm` shows "Unknown command", which is safe but unclear.
- **Older tasks lose summaries after a restart**, because T3's list has no latest message for them.

## Not tested
- Remote access. Tailscale is stopped on this laptop, and starting it changes network settings, so I left it.
- A real phone, Bluetooth or board.
- File-change and subagent approvals.
- A dropped link without a T3 restart, beyond unit tests.
- Real voice; it is typed text in the simulator.

## Simulator state
It is still running on loopback, with T3 on port 3927. Its test token lasts up to 12 hours.
