# Software feasibility check – agent device as an interactive layer

Date: 2026-11-05. Desk research only: nothing here has been run on a phone, a board or an agent. Most sources are secondary, so check official docs before relying on a detail.

## Latest direction: T3 Code as the backbone

T3 Code (MIT, about 47.8k stars) is a control surface for Claude Code, Codex and other coding agents on the user's own machine. Users keep their existing subscriptions. It has web, desktop, iOS and Android clients.

**What it replaces:** most of the laptop companion and relay described in the next section. It already runs both harnesses, streams thread state, and reaches the laptop remotely through T3 Connect, a private network, or Tailscale.

```
Claude Code / Codex  ->  T3 server on laptop  ->  T3 Connect or Tailscale
                                                          |
                              Device  <-Bluetooth->  Your phone app (T3 client)
```

**Limits**
- **Only T3 threads are visible.** Sessions started in a terminal are invisible to T3. PR #5499 would add import but is unmerged. Users must start work in T3, or we add Claude and Codex hooks later.
- **No promised third-party API.** The RPC contract exists in `packages/contracts/src/rpc.ts`, but no stability promise was found. The project says "Expect bugs" and mostly does not accept contributions. MIT allows a fork as a fallback.
- **Approval over RPC confirmed by test.** `runtime-request.respond` approved and denied real Claude Code and Codex commands. See [M0_RESULTS.md](M0_RESULTS.md).
- **Default is Full access**, with no approval prompts. Our setup must switch threads to Supervised.
- **Pairing tokens get every permission by default.** Issue limited `--scope` tokens, and store them in the phone app, not the device.
- **The host must stay running.** T3 can run as a background service, but the laptop must stay on.

### Is Hermes needed?
**Not for version 1.** Email, calendar and notes can run as T3 threads using the harness's own connectors:
- Claude Code can use connectors set up on claude.ai.
- Codex plugins list Gmail and Google Calendar (third-party sources).

Add Hermes only for work that must continue while the laptop is off, or for scheduled personal tasks on an always-on server. It adds Google OAuth setup, a reported Gmail account block, and its own security issues.

## Claude Code and Codex details (direct route, fallback to T3)

Most users will work in Claude Code or Codex, so these come first. Hermes and OpenClaw become later add-ons, and the Nous plan is not needed.

| Need | Claude Code | Codex |
|---|---|---|
| Know what is happening | Hooks for session, tool, subagent, notification and stop events; can POST to a local server | Lifecycle hooks (third-party docs) |
| Approve or deny | `PermissionRequest` hook returns allow or deny; 600 s default timeout, configurable | `PermissionRequest` hook; no decision falls back to the normal prompt. Hooks must be trusted once |
| Delegate a new task | Companion starts a headless run. `claude remote-control` spawns sessions, but only from Anthropic's apps | `app-server`: `thread/start`, `turn/start`, plus approval requests. Marked experimental in some versions |
| Steer a session already open | Channels, if started with the flag and enabled; session must stay open | `turn/steer`, for threads run through app-server |

All Codex details come from third-party guides; OpenAI's engineering post could not be fetched.

### Architecture
```
Claude Code / Codex on the user's laptop
        | hooks, headless runs, app-server
Laptop companion (new, small)
        | encrypted relay when away; Bluetooth when near
Phone app  <-Bluetooth->  Device
```
Users keep their own logins. You run no models and hold no API keys. Requires: a laptop companion, a small relay, and a laptop that stays awake.

### Existing products (the niche is crowded)
| Product | Does | Missing, per what I read |
|---|---|---|
| Anthropic `claude-desktop-buddy` | Status and approve or deny over BLE via the Claude desktop app | Unsupported developer feature; no task sending described; needs to be near the computer |
| `codex-buddy-bridge` | Codex approvals on that same hardware | macOS, one connection at a time |
| Agent Buddy (MIT, name trademarked) | Status for Claude Code, Codex, Hermes, OpenClaw and more | No Bluetooth; USB or same Wi-Fi; approvals and tasks not described |
| Codex Micro (about €210) | Macro pad, status lights, launches workflows | Codex-focused; no display mentioned |
| VibeMon, AgentDeck, Clawdmeter | Status displays and dashboards | Mostly desk-bound |

