# M0 results – T3 Code test

**Date:** 2026-11-05 · **Result: passed** · T3 Code 0.1.7 · Claude Code 2.1.316 · Codex CLI 0.159.3

## What was tested
A small script, [m0/m0-test.mjs](m0/m0-test.mjs), acted as a third-party client. It connected with a token limited to `orchestration:read` and `orchestration:operate`, started real Supervised threads in a throwaway git repo, waited for each approval, answered it, and checked on disk whether the command ran.

| Harness | Model | Answer | Approval arrived | Answer accepted in | Command ran? | Result |
|---|---|---|---|---|---|---|
| Claude Code | claude-haiku-5-5 | Approve | 3.8 s | 9 ms | Yes | Pass |
| Claude Code | claude-haiku-5-5 | Deny | 2.5 s | 9 ms | No | Pass |
| Codex | gpt-6-luna | Approve | 7.2 s | 7 ms | Yes | Pass |
| Codex | gpt-6-luna | Deny | 6.3 s | 7 ms | No | Pass |

Times are from launch and include agent start-up and thinking. All runs were on loopback, one run each. Deny cases waited 25 s more to catch late execution.

Two earlier Codex runs failed because I chose `gpt-6-astra`, which this ChatGPT login does not support. T3 reported the error as a failed thread. The test script timed out rather than noticing, so the real client must handle failed turns.

## What this proves
- A third-party client can start threads in both harnesses through one interface.
- Status streams through `orchestration.subscribeThread`.
- Approvals arrive as `approval_request` items, with the command text in `prompt`.
- `runtime-request.respond` approves and denies. **This was the biggest open question.**
- A read-only token is refused when it tries to approve.
- Personal Claude and Codex settings were not changed. T3 used its own data folder.

## New findings
- **The API is reachable with plain WebSocket and HTTP.** No T3 libraries were needed. The handshake needs `orchestrationProtocol=2` and a ticket from `/api/auth/websocket-ticket/rpc`.
- **The limited token is not a strong boundary.** T3's docs say `orchestration:operate` can start a full-access agent, which can then issue a token with any scope. Treat the phone's token as equal to shell access on the laptop.
- **T3's `/mcp` route may be safer.** It supports a signed runtime-mode limit, and tools such as `t3_thread_launch` and `t3_pending_request`. Not tested yet; evaluate in M1.
- **The default mode really is Full access.** Every launch must set `runtimeMode: "approval-required"`.
- **Test runs appear in local history:** one Claude project entry and four Codex session files mention `sandbox-repo`.

## Not tested
- T3 Connect or Tailscale; everything ran on this laptop.
- Whether a third-party client can use T3 Connect.
- How long an unanswered approval stays valid, and what happens if T3 or the connection restarts.
- File-change approvals, subagent approvals and non-command requests.
- Reconnecting and replaying missed events.
- Long commands, and a real phone or device.

## Cleanup
The test server is stopped and the token file is deleted; the token would have expired within 2 hours anyway. `m0/` still holds the script, logs, throwaway repo and T3 test data, and can be deleted.
