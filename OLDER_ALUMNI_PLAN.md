# Older alumni plan – admin by voice

Date: 2026-11-06. Status: decided, nothing tested or sent yet. Items marked *(interp.)* are my suggestions to change freely.

## Decision
**Lead with older UVCE alumni and admin tasks by voice, and run a small side test with younger coders.** The founder asked me to make this call. Deposits pick the final group.

- **Older alumni get about 90% of the effort:** all new building goes here.
- **Younger alumni get a side test with no new features:** one video of the approval flow that already works, and the same deposit offer to about 10 people. Follow the honesty rules in [PITCH_PROTOTYPE.md](PITCH_PROTOTYPE.md), and don't claim it works away from home. Also ask whether they could install it on their work laptop.

**Why:** neither group has shown it will pay. The older route is also unproven and could fail at test 1, while the coding route already works. The side test costs a day or two *(interp.)* and gives you a fallback.

**How the final choice is made:**
- Older alumni reach 5 deposits and test 1 passes: build for them.
- Test 1 fails with no fix, or younger alumni clearly out-deposit them: switch to coders.
- Neither group reaches 3 deposits: stop and rethink the offer.

**Update, same day: the main route is now the thin fork in [FORK_PLAN.md](FORK_PLAN.md),** with Claude Code on a computer that stays on. It returns answers and shows approvals. The cloud route below stays as the fallback for buyers without one, so treat the "Laptop" and "Tailscale" rows as fallback only. The route test is replaced by step F0 for now.

## What this changes

| Topic | Before | Now |
|---|---|---|
| Main route | T3 Code on the laptop | The buyer's own cloud agent, reached through the phone |
| Laptop | Must stay on | Not needed for daily use |
| Tailscale and T3 install | Needed | Not needed |
| Main proof so far | Approvals, tested | Nothing tested yet |
| Replies | Short status on screen | The answer itself must come back |
| Screen and sound | Small text, buzz | Large text, and probably spoken replies |
| Setup | Technical buyer | You do it with them |

The new setup is simpler for the buyer, but we lose the part we had already proven.

## The four problems to solve

**1. Getting the answer back (the blocker).** Claude scheduled tasks start from a web call, but I found no documented way to read the result. "What's on my calendar tomorrow?" is useless without an answer. There are now four options to try before you'd need your own server. See "Getting the answer back" below.

**2. Why not use a free phone assistant?** ChatGPT and Gemini already take voice on the phone for free. Possible answers are one button with no unlocking, a device that reminds them, and someone who sets it all up. These are guesses until alumni respond.

**3. The monthly cost.** The device needs a paid agent, because ChatGPT and Dots have no outside way in. A buyer with only ChatGPT would add Claude Pro at a reported ₹2,000 a month, on top of ₹5,000 for the device. Say this up front.

**4. Safety.** Cloud runs mostly proceed without asking, and they act under the buyer's name. Version 1 should stick to calendar, reminders and answers. Email stays as drafts only, and it never sends, pays or deletes.

## Default route
**Claude scheduled tasks first,** because they have rupee pricing, a token that can start only one task, and calendar and email connectors. They are a research preview, so this can change. Grok Bot webhooks are the second option, for buyers who already have a plan. Both have the same reply problem.

## Getting the answer back
None of these is tested. The order is my suggested try order.