**Apparent gap:** one portable device that works away from the desk across both harnesses, with status, approvals and delegation together. The real rival is the phone itself: Claude's app already reaches local sessions through Remote Control.

### Risks
- A remote approval runs commands on someone's laptop. Encrypt end to end, show the exact command, deny by default, and keep risky commands on the laptop.
- Never start runs with permission checks turned off.
- Hook and app-server interfaces change often. Check each tool's terms for automated use.

## Verdict (Duet, Dots, Muse, Instinct and open-source agents)

| Capability | Commercial agents (Duet, Dots, Muse, Instinct) | Open-source agents (OpenClaw, Hermes) |
|---|---|---|
| Show agent updates on device | Works (notifications or connector) | Works |
| Send a new request from device to agent | Weak: no public inbound route found for Duet or Dots | Works (OpenClaw gateway; Hermes API or chat gateway) |
| Approve or deny on device | Unconfirmed: depends on notification buttons | Both documented. Hermes has `POST /v1/runs/{run_id}/approval`; which actions trigger it needs testing |
| Works without the customer running a server | Yes | No: they run the agent and bring a model key |

**Conclusion:** the idea works in software. It works fully only on open-source agents. On commercial agents it is mostly a one-way display today.

## Routes into commercial agents

### 1. Notification mirror (no permission needed from the agent company)
- **Android:** a notification listener can read notifications. Gadgetbridge shows replies from a wearable work when the notification has a reply button. Android 15 hides notifications it flags as sensitive, such as 2FA codes.
- **iOS:** Apple's ANCS lets a Bluetooth accessory read notifications and trigger positive or negative actions, only where the notification flags them. Interactive forwarding to third-party wearables is EU-only (iOS 26.5).
- **Unknown:** whether Duet and ChatGPT notifications carry approve buttons at all, or just open the app. Only a real-phone test answers this.

### 2. Custom connector (official route)
Host a small remote MCP server with tools such as `show_on_device` and `ask_user`. The customer adds it to their agent. The agent company still runs the models and tokens.
- **Dots:** one developer report shows a dot running a custom connector unattended. Needs paid developer mode; a public listing would go through the Apps SDK.
- **Duet:** custom connectors reportedly do not work in scheduled runs. Chat use is unconfirmed.
- **Muse:** sources conflict on custom connectors.
- **Instinct:** nothing found.
- **Limits:** the agent chooses when to call your tool, so this is not a real approval gate; native approvals stay in the agent's app. Blocking while a human answers is fragile (around 60 s timeouts in some clients), so use request-then-check-later.

### 3. Inbound commands
No public way found to start a task in Duet or Dots from outside their apps. **Instinct is the exception:** it lives in SMS and WhatsApp, so messages can be mirrored and replied to on Android. Reply delivery needs testing (one Gadgetbridge user saw a WhatsApp reply fail).

## Open-source alternatives

| Project | Replaces | Device fit | Watch out for |
|---|---|---|---|
| **Hermes Agent** (MIT) | Duet and Dots style always-on agent, with stronger memory | Best. Plain HTTP API with bearer key: Runs with live events, approvals, sessions, scheduled jobs. Nous Portal paid plans include a cloud agent | Many 2026 CVEs, including a reported RCE. The API key gives full terminal access. Approvals over chat default to a 60 s timeout |
| **OpenClaw** | Same | Good. Documented WebSocket gateway with client roles, device pairing, and `approval.resolve` for exec and plugin approvals. More work for a client | Serious 2026 security history: RCE CVEs, exposed gateways, malicious skills |
| **xiaozhi-esp32** (MIT) | Muse Charm style firmware, not an agent | Voice, display and MCP firmware for ESP32-S3, with a pendant build listed | Link to OpenClaw or Hermes unverified |
| Goose, Agent Zero, Khoj | Narrower use | Lower priority | Not checked in depth |

