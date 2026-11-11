# Backend strategy – version 1

Date: 2026-11-06. Desk research only: nothing new was run on a phone, board or agent. A DeepSeek sweep through Command Code found the Happy, Cursor and Grok hook leads, and I rechecked those against their own pages. Each fact is tagged **[official]** (the company's own page, or the project's own GitHub repo) or **[secondary]** (blogs and press, some single-source). Setup times are my estimates.

## Update: founder's decision, 2026-11-06
Version 1 leads with **older alumni and admin tasks by voice**, with a small side test on younger coders using what already works. The main route is now a thin fork of T3's mobile app, with Claude Code on a computer that stays on. See [FORK_PLAN.md](FORK_PLAN.md). Cloud tasks are the fallback. Deposits pick the final group. The analysis below still holds, but the priorities have changed. Follow [OLDER_ALUMNI_PLAN.md](OLDER_ALUMNI_PLAN.md) for next steps.

## Recommendation (written before the decision above)
**Use a mix, with T3 Code still at the centre.** Do not replace it, and do not build on Muse.

| Layer | Job | Route | Status |
|---|---|---|---|
| 1. Live sessions | Status, prompts, approvals for Claude Code and Codex, then Cursor and Grok Build | T3 Code on the buyer's laptop | Works on one laptop. Remote access untested |
| 2. Admin tasks with the laptop off | Calendar, reminders, search, answers | The buyer's own cloud agent, triggered by a web call. First Claude scheduled tasks, then Grok Bot webhooks | Documented, untested by us |
| 3. Everything else | See alerts from ChatGPT, Claude, Grok and other apps | Android notification mirror, display only | Untested |

You still run no models and hold no tokens. Each buyer pays their own agent company.

**Why this mix:** no single route covers the brief. T3 is the only route that gave approvals across two agents, but it dies when the laptop sleeps. Cloud agents survive that, but cannot show approvals on your device. The notification mirror reaches every app, but mostly one way.

**What to tell buyers plainly:** version 1 does not let you talk to the ChatGPT or Claude chat apps, Dots or Duet by voice. I found no outside way in, so the device can only show their alerts.

**Who to sell to first:** alumni who already use Claude Code or Codex daily, on Android, whose laptop can stay on. That group may be small, so test it first (test 1 below).

**The one thing that could change this:** Happy, an open-source client with its own encrypted relay, might replace Tailscale and make setup much easier. Try it before locking in T3 (see "The challenger" below).

## What changed from the earlier research
1. **T3 Code now lists Cursor and Grok Build** as well as Claude Code, Codex, OpenCode, Antigravity and fx [official]. That is four of your six named products, if they work as well as the two we tested.
2. **The Muse SDK is real, but its terms block your business.** See the next section.
3. **There are official ways in from outside that FEASIBILITY.md missed:** Claude scheduled tasks can be started by a web call, and Cursor has a full Cloud Agents API [official].
4. **Claude, Codex and Cursor all have official phone remotes** [official]. Claude and Codex approve from the phone. ChatGPT Voice can drive Codex on iOS, and Cursor's iPhone app takes dictation. Your real rival is the buyer's phone, more than any other device, especially on iPhone.

## Founder's claims, checked

| Claim | Verdict | Evidence |
|---|---|---|
| "Grogbot" | **Grok Bot**, as you confirmed. A real product, not OpenClaw | xAI launched it in beta on 11 Aug 2026 [official] |
| It was the first mover | **True among the big four.** Grok Bot 11 Aug, Muse 8 Sep, Dots 29 Sep, Duet 21 Oct. OpenClaw and Hermes came earlier | [official] for Grok Bot, [secondary] for the sequence |
| It has the largest share | **No evidence.** I found no published user numbers. One comparison calls it the hardest of the four to get into | India trackers put ChatGPT far ahead, about 78% of AI chatbot web traffic in June 2026, and the Statcounter figures I saw list no Grok share [secondary] |
| Muse is free | **Partly.** It has a free tier plus $20 and $100 plans. One source says a card is needed even on free | [secondary] |
| Muse is unavailable in India | **True.** No date given. Sources differ on where it is live (US and Canada, one adds Japan and Singapore) | [secondary, including Medianama] |
| Meta released an open-source charm SDK | **True, with a big catch.** It is called `muse-gadget-sdk`, Apache 2.0, released about 2 Oct 2026. "Charm" is what community forks call their builds | [official] |
| Dots and Duet have nothing similar | **True as far as I can find.** No device SDK and no public API | Duet: US waitlist preview [official]. Dots: no public API announced [secondary] |
| Someone on X built a Tamagotchi-style charm for Grok Bot | **Could not confirm the exact post.** Two close matches below | Please send the link if you have it |

