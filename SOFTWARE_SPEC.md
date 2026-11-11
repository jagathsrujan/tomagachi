# Software spec v1 – pocket agent remote

Status: proposed, not yet tested. Facts about T3 Code, Claude Code and Codex come from [FEASIBILITY.md](FEASIBILITY.md). Items marked *(assumption)* need checking. Latency and battery figures are targets.

## Update, 2026-11-06
Version 1 now leads with older alumni and admin tasks by voice, built on a thin fork of T3's mobile app. See [OLDER_ALUMNI_PLAN.md](OLDER_ALUMNI_PLAN.md) and [FORK_PLAN.md](FORK_PLAN.md). Where they differ from this spec, they win. The device firmware, Bluetooth protocol, approval rules and security sections still apply.

## Product in one line
A pocket device that shows what your Claude Code and Codex work is doing, lets you approve or deny it, and lets you start new tasks, without opening a laptop or phone.

## Version 1 scope
**In:** status, approvals, delegation by preset and voice, custom agent profiles, personal tasks through harness connectors.

**Out:** Hermes, OpenClaw, Duet, Dots, your own cloud or accounts, terminal-started sessions, iOS, the full pet game.

## System
```
Laptop: Claude Code + Codex + connectors
           |
        T3 Code server (Supervised threads, unmodified)
           |  T3 Connect or Tailscale
Android app: T3 client, risk rules, voice-to-text, profiles
           |  encrypted Bluetooth LE
Device: screen, buttons, haptics, mic
```
You run no servers, models or API keys. Users keep their own Claude and Codex logins.

## Components

### 1. Laptop setup (no custom software in v1)
- Install T3 Code, Claude Code and Codex; log in to each.
- Run T3 as a background service.
- Set new threads to **Supervised**, since T3's default is Full access.
- Pair the phone with a limited-scope token by QR code.
- Create a `personal` project for email, calendar and notes tasks.

### 2. Phone app (thin fork of T3's mobile app)
- **Why this stack:** T3's mobile app is Expo and React Native, MIT licensed, and already covers pairing, voice to text and notifications. Android first; iPhone is possible from the same code.
- **Main addition:** a Bluetooth link to the device, kept in its own module.
- Runs as a foreground service to hold both the T3 and Bluetooth connections.
- Stores the T3 token in Android Keystore.
- Screens: pairing, profiles, presets, activity log, full approval view, settings.
- Converts speech to text on the phone, using T3's voice module if device audio can feed it *(assumption)*.
- Turns T3 thread events into short device messages.

**Adapter, so T3 is replaceable:**
```ts
interface AgentBackend {
  connect(): Promise<void>;
  close(): void;
  subscribe(onEvent: (e: AgentEvent) => void): Unsubscribe;
  listProjects(): Promise<Project[]>;
  startTask(t: TaskRequest): Promise<ThreadId>;
  sendMessage(id: ThreadId, text: string): Promise<void>;
  stopTask(id: ThreadId): Promise<void>;
  getSummary(id: ThreadId): Promise<string>;
  getApproval(id: ThreadId, requestId: string): Promise<ApprovalDetail | null>;
  answerApproval(id: ThreadId, requestId: string, d: "approve" | "deny"): Promise<void>;
}
```
Implementations: `T3Backend` in v1, `HooksBackend` as fallback, `HermesBackend` later.

### 3. Device firmware (ESP-IDF, LVGL, NimBLE)
Holds no agent tokens and makes no internet connection.

| Screen | Shows | Controls |
|---|---|---|
| Home | Count of working, waiting and done tasks | Scroll, select |
| Task list | Agent, project, state, age | Open |
| Approval | Agent, risk, exact action | Approve, deny, send to phone |
| Task detail | Latest short summary | Reply by voice, stop |
| Delegate | Profiles and presets | Select, push to talk |

- **States:** Idle, Working, Needs you, Done, Failed, Offline.
- **Alerts:** a distinct vibration for Needs you, Done and Failed. Quiet hours set in the app.
- **Privacy:** message and email text stays hidden until the user opens it.
- **Character:** a simple face reflecting the state, nothing more in v1.
- **Storage:** recent summaries in RAM only.

## Bluetooth protocol
Custom GATT service, LE Secure Connections with bonding. Versioned compact messages.