`hrwtech/openclaw-esp32` (11 stars) is a fork of OpenClaw for ESP32. Its README does not explain ESP32 behaviour, so do not count on it.

### Architecture for open-source agents
```
Hermes on customer's Mac mini, VPS or Nous cloud agent
        | private network or TLS, never an open port
Phone app (bridge, holds gateway credentials)
        | Bluetooth
Device (screen, buttons, mic)
```

### Trade-offs
- **Friction moves to setup.** Customers must run an agent and pay for a model. Fine for engineer alumni; not for general buyers. If you host it for them, you take on the models, tokens and security you wanted to avoid.
- **The device becomes a security key.** It approves commands on someone's machine, so it needs pairing, a lock, and a clear display of exactly what is being approved. It should not approve payments or destructive actions without a second check.
- **Do not ship skills from a public marketplace** without review.

## Market blockers
- **Duet:** research preview, US Pro and Max subscribers only, via waitlist. Most Indian alumni cannot use it yet.
- **Dots:** reportedly Pro and Business Premium only (single blog plus one developer report).
- **Muse Charm:** Meta's own device, due in December.

## Recommendation
1. Make **Claude Code and Codex the first targets, through T3 Code**. Keep direct hooks as the fallback and for terminal sessions.
2. Build one adapter layer so Hermes and OpenClaw can follow. Among those two, Hermes comes first because its HTTP API is simplest.
3. Support Duet and Dots as **display-only**, then add connector features as they open up.
4. Sell first to **alumni who use Claude Code or Codex daily**, and pitch away-from-desk use, not another desk display.

## Nous Portal plans (the "Hermes token plan")
Reported prices: Free, Plus $20, Super $100, Ultra $200 per month. Paid plans cover model usage inside Hermes, tools, and a cloud agent. Portal API keys draw on credits, not the plan's included usage. The plan is optional: Hermes also works with a customer's own provider keys or local models. The device talks to the Hermes API server, which is separate from the Portal API, so the plan changes billing and hosting, not device access.

## Tests before any hardware
0. **T3 Code loop (first, about 1–2 days):** run T3 locally and connect a small script with a limited token. Start a Claude Code thread and a Codex thread in Supervised mode, stream status to a simulated device page, then approve and deny real requests. If T3 cannot do this, fall back to hooks. Then wear an existing board running Agent Buddy for a week to see if a glanceable device beats the phone.
1. **Hermes approval loop (later, about 1 day):** run the latest Hermes in a throwaway VM with no real accounts. Write a small client that starts a run, receives an approval and resolves it. Success: a laptop script can approve and deny a risky terminal command, and we know how long a request can wait.
2. **Notification dump (about 1 day):** a tiny Android app logs the actions on ChatGPT, Duet, WhatsApp and SMS notifications. Success: we know which ones expose buttons.
3. **Connector test (about half a day):** a tiny MCP server tested with a dot. Needs a Dots-eligible plan.

