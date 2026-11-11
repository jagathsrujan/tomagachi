# Hardware and budget plan

Status: proposed, nothing bought or measured. Items marked *(interp.)* are my estimates, not quotes. I assumed about ₹90 per US dollar *(interp.)*.

## Requirements from the software
| Need | Why | Hardware impact |
|---|---|---|
| Bluetooth LE to phone | All data goes through the phone | Required |
| Wi-Fi | **Not needed.** The device never touches the internet | Any chip without Wi-Fi is allowed |
| Readable approval text | Up to 140 characters, never truncated | Screen size sets this limit |
| Two physical controls | Approve and deny without looking at touch | Buttons |
| Vibration | The core alert in a pocket | **Often missing on dev boards** |
| Microphone | Voice delegation | Optional in the first test |
| Full day on battery | Otherwise it won't become a habit | Sets chip and battery |
| No agent tokens on the device | Security | Little storage needed |

Voice over Bluetooth should fit, because compressed speech needs only tens of kilobits per second *(interp.)*.

## Candidate hardware
| | Waveshare AMOLED 1.75 | M5Stack StickS3 |
|---|---|---|
| Price | ₹3,775–3,835 in India | $21.50, about ₹1,950 before import costs |
| Screen | 1.75" round AMOLED, 466×466, touch | 1.14" LCD, 240×135 |
| Audio | Dual mics with echo cancelling, speaker header | Mic and speaker |
| Motion | 6-axis IMU | 6-axis IMU |
| Battery | Header only; battery bought separately | 250 mAh built in |
| Case | Optional | Included, 48×24×15 mm, 20 g |
| Vibration | Not listed | Not listed |
| Chip | ESP32-S3 | ESP32-S3 |
| Best for | The look of the final product | Cheapest wearable test |

**Replaces the earlier pick:** the 1.75 AMOLED replaces the 1.28" LCD in [PLAN.md](PLAN.md), because Agent Buddy supports it and it has better mics. A 140-character command also reads better on it.

## Battery is the biggest hardware risk
Assumptions *(interp.)*: 400 mAh with 340 mAh usable. Per day, 40 glances of 10 s and 5 voice commands of 20 s, at about 130 mA while awake. That adds about 18 mAh per day.

| Standby current, Bluetooth connected | Daily use | Battery life |
|---|---|---|
| 3 mA, tuned design | 90 mAh | About 3.8 days |
| 10 mA, likely dev board | 258 mAh | About 1.3 days |
| 100 mA, sleep not working | 2,400 mAh | About 3.4 hours |

Staying asleep while keeping the Bluetooth link is what decides this. Published ESP32-S3 sleep figures run from 0.23 to 3.3 mA, but none is a measured connected figure on this type of board. One guide puts idle at about 100 mA without light sleep, and forum users report dropped links in sleep. **Measure this first.**

**If it fails:** a Nordic chip such as the nRF5340 is the natural replacement, because Wi-Fi isn't needed. It costs a firmware rewrite on Zephyr, so only move if measurements force it.

## Budget by stage

### Stage 1: test the hardware (about ₹8,000–10,000)
| Item | Cost |
|---|---|
| Waveshare AMOLED 1.75 | ₹3,800–4,250 |
| M5Stack StickS3, imported | ₹2,500–3,500 *(interp.)* |
| LiPo cell, 400 mAh, and lead; check connector and polarity | ₹250 |
| Vibration motor and driver parts | ₹100 *(interp.)* |
| Filament for test shells | ₹300 *(interp.)* |
| Multimeter with mA range, if you don't own one | ₹1,500 *(interp.)* |

### Stage 2: pilot of 5 units (about ₹25,000)
Dev board in your printed shell, about ₹4,500 each *(interp.)*, plus spares. Treat these as loaned prototypes unless compliance advice says otherwise.

### Stage 3: custom board, 50 units (about ₹2.5–3.5 lakh) *(interp.)*
| Item | Per unit |
|---|---|
| Chip module, power, audio and haptic parts | ₹800–1,000 |
| 1.75" AMOLED panel | ₹1,100–1,600 |
| PCB and assembly at 50 units | ₹500–800 |
| Battery | ₹250 |
| Shell and finishing | ₹250–400 |
| **Unit cost** | **₹2,900–4,050** |

One-time: two board spins ₹30,000–50,000, plus compliance. JLCPCB's Standard assembly setup is $25–50, and one 5-board ESP32-S3 project paid about $40 per board.

**Key insight:** at 50 units, a custom board costs about the same as the retail dev board. Build it for size, battery and vibration, not to save money. Injection moulding only makes sense at several hundred units.