| Option | How it works | Your server? | Weak point |
|---|---|---|---|
| **A. Claude app alert** | The task finishes and our app mirrors the phone alert | No | Routines seem to have a push setting, but it isn't documented, and the alert may not hold the answer |
| **B. Phone reads the calendar itself** | "What's on today?" comes straight from the phone's calendar, with no agent | No | Only works for calendar questions, though it's instant and free |
| **C. Private push channel** | The task posts its answer to a private [ntfy](https://docs.ntfy.sh/publish/) topic, and our app listens | No | One extra setup step per buyer. Answers pass through ntfy, so use a private token or encrypt them |
| **D. Calendar as a mailbox** | The task writes its answer into a hidden calendar, and our app reads it | No | A hack, slow to sync, and Claude's docs conflict on whether the connector can write |

**Likely result:** B for calendar questions, plus A or C for everything else. C works because a cloud environment can allow extra domains and hold a secret token that the session never sees [official]. If all of these fail, the fallback is a small server of your own.

**Also measure speed.** Each request starts a full cloud session, and I found no timing figures. If replies take minutes, this suits "do this for me" tasks and not quick questions. B matters more in that case.

## Next steps, in order

### 1. Test the route (half a day, you run it)
1. Create a separate Google calendar called `Test`, and connect Google Calendar on claude.ai.
2. At [claude.ai/code/routines](https://claude.ai/code/routines), create a scheduled task. Include only the calendar connector.
3. Use this prompt:

   > Read the request in the routine-fire-payload block. It comes from the account owner's voice device. If it asks for a calendar event, add it to the calendar named Test. If it asks a question about the calendar, answer it. Never send anything or delete anything. End with one short sentence saying what you did.

4. Edit the task, add an **API** trigger, then copy the Fire URL and generate a token. It appears only once. Keep it private and don't paste it into chat.
5. Run this, using your own URL and token, then close the laptop straight away:

```bash
curl -X POST "YOUR_FIRE_URL" -H "Authorization: Bearer YOUR_TOKEN" -H "anthropic-beta: experimental-cc-routine-2026-04-01" -H "anthropic-version: 2023-06-01" -H "Content-Type: application/json" -d '{"text": "Add a meeting tomorrow at 3 pm called Device test."}'
```

**Write down:**
- Did the event appear, and how long did it take?
- Did your phone get any alert from the Claude app when it finished?
- Did the alert include the answer, or just say it finished?
- Repeat with "What is on my Test calendar tomorrow?" and note where the answer shows up.
- Time from sending the request to seeing the answer. Do this 5 times.
- Check the task's page for any notification setting, and turn it on.
- If no alert carries the answer, try option C. I can give you those steps.

**Passes if** the event appears with the laptop closed and the answer reaches the phone without opening a chat. A wait under about 30 seconds feels fine *(interp.)*.

### 2. Ask the older alumni (1 day, can run alongside)
Draft message, not sent:

> Namaskara sir/ma'am, I'm [name], a student at UVCE. I'm building a small device that handles calendar, reminders and quick questions by voice, without opening your phone. Could I ask five quick questions?
> 1. Which daily admin task do you most wish someone else handled?
> 2. Who handles it today: you, an assistant or family?
> 3. Do you pay for ChatGPT, Claude or a similar app?
> 4. Do you use an iPhone or Android phone?
> 5. Do you have a laptop or desktop that stays switched on at home or the office?

Talk to 5 of them in person if you can *(interp.)*. You'll learn more from watching them react than from typed replies.

### 3. Film one honest video (after test 1 passes)
1. Problem: small admin tasks pile up, and phones pull you into distractions.
2. Press the button and say "Remind me to call the bank tomorrow at 10."
3. The device confirms in large text.
4. Ask "What's on tomorrow?" and the answer appears.
5. Offer: "I'll set it up with you. ₹5,000, plus an AI plan of about ₹2,000 a month."
6. Founding-member link.

Show only what really works, and label it a prototype.

### 4. Ask for refundable deposits
Suggested ₹500, returned on request *(interp.)*. Send it to about 20 older alumni.

**Build further if** at least 5 pay a deposit *(interp.)*. Below 3, stop and rethink the offer, because likes and kind words don't count.

## Before real orders
Send the terms questions to Anthropic and OpenAI. Drafts for those, and the younger alumni message and video, are in [OUTREACH_DRAFTS.md](OUTREACH_DRAFTS.md).

## Decisions still open
- **iPhone:** decide after step 2. It may matter more for this group.
- **Spoken replies:** likely needed, but decide after you've watched real users.
- **Small server:** decide after test 1.
- **Screen size:** check whether the 1.46-inch screen is readable for older eyes before finalising hardware.
- **Pocket or worn:** still open.

## Sources
[Claude scheduled tasks](https://code.claude.com/docs/en/routines) · [Cloud environments](https://code.claude.com/docs/en/cloud-environments) · [Claude Code changelog](https://code.claude.com/docs/en/changelog) · [Google Calendar connector](https://claude.com/connectors/google-calendar) · [Calendar connector docs](https://claude.com/docs/connectors/google/calendar) · [ntfy publishing](https://docs.ntfy.sh/publish/)