## Sources
- [T3 Code](https://github.com/pingdotgg/t3code)
- [T3 Code architecture overview](https://github.com/pingdotgg/t3code/blob/main/docs/internals/overview.md)
- [T3 Code RPC contract](https://raw.githubusercontent.com/pingdotgg/t3code/main/packages/contracts/src/rpc.ts)
- [T3 Code permission modes](https://raw.githubusercontent.com/pingdotgg/t3code/main/docs/user/permission-modes.md)
- [T3 Code remote access](https://raw.githubusercontent.com/pingdotgg/t3code/HEAD/docs/user/remote-access.md)
- [T3 Code session import PR](https://github.com/pingdotgg/t3code/pull/5499)
- [Claude Code MCP docs](https://code.claude.com/docs/en/mcp)
- [Codex plugins overview](https://ustack.app/products/codex-plugins)
- [Hermes personal assistant setup notes](https://composio.dev/blog/best-hermes-plugins)
- [Claude Code hooks](https://code.claude.com/docs/en/hooks)
- [Claude Code Remote Control](https://code.claude.com/docs/en/remote-control)
- [Claude Code channels overview](https://www.atcyrus.com/stories/what-are-claude-code-channels)
- [Anthropic claude-desktop-buddy](https://github.com/anthropics/claude-desktop-buddy)
- [codex-buddy-bridge](https://github.com/JaredBoone/codex-buddy-bridge)
- [Agent Buddy](https://github.com/DanWahlin/agent-buddy)
- [Codex Micro](https://www.cloudmagazin.com/en/2026/07/14/codex-micro-desktop-agent-control/)
- [VibeMon](https://nalbam.github.io/vibemon-app/)
- [AgentDeck](https://gh.r-mj.com/puritysb/AgentDeck)
- [Codex app-server guide](https://codex.danielvaughan.com/2026/03/28/codex-app-server-json-rpc-protocol/)
- [Codex PermissionRequest hooks](https://codex.danielvaughan.com/2026/04/15/permissionrequest-hooks-programmatic-policy-engine/)
- [OpenClaw gateway protocol](https://github.com/openclaw/openclaw/blob/main/docs/gateway/protocol.md)
- [OpenClaw device, node and approval methods](https://docs.openclaw.ai/gateway/protocol/rpc-devices-nodes-and-approvals)
- [OpenClaw nodes](https://docs.openclaw.ai/nodes)
- [OpenClaw known vulnerabilities](https://clawdocs.org/security/known-vulnerabilities)
- [Hermes Agent](https://github.com/NousResearch/hermes-agent)
- [Hermes security docs](https://hermes-agent.nousresearch.com/docs/user-guide/security/)
- [Hermes API server docs](https://hermes-agent.nousresearch.com/docs/user-guide/features/api-server)
- [Hermes Agent costs](https://www.gradually.ai/en/hermes-agent-costs/)
- [CVE-2026-14625 in Hermes](https://www.sentinelone.com/vulnerability-database/cve-2026-14625/)
- [xiaozhi-esp32](https://github.com/78/xiaozhi-esp32)
- [openclaw-esp32 fork](https://github.com/hrwtech/openclaw-esp32)
- [Nordic ANCS client docs](https://docs.nordicsemi.com/r/bundle/sdk_nrf5_v16.0.0/page/ble_sdk_app_ancs.html)
- [Android 15 sensitive notifications](https://androidauthority.com/android-15-sensitive-notifications-3416414)
- [iOS 26.5 EU wearable changes](https://www.macrumors.com/2026/05/11/ios-26-5-eu-third-party-wearable-changes)
- [Gadgetbridge calls and replies](https://gadgetbridge.org/basics/features/calls-replies)
- [Bangle.js WhatsApp reply thread](https://forum.espruino.com/conversations/373668)
- [Claude connectors help](https://support.claude.com/en/articles/11176164-use-connectors-to-extend-claude-s-capabilities)
- [Duet early review](https://superpowerdaily.com/posts/anthropic-opens-claude-duet-preview-for-ai-work-that-keeps-going-between-chats)
- [Connecting an MCP server to Dots](https://www.marqeable.com/blog/connect-marqeable-mcp-to-openai-dots/)
- [Dots regional availability](https://smartscope.blog/en/blog/openai-dots-regional-availability-2026/)
- [Meta opens Muse connectors](https://runtimewire.com/article/meta-opens-muse-connectors-developers)
- [Muse custom integrations](https://parallel.ai/articles/meta-muse-custom-integrations)
- [Cloudflare human in the loop](https://developers.cloudflare.com/agents/concepts/human-in-the-loop)
