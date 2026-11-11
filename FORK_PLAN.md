# Thin fork plan

Date: 2026-11-06. Status: decided, nothing built. Facts about T3 come from reading its repo today, mostly file lists, package files and docs, not the full code. Time estimates are my guesses *(interp.)*.

## Decision
Build version 1 on a **thin fork of T3 Code's mobile app**. The laptop keeps running T3 unchanged. The device talks to the phone over Bluetooth, and the phone talks to T3.

```
Device  <-Bluetooth->  Your phone app (T3 mobile fork)  <-network->  T3 on laptop  ->  Claude Code + connectors
```

This replaces the cloud route as the main plan for admin tasks. Claude scheduled tasks remain the fallback for buyers whose computer can't stay on.

## What T3's mobile app already gives you
- **iPhone and Android** from one Expo and React Native codebase, MIT licensed.
- **Voice to text:** a `t3-voice-transcription` module. It has audio pipe code that may accept the device's audio, which I haven't confirmed.
- **Pairing and QR scanning**, using its camera and secure storage packages.
- **Agent notifications:** a `t3-agent-notifications` module.
- **Shared connection code:** `client-runtime` and `contracts`.
- **T3 Connect switched off by default** in a fresh copy, so no T3 cloud account is needed.

**Missing:** I found no Bluetooth library or device link, so that's the main thing to add. Background running also needs a check.

**Reusable from our work:** the approval rules and risk rules in `m1/src/bridge.mjs` and `m1/src/risk.mjs` are JavaScript, so they should move across.

## Rules to keep it thin
1. **Change as little of T3 as possible.** Put new code in one module, `apps/mobile/modules/device-link`, plus a few new screens.
2. **Pin one T3 version** for the app and laptop. T3 made 100 commits in about 10 hours, so never follow its daily builds.
3. **Update on purpose,** for security fixes or a needed feature, and test before releasing.
4. **Stop T3 updating itself on buyers' laptops** if it can, because a newer server could break a pinned app. Needs checking.
5. **Remove T3's name and logo,** and keep the MIT copyright notice.
6. **Keep Claude's name out of the product name.** Buyers sign in to the unmodified Claude Code themselves, and you never touch their login.

## Build order

| Step | Build | Done when | Effort | Needs |
|---|---|---|---|---|
| F0 | Ask Claude Code about the calendar through the existing simulator | A real answer comes back, and a calendar write asks for approval | 1–2 hours | Calendar connected; your OK to use the simulator |
| F1 | Build the unmodified T3 mobile app on your Android phone and pair it with the laptop | App controls a real thread on home Wi-Fi | 1–2 days | Android phone, Android build tools |
| F2 | Add Bluetooth and link the board | Board shows live status | 1 week | The board |
| F3 | Move the approval and risk rules across | Rules pass the same tests as `m1` | 2–3 days | |
| F4 | Send device audio through the phone's voice to text | Spoken request starts a task | 1 week | |
| F5 | Simple mode: large text, hides coding screens, one `personal` project | An older person uses it unaided | 1 week | Someone to try it |
| F6 | Remote access through Tailscale | A task works from mobile data | 1 day | Your OK to start Tailscale |
| F7 | One-command installer and your own name | Fresh laptop set up in under 15 minutes | 1 week | Only after deposits |

F0 and F1 don't need the board, so they can start now.

**The pitch video needs F0 to F2 only.** Everything after that can wait for deposits.

## Risks
- **The computer must stay on.** This decides whether older alumni can use it, so the alumni message now asks.
- **Remote access is unproven.** Running T3's relay would mean Cloudflare, PlanetScale, Clerk and push notification accounts. Try Tailscale first.
- **Background Bluetooth,** especially on iPhone. Android needs a long-running background service. Both may need custom native code.
- **App store costs and review.** Apple's yearly developer fee and Google's one-off fee apply. Check current prices. For the pilot, install on Android directly and skip the stores.
- **Security is now yours.** The phone holds a token equal to shell access on the laptop. Keep T3's secure storage, and release fixes quickly.
- **Build setup may be heavy.** The repo is large and uses its own tools. F1 will show how hard it is on your machine.

## If F0 or F1 fails
- **F0 fails** because Claude Code can't use the calendar through T3: go back to the cloud route.
- **F1 is too hard to build:** write a small app using T3's documented protocol, as `m0` and `m1` did.

## Before making anything public
The repo is public MIT code, so a private copy is fine for now. I haven't created any fork or repo. Publishing or renaming it is your call.
