# Handheld remote v1 shell

Status: modelled, **not printed, not fitted**. Every part size is a nominal guess *(interp.)* until measured on the real parts.

Overall 68 x 162 x 34 mm (head 68 mm across, handle 46 mm wide). It is 34 mm thick because the dev board keeps its header pins and Dupont jumpers.

## Files
| File | What |
|---|---|
| `remote_v1.py` | Parametric source. Edit the numbers at the top, then in FreeCAD's Python console: `exec(open(r"...\cad\remote_v1.py").read())`. It rebuilds, checks for clashes, and re-exports. |
| `remote_v1.FCStd` | FreeCAD document with printed parts plus stand-in components (`mock_*`). |
| `out/*.stl` | `front_shell`, `back_shell`, `toe_clamp_a`, `toe_clamp_b`, `speaker_bridge`. |
| `out/remote_v1_printed.step`, `out/remote_v1_assembly.step` | STEP of the printed parts, and with the stand-ins. |

## Measure before printing (parameter in the script)
| Part | Measure | Parameter | Assumed |
|---|---|---|---|
| Display | Glass diameter | `DISP_GLASS_D` | 36.0 |
| Display | Round PCB diameter | `DISP_PCB_D` | 38.0 |
| Display | Glass face to PCB back | `DISP_STACK` | 4.5 |
| Display | Header row distance below centre | `DISP_TAB_Y` | -20.5 |
| Speaker | Diameter, depth (max depth 22: bridge needs 2.5 mm, display PCB back is at z=28.25) | `SPK_D`, `SPK_DEPTH` | 40, 20 |
| Board | Length, width | `BRD_L`, `BRD_W` | 64, 28 |
| Board | Tallest part on the chip side | `BRD_COMP_H` | 3.2 |
| Mic | Breakout diameter | `MIC_D` | 14.0 |
| Amp | Breakout width, length | `AMP_W`, `AMP_L` | 18, 19 |
| Inserts | Hole for your M3 insert | `INSERT_HOLE_D` | 4.0 |

If the display has mounting ears or a square PCB, or the speaker is not round, tell the next tool: the mounts need a redesign, not a number change.

## Printing
- PLA or PETG, 0.2 mm layers, 3 walls.
- `front_shell`: front face on the bed. `back_shell`: back face on the bed. Neither needs supports except the USB bay bridge (26 mm, fine).
- `speaker_bridge`: flat bar on the bed, legs up. `toe_clamp`: foot up.
- Print one switch cutout test first: a 14.0 mm square in a 1.5 mm plate. If the Outemu is loose or tight, change `SW_CUT` by 0.1.

## Hardware list
- 10 x M3 heat-set inserts (6 closing, 2 display, 2 speaker).
- 6 x M3x16 cap screws (closing, from the back). 2 x M3x6 (toe clamps). 2 x M3x12 (speaker bridge).
- 3 x Outemu switches and MX keycaps. Dupont female jumpers. Thin foam pad for the speaker magnet.

## Wiring
3V3 logic everywhere. Display and mic pins match the current firmware; amp and key pins are proposals *(interp.)* from `STATUS.md`, not yet in firmware.

| Part | Pin | ESP32-S3 GPIO |
|---|---|---|
| GC9A01 display | SCL | 8 |
| | SDA | 7 |
| | CS | 6 |
| | DC | 4 |
| | RST | 5 |
| | VCC / GND | 3V3 / GND |
| INMP441 mic | SCK | 12 |
| | WS | 13 |
| | SD | 14 |
| | L/R | GND |
| | VDD / GND | 3V3 / GND (never 5 V) |
| MAX98357A amp | BCLK | 15 |
| | LRC | 16 |
| | DIN | 17 |
| | VIN / GND | 5V / GND |
| | SD, GAIN | leave open (default: on, 9 dB) |
| | Speaker + / - | speaker terminals |
| Key 1 (top), Talk | one leg | 9, other leg GND |
| Key 2, Approve | one leg | 10, other leg GND |
| Key 3, Deny | one leg | 11, other leg GND |

| Dial pot (10k linear) | outer legs | 3V3 and GND |
| | middle leg (wiper) | 1 (ADC1, proposal) |

Dial: a 9 mm pot (RV09 / RK09 type, 6 mm D shaft) sits in the back shell on the right side of the neck, shaft pointing to the front. `dial_wheel.stl` (21 mm thumbwheel) pushes onto the shaft and stands 3 mm out of the side wall. Sizes are the `POT_*` and `WHEEL_*` parameters; all unmeasured. `out/dial_knob.stl` is from the earlier front-dial version and is no longer used.

Fitting the dial: solder three wires to the pot and bend its legs flat. Slide the pot under the plate from the head side so the bushing enters the U slot, then fix it with its M7 nut (threaded bushing) or hot glue. Push the wheel on, rim side down. Do this before step 10; the front shell's side notch drops over the wheel. Print the wheel rim-down, hub up.

Keys use the internal pull-ups (pressed = LOW). The amp is rated about 3 W into 4 ohm, so it cannot overdrive a 4 W speaker, but check the speaker's impedance: 4 or 8 ohm is fine, lower is not. USB can sag at high volume; keep volume moderate on v1.

## Assembly order
1. Melt the 10 inserts: 6 into the front-shell bosses at the split face, 2 into the display bosses, 2 into the tall speaker bosses in the back shell.
2. Solder wires to the mic and amp (do not fit header pins to them; there is no room for jumpers there). Solder two wires to each switch.
3. Front shell, face down: drop the display into its recess, header toward the handle. Fit both toe clamps with M3x6 so the feet press on the PCB edge. Do not overtighten on the glass.
4. Push the mic into its ring with the sound port over the 2.5 mm hole (the port is not always at the centre of the breakout; align by eye). Fix with a dab of hot glue on the rim, not over the port.
5. Click the three switches into the well from the front.
6. Back shell: seat the speaker in its ring, cone toward the grille, wires through the notch. Foam on the magnet, then the bridge with M3x12.
7. Lay the amp in its frame (double-sided tape or hot glue). Screw the speaker wires into its terminal.
8. Lay the board in the cradle chip side down, pins pointing up, USB ports in the bay.
9. Plug the jumpers onto the board pins and connect everything per the table. Bend jumpers outward toward the side walls, away from the switches.
10. Close the shells (lip on the front shell drops into the back shell) and fit 6 x M3x16 from the back. Two ribs in the front shell press the board down; if the shell will not close, a wire is trapped under a rib.

## Known limits of this model
- The board is held by pads under the module can and the USB shells, plus the two ribs. This depends on your clone's layout.
- BOOT and RST on the board are not reachable with the shell closed. Auto-reset over USB handles flashing with the current firmware.
- No battery, vibration motor or lanyard point.
- The bottom of the handle has no screw; the lip and the lower pair of screws 16 mm above it hold it shut.
