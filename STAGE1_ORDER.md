# Stage 1 order list

Checked 2026-11-06 from web listings. Prices and stock change, so confirm at checkout. Nothing has been ordered. Items marked *(interp.)* are estimates.

## Latest: 1.46" round touch board (1.46B) is the first choice
Waveshare ESP32-S3-Touch-LCD-1.46B has the microphone and speaker built in and still meets the ₹4,000 target.

| Item | Budget | Notes |
|---|---|---|
| ESP32-S3-Touch-LCD-1.46B | ₹2,249–3,061 | Hubtronics ₹2,249, 0 in stock; Zbotic ₹3,060.92, shown in stock, confirm the variant; Tenettech about ₹2,766 with 20–25 working days shipping |
| Battery, leads, motors, driver parts, filament, shipping | ₹791 | As in the list below |
| Clear window for the shell | ₹50 *(interp.)* | The board has no cover glass |
| **Core kit total** | **About ₹3,100–3,900** | |

**Do not buy from ElectroPi.** Its public complaints allege payment without delivery and no response from support, and scanner and forum reports warn about the site. These are unverified, but the risk isn't worth the saving.

Pay by card, a protected checkout, or cash on delivery where offered. Avoid direct bank transfers to any seller you haven't used.

- **Has:** 412×412 round IPS touch screen, mic, speaker, 6-axis IMU, RTC, TF slot, battery header and charger, programmable buttons.
- **Watch out:** no cover glass, so the bare screen needs protecting. Touch through a window is untested, so rely on buttons for approvals.
- **Vibration:** spare pins are on a fine 1.27 mm 20-pin header, so wiring is fiddlier. Check the schematic.
- **Same risk:** standby current isn't published, so measure it.
- **Not listed by Agent Buddy:** the wear test needs our own firmware.

## Backup: 1.69" touch board
Waveshare ESP32-S3-Touch-LCD-1.69 is the cheaper backup if voice is dropped.

| Item | Budget | Notes |
|---|---|---|
| ESP32-S3-Touch-LCD-1.69 (touch version) | ₹2,200 | Hubtronics ₹1,999; Robu about ₹2,203, unconfirmed; Zbotic ₹2,830.82; Robocraze sold out |
| Battery, leads, motors, driver parts, filament, shipping | ₹791 | As in the list below |
| I2S or PDM microphone, if voice stays in version 1 | ₹150 *(interp.)* | Not onboard; wiring untested |
| **New core kit total** | **About ₹3,000–3,150** | |

- **Gains:** about ₹2,000 cheaper, onboard buzzer, RTC, a text-friendly 240×280 screen, and documented spare GPIOs (2, 3, 17, 18).
- **Losses:** no microphone or speaker, an LCD rather than AMOLED, and no listed Agent Buddy support.
- **Check:** buy the touch version. Robu's ₹1,619 listing is the non-touch board.
- **Same risk:** standby current isn't published, so measure it.

The AMOLED list below is now optional, as a comparison for looks and voice.

## Order (original AMOLED kit)
| Item | Qty | Budget | Best listing found | Stock |
|---|---|---|---|---|
| Waveshare ESP32-S3-Touch-AMOLED-1.75 | 1 | ₹4,250 | Zbotic ₹3,799.60 (not confirmed live); Robu ₹4,199; Hubtronics ₹4,239 | Hubtronics out of stock; Robu listed out of stock on an old page; Zbotic unknown |
| 3.7V 400 mAh LiPo with protection circuit | 1 | ₹209 | Robocraze Witty Fox, about ₹180–209 | Connector and stock unconfirmed |
| 2-pin 1.25 mm lead | 2 | ₹36 | DNA Tech India, ₹17.94 each | Check it fits MX1.25 |
| 3V coin vibration motor | 3 | ₹66 | Robocraze, ₹22 each | Confirm exact model |
| Motor driver parts: small transistor, diode, resistor | 1 set | ₹30 *(interp.)* | Any local shop | n/a |
| Filament for test shells | Some | ₹300 *(interp.)* | Skip if you have some | n/a |
| Shipping across stores | | ₹150 *(interp.)* | Some orders ship free | n/a |
| **Core kit total** | | **About ₹5,040** | | |

## Optional
| Item | Budget | Notes |
|---|---|---|
| M5Stack StickS3 | ₹2,500–3,500 *(interp.)* | No Indian listing found; $21.50 from M5Stack. Check Robocraze, the authorised distributor |
| Multimeter with mA range | ₹1,500 *(interp.)* | Skip if you own one |

**All-in total: about ₹9,000–10,000.**

## Before paying
1. Confirm the seller has the board in stock. Hubtronics is closed 7–10 November.
2. Buy the standard or -B board. The 1.75C is a different product, with no RTC or TF slot.
3. Agent Buddy names the -B and C boards. The -B is the same board with a case, so prefer it if the price is similar.
4. Check the battery fits. Measure the board using Waveshare's drawing, because no numeric size is listed.
5. Check the battery's connector. If it isn't MX1.25, join it to the 2-pin lead.

## Battery safety
- Check polarity with a multimeter before plugging in. Connector colours are not reliable, and Waveshare warns against reversing it.
- Cut and join one wire at a time, so the leads can't touch.
- Use a protected cell, and avoid the cheapest unbranded ones. Waveshare warns against low-quality cells.
- Don't charge it unattended, and stop using it if it swells.

## Vibration wiring
The board has no listed vibration motor. Its 8-pin header has 3 GPIOs, but Waveshare's wiki page doesn't list which pins. Check the schematic first. Never drive the motor directly from a GPIO pin; use the transistor and diode.

## Effect on the ₹4,000 target
Indian retail is now ₹3,800–4,240 for the board alone, so a finished unit is about ₹4,200–4,600 *(interp.)*.

## Sources
- [Hubtronics listing](https://hubtronics.in/esp32-s3-touch-amoled-1.75)
- [Zbotic listing](https://zbotic.in/product/waveshare-esp32-s3-1-75inch-amoled-round-touch-display-development-board/)
- [Robu listing](https://robu.in/product/esp32-s3-1-75inch-amoled-round-touch-display-development-board/)
- [Waveshare product page](https://www.waveshare.com/esp32-s3-touch-amoled-1.75.htm)
- [Waveshare wiki](https://www.waveshare.com/wiki/ESP32-S3-Touch-AMOLED-1.75)
- [Robocraze Witty Fox 400 mAh](https://robocraze.com/products/witty-fox-400mah-rechargeable-3-7v-lithium-polymer-battery)
- [DNA Tech 2-pin 1.25 mm lead](https://www.dnatechindia.com/2-pin-1.25-mm-jst-female-connector-with-wire-buy-in-india.html)
- [Robocraze coin vibration motor](https://robocraze.com/products/coin-type-micro-vibration-motor)
- [Robocraze M5Stack range](https://www.robocraze.com/collections/m5stack)
- [M5Stack StickS3 overview](https://www.cnx-software.com/2026/01/23/m5stack-sticks3-esp32-s3-mini-iot-dev-kit-features-1-14-inch-color-display-speaker-and-mic-for-voice-interaction/)
