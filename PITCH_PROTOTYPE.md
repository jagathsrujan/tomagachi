# Pitch prototype plan

Status: proposed, nothing bought or built. Items marked *(interp.)* are estimates; other prices are web listings to confirm at checkout.

## Decision
Use the Waveshare 1.46B dev board for the pitch video, not a breadboard. The real idea to keep from the breadboard plan is **pitch before building more**.

## Cost comparison
| | Breadboard | 1.46B dev board |
|---|---|---|
| Parts | ESP32-S3 board ₹600–1,000, round display ₹425, mic ₹212–250, amp ₹200–350, speaker ₹80 *(interp.)*, breadboard, wires and buttons ₹200 *(interp.)*, vibration ₹52 | Board ₹2,249–3,061 |
| Total before shipping | About ₹1,770–2,360 | ₹2,249–3,061 |
| Shipping | Several stores, ₹150–300 *(interp.)* | One store |
| Screen | 240×240, no touch | 412×412, touch |
| Battery charging | Not included | Built in |
| Looks like a product in video | No | Yes, with a printed shell |
| Reused in the pilot | Mostly no | Yes |

Even a breadboard build needs an ESP32 dev board, because a bare ESP32 module doesn't fit a breadboard or include USB and power parts.

## Video build
1. Buy the 1.46B and print a simple shell.
2. Power it by USB or a battery.
3. Connect it to the laptop simulator over Wi-Fi. This skips the Android app for the video, but the simulator is loopback-only today, so it needs a secured local connection.
4. Write firmware for the home, approval and task screens.

## 60-second video
1. Problem: agents stop and wait while you're away from the laptop.
2. Device buzzes or chimes in hand; the screen shows the exact command.
3. Approve on the device; the real agent carries on.
4. A risky `rm` gets sent to the phone instead.
5. Start a task by voice, if voice works by then.
6. Founding-member offer and a link to register interest.

## Honesty rules
- Label it a working prototype.
- Show only real behaviour, and don't fake any screen.
- Don't claim it works away from home until remote access is tested.
- Don't claim every command is reviewed. Harnesses allow some commands without asking.
- Don't promise battery life before it's measured.

## Success check
Send to about 20 alumni who use Claude Code or Codex. Count replies, calls and refundable deposits, not likes.

## Sources
- [Controllers Tech round display](https://shop.controllerstech.com/products/1-28-inch-gc9a01-round-tft-lcd-display-module)
- [Robo Bazar ESP32-S3 board](https://robobazar.in/product/esp32-s3-devkit-n16r8-development-board/)
- [SRK Electronics INMP441](https://srkelectronics.in/?p=39271)
- [Hubtronics 1.46B listing](https://hubtronics.in/other-lcds/esp32-s3-touch-lcd-1.46b)