| Direction | Message | Key fields |
|---|---|---|
| Phone to device | `state.snapshot` | tasks, counts, connection state |
| Phone to device | `task.update` | id, state, summary |
| Phone to device | `approval.request` | id, nonce, agent, project, risk, text, expires |
| Phone to device | `approval.cancel` | id, reason |
| Phone to device | `approval.result` | id, status, decision |
| Phone to device | `profiles.sync` | profiles, presets |
| Phone to device | `task.started`, `task.error` | ref, id or reason |
| Device to phone | `approval.response` | id, nonce, decision |
| Device to phone | `task.start` | ref, profile, preset or text |
| Device to phone | `task.reply` | id, text |
| Device to phone | `task.stop` | id |
| Device to phone | `audio.start`, `audio.frame`, `audio.end` | session, sequence, data |
| Device to phone | `device.status` | battery, firmware, last message |

## Core flows

### Approval
1. T3 thread pauses for approval.
2. The app rates the risk with local rules and sends the request.
3. The device vibrates and shows the exact action.
4. The user answers; the app checks the id and nonce, then answers T3.
5. The device confirms only after T3 accepts.

Rules:
- No answer means **no approval**. The request expires on the device and stays pending in T3 or is denied.
- If an action doesn't fit the screen, it goes to the phone. Never approve truncated text.
- The first answer wins, and stale answers are rejected.

| Risk | Examples | Where it can be approved |
|---|---|---|
| Low | Read files, run tests, lint | Device |
| Medium | Edit files, local commits | Device |
| High | Delete, push, deploy, send email or message, secrets, payments, unknown commands | Phone with biometric check, or laptop |

Unknown commands count as high risk.

### Delegation
1. Pick a profile or preset, or hold to talk.
2. The phone transcribes speech, and the device shows the text.
3. Confirm, then the app starts a Supervised T3 thread.
4. Coding tasks use a separate worktree where the project supports it.

### Personal tasks
These run as T3 threads in the `personal` project through the harness's connectors. Drafting and reading are allowed; sending, inviting and deleting are high risk.

## Custom agent profile
```json
{
  "name": "PR Reviewer",
  "icon": "eye",
  "harness": "codex",
  "project": "~/work/api",
  "permissionMode": "supervised",
  "prompt": "Review the latest PR and list real bugs.",
  "presets": ["Review latest PR", "Run tests"]
}
```

## Security
- Tokens stay on the phone, limited scope, revocable. Treat them as shell access: `orchestration:operate` can start a full-access agent.
- T3 is never exposed on the open internet.
- Lost device: unbond it in the app. It holds no credentials.
- Device PIN or phone-nearby check before approvals.
- Every approval is logged in the app.
- Default deny if connections drop.

## Targets
- Approval on device within 3 s of T3 raising it, on a good connection.
- Answer reaches T3 within 2 s.
- Reconnects within 10 s after a dropped link.
- No missed approvals across 100 test requests.
- A full day of battery in normal use.

## Build order

| Milestone | Build | Done when |
|---|---|---|
| M0 | T3 test script | Script starts Claude Code and Codex threads, reads status, approves and denies |
| M1 | Web device simulator and `AgentBackend` | Whole flow works in a browser |
| M2 | Android app and Bluetooth | Real board shows live status |
| M3 | Approvals with risk rules | All approval rules pass tests |
| M4 | Presets, then voice | A task starts from the device |
| M5 | Personal tasks and profiles | Calendar and email tasks work safely |
| M6 | Pilot with 5 alumni | Two weeks of daily use and feedback |

**If M0 fails:** switch to `HooksBackend`, which adds a laptop app and your own relay.

## Open questions
- ~~Can a third-party client answer approvals through T3's RPC?~~ Yes, passed for Claude Code and Codex. See [M0_RESULTS.md](M0_RESULTS.md).
- Can a third-party client use T3 Connect? No documented way: it needs T3's cloud sign-in. Test Tailscale next. See [M1_RESULTS.md](M1_RESULTS.md).
- How long does an unanswered approval last? At least 6 minutes; upper limit unknown. It does not survive a T3 restart: the run is cancelled and the command never runs.
- Which commands does each harness allow without asking? Claude Code ran `sleep` with no approval.
- ~~Should the app use T3's `/mcp` route instead of the RPC token?~~ No. By design `/mcp` cannot approve permission requests, so the app needs the RPC token.
- Does Android reliably keep the connections alive in the background? (M2)
- Neck-worn or pocket? This affects mic and buttons.
- Do T3 and connector use meet each tool's terms for this product?