## Selling in India
Not legal advice; consultant sources conflict, so get a quote before selling.
- **WPC ETA:** each wireless model needs one. BLE-only below 0 dBm may use self-declaration in about 7–15 working days; Wi-Fi plus BLE may need lab testing, 30–60 working days.
- **Cost:** reported figures vary. Plan on ₹25,000–50,000 per model *(interp.)*.
- **Open question:** whether an ESP32 with Wi-Fi disabled counts as BLE-only. A Nordic chip might avoid this.
- **BIS and battery rules:** not researched yet.

## What the budget limits
| Feature | Dev board stage | Custom board stage |
|---|---|---|
| Status and approvals | Yes | Yes |
| Voice | Yes, on the AMOLED | Yes |
| Vibration | Needs a hand-wired add-on | Built in |
| Pocket or neck size | Chunky, about watch-head size | Can be slimmer |
| Multi-day battery | Unlikely | Possible |
| Polished shell | 3D printed | Printed, moulded at volume |

## Recommendation
1. Spend Stage 1 only: about ₹10,000.
2. Measure standby on both boards before designing anything.
3. Wear one for a week with Agent Buddy or our firmware.
4. Confirm the price after that, using the target below.

## Target: ₹4,000 build cost, ₹5,000 price
| Route | Unit cost | Meets ₹4,000? |
|---|---|---|
| Waveshare AMOLED at Indian retail, see [STAGE1_ORDER.md](STAGE1_ORDER.md) | About ₹4,200–4,600 | No, ₹200–600 over |
| **Waveshare 1.46B round LCD, voice built in (first choice)** | **About ₹2,700–3,450** *(interp.)* | **Yes, about ₹1,200–1,900 left per unit at ₹5,000** |
| Waveshare 1.69" touch LCD, plus add-on mic (backup) | About ₹2,500–2,800 *(interp.)* | Yes, about ₹1,900 left per unit at ₹5,000 |
| Waveshare bought direct in a batch | About ₹4,000 *(interp.)* | Just, if import costs stay low |
| StickS3 with a simple shell | About ₹2,800–3,000 *(interp.)* | Yes, but with a small screen |
| Custom board, 50 units | ₹2,900–4,050 *(interp.)* | Yes, after ₹55,000–1 lakh one-time spend |

Per unit at ₹5,000, not charging GST *(interp.)*:

| Item | Amount |
|---|---|
| Sale price | ₹5,000 |
| Build cost | −₹4,000 |
| Payment fees, about 2% | −₹100 |
| Shipping | −₹100 |
| Repairs and replacements, about 5% of cost | −₹200 |
| **Left per unit** | **About ₹600** |

- **If ₹5,000 includes 18% GST:** about ₹760 goes to tax, before claiming credit on parts. Check this with an accountant.
- **Break-even on the custom board:** about 90–170 units, so 50 units would lose money.
- **Better pilot use of ₹5,000:** a founding-member price on dev-board units, with no custom-board spend yet.

## Key checks
- Bluetooth stays connected in light sleep, at under 10 mA average.
- Full day of normal use on the battery.
- A 140-character command is readable at arm's length.
- Vibration is noticeable in a pocket.
- Voice sent over Bluetooth is clear enough for speech-to-text.
- Fits the shell, with no pressure on the battery.

## Sources
- [Waveshare ESP32-S3 AMOLED 1.75](https://www.waveshare.com/esp32-s3-touch-amoled-1.75.htm)
- [Zbotic India listing](https://zbotic.in/product/waveshare-esp32-s3-1-75inch-amoled-round-touch-display-development-board/)
- [Hubtronics India listing](https://hubtronics.in/esp32-s3-touch-amoled-1.75)
- [M5Stack StickS3 overview](https://www.cnx-software.com/2026/01/23/m5stack-sticks3-esp32-s3-mini-iot-dev-kit-features-1-14-inch-color-display-speaker-and-mic-for-voice-interaction/)
- [Espressif NimBLE power save example](https://gitea.techquadbit.net/alex/esp-idf/src/branch/master/examples/bluetooth/nimble/power_save)
- [Qoitech ESP32-S3 sleep measurement](https://www.qoitech.com/?p=19961)
- [ESP32 BLE power guide](https://hubble.com/community/guides/esp32-power-consumption-in-ble-mode-what-to-expect-from-advertising-scanning-and-connected-states/)
- [JLCPCB assembly FAQ](https://jlcpcb.com/help/article/98-pcb-assembly-faqs)
- [ESP32-S3 PCBA cost example](https://pjcau.github.io/esp32-emu-turbo/docs/manufacturing)
- [WPC ETA for smart devices](https://blogs.seyecs.com/wpc-eta-certification-for-smart-home-devices/)
- [WPC ETA fees overview](https://evtlindia.com/blog/wpc-eta-certificate-consultant)
