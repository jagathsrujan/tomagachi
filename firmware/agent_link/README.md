# Agent link (version 1)

The board connects to the laptop over Wi-Fi with no phone, and starts Claude Code sessions.

```
ESP32-S3 --Wi-Fi--> m1/scripts/link.mjs --> Agent SDK --> Claude Code   (default)
                                        \-> T3 Code   --> Claude Code   (--t3)
```

## Setup
1. Start the bridge from `m1/`. The first run writes `secrets.h` here with the pairing token and laptop address.
   ```bash
   npm run link
   ```
2. Put your Wi-Fi name and password in `secrets.h`. The ESP32 needs 2.4 GHz, so on an iPhone hotspot turn on Maximize Compatibility. If the laptop's address changes, update `BRIDGE_HOST`.
3. Flash the board with the board on USB, replacing the port with yours:
   ```bash
   arduino-cli compile --upload -p /dev/cu.usbmodemXXXX --fqbn esp32:esp32:esp32s3:CDCOnBoot=cdc .
   ```

The FQBN above sends the serial log to the native USB port. If your cable is in the board's UART port, use `--fqbn esp32:esp32:esp32s3` instead, or you'll see only the boot messages at 115200.

To use T3 instead, run `npm run link:t3`.

## Using it
| Screen | Tap BOOT | Hold BOOT |
|---|---|---|
| Home | Next preset | Start that session (0.8 s) |
| Approval | Deny | Approve (1.5 s) |

You can also type a request in the serial monitor at 115200 and press Enter to start a session with that text.

High-risk actions can't be approved on the board. Open http://127.0.0.1:3918 on the laptop to answer them. Requests nobody answers are denied after 10 minutes on the direct route.

Presets and the project folder come from `m1/profiles.json`. Sessions run in `m1/sandbox-repo` until you add another project in `m1/scripts/link.mjs`.

## Limits
- Traffic isn't encrypted, so use trusted Wi-Fi only. The pairing token itself never crosses the network.
- The direct route loads no personal Claude Code settings, so your allow rules, CLAUDE.md files and connectors don't apply yet.
- Claude Code decides what needs approval, and some harmless commands may run without asking.
- For file edits the board shows the file path, not the change.
- The laptop must stay on and on the same network.