**Close matches for the X project:**
- **An X article by `Liu_zhongxisn`** puts Grok Bot on an ESP32 with a 1.75-inch screen, using a computer backend and a Grok Bot webhook. X blocked my fetch, so this comes from a search summary only. It is the closest match.
- **[Tamagroki](https://github.com/mills-builds/tamagroki)** is a printed cube, not a charm. It talks to Grok's voice API through a Python relay on a Mac and needs an xAI API key. Grok Bot support is still planned. It has 1 star.

Meta's own Muse Charm also gets called Tamagotchi-like in the press, so it may be what you saw. It targets December, with no price announced [secondary].

## Why Muse is out
Meta's [SDK terms](https://gadgets.muse.ai/sdk-terms) [official] say:
- Tokens are for personal, non-commercial use with your own Muse account.
- You may not embed a token in a device you sell, or advertise such a device publicly.
- The cap is 50 devices per token, and anything wider needs Meta's written permission.
- Meta can withdraw access at any time.

Even with permission, your buyers in India cannot use Muse. The 1.46B board is also not on the lists I saw.

**What you can still use:** the Apache-licensed firmware code as a reference, leaving out the Jollybot avatar and any Meta tokens. Read the licence file first. Revisit Muse only if it reaches India and Meta offers commercial terms.

## What each product lets an outside device do

| Product | See status | Send a prompt | Approve | Works with laptop off | Route |
|---|---|---|---|---|---|
| Claude Code | Yes, tested | Yes, tested | Yes, tested | No | T3 |
| Claude Code, new cloud task | Not found | Yes, one-way | No, answer in Anthropic's app | **Yes** | Scheduled task web call [official] |
| Codex | Yes, tested | Yes, tested | Yes, tested | No | T3 |
| Cursor, local | Listed, untested | Listed, untested | Unknown | No | T3 |
| Cursor, cloud | Yes, live stream | Yes, plus steering | No step found | **Yes** | Cloud Agents API, public beta [official] |
| Grok Build | Listed, untested | Listed, untested | Unknown | No | T3, or Grok's own scripting mode [official] |
| Grok Bot | No | Yes, one-way | No, in its own app | **Yes** | Webhook routine [official] |
| ChatGPT chat, Claude chat | Alerts only, untested | No route found | No | n/a | Notification mirror |
| Dots | Alerts only, untested | No route found | No | n/a | Notification mirror |
| Duet, Muse | Not usable in India | No | No | n/a | None |

**Details that matter:**
- **Grok Bot has no official API,** and staff confirmed this in October [official]. A webhook starts a routine, but a 200 reply only means it started, and the result stays in the Bot's chat. Community projects get replies back only through their own server.
- **Claude scheduled tasks are a research preview,** and the web call is marked experimental [official]. Each token can start only one task, which is safer than T3's token. The call returns a session link, and I did not find a way to read the result. Runs use auto mode, so most actions go through without asking you. Connector writes included, so limit each task to the connectors it needs.
- **Grok Build is the weakest for approvals.** Its hooks can only block a tool call, they let it through on a timeout or error, and no hook fires on a permission request [official]. Whether T3 can show its approvals is the key question in test 3.
- **Cursor's API has no approval step** that I could see, though only the first 100,000 characters of a long page were read. Cursor Start is ₹649 a month in India with UPI and includes cloud agents [official]. Whether that plan allows API use is unconfirmed.

## Routes compared

| | T3 Code | Muse SDK | OpenClaw or Hermes | Direct hooks | Vendor cloud triggers | Notification mirror | **Mix (recommended)** |
|---|---|---|---|---|---|---|---|
| **Buyer setup** | Medium, 15–25 min | Impossible in India | High, hours, plus a model bill | Medium, plus your companion app | Low to medium, 5–10 min per agent | Lowest, 2 min | Medium, but you can do it for them |
| **Admin tasks** | Yes, laptop on only | n/a | Yes, always on | No | **Yes, laptop off** | No | Yes |
| **Live sessions** | **Best** | No | Their own only | Good, includes terminal sessions | Cursor only | Alerts only | Best available |
| **Voice on the go** | Via phone, needs remote link | n/a | Via phone | Via phone and your relay | Via phone, works anywhere | No | Via phone |
| **Safety** | Token equals shell access | Meta's Sentinel, per Meta | Weakest record: many 2026 CVEs, malicious skills | You own the risk | Good: narrow tokens, but auto mode | Good: read-only | Rules below |
| **Cost to you** | Nil | Nil | Nil unless you host | **A relay to run** | Nil | Nil | Nil, if results need no relay |
| **Cost to buyer** | Existing plans | n/a | Host $4–29 a month plus model | Existing plans | Claude Pro about ₹2,000, Cursor ₹649, X Premium $30 | Nil | Existing plans |
| **Reliance** | One small project, MIT so forkable | Total on Meta | Low, but self-run | Spread across companies | Each company | Low, Android limits | Spread |

**Why not the others as the default:**
- **OpenClaw or Hermes:** the opposite of "almost no setup", and Anthropic stopped covering third-party tools like these under Claude subscriptions in April 2026 [secondary]. Offer an adapter later for people already running one.
- **Direct hooks:** they would add terminal sessions, but need a laptop app and a relay you run, which you want to avoid. Keep them as the fallback if T3 breaks.

## The challenger: Happy
[Happy](https://github.com/slopus/happy) is MIT licensed, with about 24,500 stars. It covers Claude, Codex and Grok on plans the buyer already has, and lists Gemini and other agents in beta [official]. You answer permission requests from its phone app.

**Why it matters:** its encrypted relay reaches the laptop without Tailscale, and sessions start from the terminal with `happy claude`. Those are T3's two biggest weaknesses.

**Why it is not the backbone yet:**
- We have tested nothing on it.
- I found no documented way for an outside app to connect, so we would have to read its code and copy its protocol.
- The default relay is someone else's server. Self-hosting it would mean you run a server.
- It does not list Cursor. Its voice is a paid feature after 20 free minutes every 30 days, though your own app would handle voice anyway.
- The laptop must still stay on.

**Hedge:** the Agent Client Protocol (ACP) is becoming a shared way to drive these agents. Cursor and Grok Build support it directly, and Claude Code and Codex through adapters. It would need a laptop companion, so it is a later option. Omnara looked useful, but its current docs describe its own agent platform, so I dropped it.

## Biggest risks

**1. T3 setup is not "almost no setup".** The buyer installs T3, runs it as a background service, installs Tailscale on laptop and phone, then pairs. T3 Connect would be simpler, but it has no documented way in for outside apps.
*Reduce it:* write a one-command installer that sets up the service, Supervised mode and a limited token, then shows a QR code. For the first 20 buyers, do the setup with them yourself.

**2. Anthropic's terms need a direct answer.** Their [legal page](https://code.claude.com/docs/en/legal-and-compliance) [official] bars developers from routing requests "through Free, Pro, Max, or Ultra plan credentials on behalf of their users". It does allow a user to sign in to the unmodified Claude Code with their own plan, which is how T3 works. You also never touch their login. That looks fine to me, but this is not legal advice. Anthropic has enforced this without warning before and says plan limits assume ordinary individual use. Ask them before selling. Do the same for OpenAI.

**3. The laptop still has to stay on.** Codex's own remote has the same limit: a sleeping host stops it [official]. Layer 2 covers admin tasks only, so live coding sessions stay tied to the laptop.

**4. Android only excludes iPhone alumni.** The spec leaves iOS out, and many alumni may carry iPhones. Find out before building more.

**5. Voice depends on the phone.** The ESP32-S3 has Bluetooth LE only, so it cannot act as a normal Bluetooth headset for Gemini or Siri. Your app must carry the audio and turn it into text. Spoken replies are not in the current protocol, so the spec needs them added, or replies stay on screen.

**6. Betas everywhere.** T3 says to expect bugs, Grok Bot and Claude scheduled tasks change quickly, and usage limits can shift. Keep the `AgentBackend` adapter strict so any one route can be swapped out.

**Safety rules to keep:** the spec's rules stand: no answer means no approval, high-risk actions go to the phone, and text is never truncated. Add one: for cloud tasks, give each task the fewest connectors and have it draft rather than send. The device cannot approve those runs, so it should not claim to.

## Is "open and works across environments" a real moat?
**No. It is a good position, but not a moat.**

**Why it won't protect you:**
- **The hard part is free.** T3 Code and Happy are both MIT and already multi-agent. Anyone can build on them.
- **The hardware is off the shelf.** Community forks already run on Waveshare boards, and Tamagroki shows one person can build a voice cube.
- **Others already sell or give this away.** Agent Buddy covers many agents, and Anthropic and OpenAI each offer their own routes free.
- **The agent companies hold the keys.** Your integration works only while they allow it. Anthropic's April cut-off of OpenClaw shows how quickly that can change.
- **"Open" has little pull for the buyer.** Alumni will pay for ease and trust, and few will pay for a licence.

**What openness does give you:**
- **Trust.** A device that approves commands on your laptop is easier to trust if its code can be read.
- **Insurance.** Buyers don't lose the device if one agent company changes direction.
- **A real India gap, for now.** Muse and Duet are not available here, and I found no sign of Muse Charm reaching India. This is timing, and it will close.

**What could become an advantage, in order:**
1. **Your alumni network and hands-on setup.** A big company won't sit with 20 UVCE alumni. This is the strongest thing you have.
2. **A device that really beats the phone.** Faster to glance at, safer to approve. It is still unproven, and the product depends on it.
3. **A trusted approval experience.** Good risk rules, clear text, and a record of never approving the wrong thing.
4. **A design people want to carry.** Custom cases and a character with personality.

Suggested line: *"One remote for the agents you already pay for, not owned by any of them."* Treat this as a way to win your first buyers, not as protection from competitors.

## What to test next

| # | Test | Effort | Passes if | Needs from you |
|---|---|---|---|---|
| 1 | Ask the 20 alumni: which agents do you pay for, Android or iPhone, does your laptop stay on? | 1 day | Enough are on Android with Claude Code or Codex | Do this before building more |
| 2 | Control T3 from a phone on mobile data through Tailscale | Half a day | Approve a real command from outside home | OK to start Tailscale, which changes network settings |
| 3 | Rerun the M0 script with Cursor and Grok Build | Half a day | Approvals arrive and work, or we learn they don't | Those tools installed and signed in |
| 3b | Try Happy: can our own client read status and answer an approval through its relay? | 1 day | Works from outside home with no Tailscale | Happy installed; I'd need your OK to run it |
| 4 | Fire a Claude scheduled task with the laptop closed, adding a calendar event | Half a day | Event appears, and we find how the result gets back | A Claude plan with Google connected |
| 5 | Log notifications from Claude, ChatGPT (Codex) and Grok on Android | 1 day | We know which have real action buttons | Android phone |
| 6 | Check the Grok Bot price and sign-up on an Indian account, then try a webhook | Half a day | India price confirmed, routine starts from a web call | Only if test 1 shows demand. Costs a plan, so ask first |
| 7 | Email Anthropic and OpenAI about this product | 1 hour | Written answer | You send it; I can draft it |
| 8 | Carry a prototype for a week against the phone's own remotes | 1 week | You reach for the device first | Board |

**Change the plan if:**
- Test 2 fails: move to Happy if 3b passes. Otherwise sell it as a home and office device.
- Test 3b passes and setup is clearly easier: make Happy the main route and keep T3 for Cursor.
- Test 3 fails: limit v1 to Claude Code and Codex, and use Cursor's cloud API.
- Test 4 can't get results back: confirm "sent" on the device, and let the result arrive as a mirrored alert.
- Test 8 fails: the phone wins, so rethink the product before spending more on it.

## Proposed changes to SOFTWARE_SPEC.md (not made, waiting for your OK)
1. Add a second backend type for one-way cloud triggers. The current `AgentBackend` assumes two-way threads.
2. Replace "no custom software" on the laptop with a small one-command installer.
3. Move Grok Bot, Claude scheduled tasks and the notification mirror into scope as later milestones.
4. Add Cursor and Grok Build, subject to test 3.
5. Say how spoken replies work, or state that replies are on screen only.
6. Add the safety rule for cloud tasks.

## Not verified
- Grok Bot's availability and price in India. xAI said the X Subscription would roll out globally, but India prices I found conflict from about ₹470 to ₹3,399 a month.
- Claude and ChatGPT prices in India: reported, conflicting, and need checking at checkout.
- Which agents UVCE alumni really use. No India developer survey turned up.
- Whether Duet or Dots custom connectors still work as FEASIBILITY.md described. I did not recheck them.
- T3 with Cursor and Grok Build: listed by T3, not tested by us.
- Whether an outside app can use Happy's relay, and how its approvals work in detail.
- Current Hermes and OpenClaw security state: the CVE list is mostly April to June 2026.

## Sources
**Grok Bot:** [launch post](https://x.ai/news/introducing-grok-bot) · [overview](https://docs.x.ai/grok-bot/overview) · [X Subscription](https://x.ai/news/x-subscription) · [routines](https://cursor.com/help/grok-bot/routines) · [no official API, Cursor forum](https://forum.cursor.com/t/grok-bot-official-api-to-create-bots-and-send-receive-chat-for-companion-uis/170985) · [community webhook bridge](https://github.com/stealth-bots/grok-bot-mcp) · [overview by Lindy](https://www.lindy.ai/blog/grok-bot) · [four-way comparison](https://moelueker.com/blog/claude-duet)

**Muse:** [SDK repo](https://github.com/facebookincubator/muse-gadget-sdk) · [SDK terms](https://gadgets.muse.ai/sdk-terms) · [Mixed News on the SDK](https://mixed-news.com/en/meta-muse-gadgets-sdk-open-source-home-link-esp32-c5/) · [Medianama on India](https://www.medianama.com/2026/09/223-meta-muse-personal-ai-agent-india/) · [country list](https://www.aiagentslibrary.com/blog/meta-muse-availability/) · [Muse Charm](https://gulfnews.com/technology/meta-muse-and-muse-charm-explained-what-they-do-and-what-they-cost-1.500686025)

**Duet and Dots:** [Duet help page](https://support.claude.com/en/articles/17631495-get-started-with-claude-duet) · [Dots in India](https://ecorpit.com/openai-dots-explained-features-price-india-safety/) · [Dots access](https://www.mindstudio.ai/blog/chatgpt-dots-pricing-access)

**X projects:** [Tamagroki](https://github.com/mills-builds/tamagroki) · [X article, not readable by me](https://x.com/Liu_zhongxisn/article/2112118491239379227)

**T3 and agent routes:** [T3 Code](https://github.com/pingdotgg/t3code) · [T3 remote access](https://raw.githubusercontent.com/pingdotgg/t3code/HEAD/docs/user/remote-access.md) · [T3 outside agents](https://raw.githubusercontent.com/pingdotgg/t3code/HEAD/docs/user/outside-agents.md) · [Claude Remote Control](https://code.claude.com/docs/en/remote-control) · [Claude scheduled tasks](https://code.claude.com/docs/en/routines) · [fire API](https://platform.claude.com/docs/en/api/claude-code/routines-fire) · [Codex remote connections](https://learn.chatgpt.com/docs/remote-connections) · [Cursor Cloud Agents API](https://cursor.com/docs/cloud-agent/api/endpoints) · [Cursor Start India](https://cursor.com/blog/cursor-start-india) · [Grok Build CLI](https://docs.x.ai/build/cli/reference) · [Happy](https://github.com/slopus/happy) · [Happy mobile app](https://happy.engineering/mobile-app/) · [Cursor for iOS](https://cursor.com/docs/cloud-agent/mobile) · [ChatGPT Voice with Codex](https://learn.chatgpt.com/docs/features/voice) · [Grok Build hooks](https://docs.x.ai/build/features/hooks) · [ACP agents](https://agentclientprotocol.com/get-started/agents) · [Omnara permissions](https://docs.omnara.com/tools/permissions)

**Terms and policy:** [Claude Code legal page](https://code.claude.com/docs/en/legal-and-compliance) · [TechCrunch on OpenClaw](https://techcrunch.com/2026/04/04/anthropic-says-claude-code-subscribers-will-need-to-pay-extra-for-openclaw-support/)

**Open agents:** [OpenClaw vs Hermes](https://blog.kilo.ai/p/hermes-vs-openclaw-when-to-reach) · [OpenClaw hosting prices](https://www.agent37.com/blog/openclaw-hosting-pricing) · [Hermes CVE-2026-14625](https://www.sentinelone.com/vulnerability-database/cve-2026-14625/)

**India market and prices:** [Statcounter India](https://gs.statcounter.com/ai-chatbot-market-share/all/india) · [Digit on India AI use](https://www.digit.in/features/general/chatgpt-vs-claude-vs-gemini-who-is-winning-the-ai-war-in-india-in-2026.html) · [Claude India prices](https://ecorpit.com/claude-india-rupee-pricing-vs-us-teams-2026/) · [X prices by country](https://opentherank.com/social-pricing/x/) · [Grok India pricing tracker](https://opentherank.com/ai-pricing/grok/india/)
