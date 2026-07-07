# CyclopsGaze — Complete Wiring Reference

Hardware:
- Teensy 4.1 (migrated from T4.0, CG-S2 — pin numbers are identical for every
  pin used here, so the T4.0 tables below apply unchanged)
- GC9A01A 240x240 round LCD (SPI) — one for single-eye, two for dual-eye
- DFRobot SEN0626 AI Camera (UART)

---

## Power Rails

Both the GC9A01A and SEN0626 run on 3.3V.
Teensy 4.0 3.3V pin can supply both.
DO NOT connect either to 5V — both are 3.3V logic only.

**Verify the SEN0626's own VCC pin reads ~3.2-3.3V under load, not just the
Teensy's 3.3V pin.** Confirmed on bench (2026-07-06): the Teensy pin measured a
healthy 3.25V while the sensor's VCC pin measured only 2.6V — the drop was in a
bad connector along the run, not the Teensy's regulator. An undervolted SEN0626
is a documented real-world failure mode for this sensor family (random resets,
frozen output, or degraded detection) — if tracking looks unreliable, check
this before touching any firmware tuning.

---

## Mounting Distance

DFRobot documents the SEN0626's detection range as **0.5–3 m (~19.7in–~9.8ft)**
for both gesture and face recognition — this is the vendor's own spec, not a
CyclopsGaze limitation. Mount it so a face is expected to sit **at least ~20
inches** from the lens; closer than that is out-of-spec and tracking
instability there is expected sensor behavior, not a bug. See NOTES.md
"External research" for sourcing and how this compares to the Person Sensor.

---

## Wire Color Legend

No color scheme exists for the CyclopsGaze eyes in IRIS — cross-checked against
`IRIS_ARCH.md`'s Teensy 4.1 pin table: IRIS documents the GC9A01A eyes only as
pin numbers, never wire colors (the one wire-color doc in that repo,
`docs/servo_teensy40_wiring.md`, is for the *separate* servo/gesture Teensy 4.0,
not the eyes). Nothing to inherit there, so the palette below is a **suggested**
scheme built from a standard 10-color wire kit (Black/Brown/Red/Orange/Yellow/
Green/Blue/Violet/Gray/White) with **zero repeats** across the single-eye +
sensor system. **SEN0626 TX/RX colors are fixed as you specified** (Green/Blue —
the DFRobot Gravity board silkscreens these pins "D/T" and "C/R" since the same
header is reused across digital and UART Gravity modules); every other color
was chosen to avoid colliding with those two. Swap in whatever you have on hand;
what matters is staying consistent, since the dual-eye section reuses these
same colors to show which wires are shared vs separate.

| Signal role                  | Wire color |
|-------------------------------|------------|
| VCC / 3.3V                    | Red        |
| GND                            | Black      |
| SCK (SPI clock)                | Yellow     |
| MOSI / SDA (SPI data)          | Orange     |
| CS — eye 0 / primary           | Violet     |
| DC (RS) — eye 0 / primary      | White      |
| RST — eye 0 / primary          | Gray       |
| BLK (backlight)                | Brown      |
| SEN0626 TX / "D/T" (sensor→Teensy) | Green  |
| SEN0626 RX / "C/R" (Teensy→sensor) | Blue   |

Dual-eye adds 3 more roles (eye 1's separate CS/DC/RST) beyond the 10-color kit.
Suggested: **Pink** (CS), **Tan** (DC), and reuse **Gray with a stripe/heat-shrink
flag** (RST) — a labeled repeat of eye 0's RST color, since the two are on
different connectors and unlikely to be confused, but the flag avoids any
ambiguity when both harnesses run side by side. See the dual-eye table below.

---

## GC9A01A Display → Teensy 4.0

| GC9A01A Pin | Teensy 4.0 Pin | Wire Color | Notes |
|---|---|---|---|
| VCC | 3.3V | Red | 3.3V ONLY — not 5V tolerant |
| GND | GND | Black | |
| SCK | 13 | Yellow | SPI clock (hardware SPI) |
| MOSI (SDA) | 11 | Orange | SPI data (hardware SPI) |
| CS | 10 | Violet | Chip select — NOT pin 0 (conflicts with Serial1 RX) |
| DC (RS) | 2 | White | Data/Command select |
| RST | 3 | Gray | Reset |
| BLK (LED) | 3.3V | Brown | Backlight — tie to 3.3V for always-on, OR wire to a free pin for software control |

**No MISO wire needed — display is write-only.**

---

## SEN0626 AI Camera → Teensy 4.0

**Onboard DIP switch — MUST be set to UART, not I2C.** The SEN0626 breakout has
a physical mode-select DIP switch. This firmware speaks Modbus RTU over UART
(Serial1) only — it has no I2C driver. Confirmed on bench 2026-07-06: with the
switch left on I2C (as-shipped/default), the sensor never answers and
`begin()` logs `SEN0626 NOT FOUND at 115200 or 9600`. Flip the switch to UART
before wiring TX/RX; there is no firmware workaround for the wrong mode.

| SEN0626 Pin | Teensy 4.0 Pin | Wire Color | Notes |
|---|---|---|---|
| VCC | 3.3V | Red | 3.3V ONLY |
| GND | GND | Black | |
| TX ("D/T") | 0 | Green | Teensy Serial1 RX — sensor transmits, Teensy receives |
| RX ("C/R") | 1 | Blue | Teensy Serial1 TX — Teensy transmits, sensor receives |

**TX→RX and RX→TX — always cross the wires. TX on sensor goes to RX on Teensy.**

---

## Full Connection Summary (all wires)

| Teensy 4.0 Pin | Wire Color | Direction | GC9A01A Display Pin |
|---|---|---|---|
| 3.3V | Red | → | VCC |
| GND | Black | → | GND |
| 13 | Yellow | → | SCK |
| 11 | Orange | → | MOSI (SDA) |
| 10 | Violet | → | CS |
| 2 | White | → | DC (RS) |
| 3 | Gray | → | RST |
| 3.3V | Brown | → | BLK (LED backlight) |

| Teensy 4.0 Pin | Wire Color | Direction | SEN0626 Camera Pin |
|---|---|---|---|
| 3.3V | Red | → | VCC |
| GND | Black | → | GND |
| 0 | Green | ← | TX / "D/T" (sensor → Teensy) |
| 1 | Blue | → | RX / "C/R" (Teensy → sensor) |

Total wires: 12

---

## Pin Conflict Notes

- CS was remapped from pin 0 (IRIS left-eye default) to pin 10.
  Pin 0 = Serial1 RX = SEN0626 TX line. Cannot double-use.
- Pin 10 is hardware CS-capable on T4.0 SPI bus. No performance penalty.

---

## When Teensy 4.1 Arrives

Same wiring works unchanged on T4.1.
Only change: `board = teensy41` in platformio.ini.
Pin numbers are identical between T4.0 and T4.1 for all pins used here.

---

## Baud Rate Auto-Detection

No USB-UART adapter needed. Firmware auto-detects baud rate on first boot:
1. Waits until ~2 s after power-on so the SEN0626 finishes booting its AI model
   into RAM before the first probe (`BOOT_SETTLE_MS`, CG-S3).
2. Tries 115200 — ~200 ms line settle, then a Modbus PID read.
3. Falls back to 9600 — same.
4. Retries the whole sweep up to 3 times so a cold sensor that misses the first
   probe is still found (`BAUD_ATTEMPTS`, CG-S3).
5. Prints the confirmed baud rate + attempt number to the serial monitor.
6. Document the confirmed baud rate in NOTES.md.

---

## Dual-Eye (optional)

Single-eye is the default and needs no code change. To drive a **second**
GC9A01A, uncomment `#define DUAL_EYE` in `src/config.h` and wire the second
display as below.

### Why not the IRIS two-bus layout

IRIS runs its two eyes on two separate SPI buses (eye 0 on SPI1 = MOSI 26 /
SCK 27 / CS 0; eye 1 on SPI0 = MOSI 11 / SCK 13 / CS 10) so both can DMA in
parallel. **That layout cannot be copied here.** On CyclopsGaze the SEN0626
owns Serial1 = pins **0 (RX)** and **1 (TX)**, and Teensy 4.1's SPI1 collides
with *both*: its hardware chip-select is pin **0** and its default MISO is pin
**1**. Bringing up SPI1 would clobber the sensor UART.

**Resolution:** keep both displays on **SPI0** (shared MOSI 11 / SCK 13) with a
separate CS per display. SPI0's MISO (pin 12) is free and unused by the
write-only displays, so there is no conflict. Updates are synchronous and the
render loop draws one eye per pass, so the shared bus has no contention — the
only cost is that each eye's refresh rate roughly halves. Fine for a bench
testbed.

### Second display → Teensy 4.1

| GC9A01A (eye 1) Pin | Teensy 4.1 Pin | Wire Color | Notes |
|---|---|---|---|
| VCC | 3.3V | Red | 3.3V ONLY |
| GND | GND | Black | |
| SCK | 13 | Yellow | SHARED with eye 0 (SPI0 clock) |
| MOSI (SDA) | 11 | Orange | SHARED with eye 0 (SPI0 data) |
| CS | 9 | Pink | Separate chip select (eye 0 uses 10 / Violet) |
| DC (RS) | 8 | Tan | Separate data/command (eye 0 uses 2 / White) |
| RST | 6 | Gray + stripe | Separate reset (eye 0 uses plain Gray on pin 3); flag this wire so it's not confused with eye 0's RST — or tie to 3.3V and set RST=-1 |
| BLK (LED) | 3.3V | Brown | Backlight always-on (shared with eye 0) |

### Dual-eye pin map (both displays)

| Signal | Eye 0 (primary) Pin | Eye 0 Wire Color | Eye 1 (second) Pin | Eye 1 Wire Color | Shared? |
|---|---|---|---|---|---|
| SCK | 13 | Yellow | 13 | Yellow | yes (SPI0) |
| MOSI | 11 | Orange | 11 | Orange | yes (SPI0) |
| CS | 10 | Violet | 9 | Pink | no |
| DC | 2 | White | 8 | Tan | no |
| RST | 3 | Gray | 6 | Gray + stripe | no |
| mirror flag | true | — | false | — | (matches IRIS left/right) |

`eyeInfo[]` in `src/config.h` (DUAL_EYE branch) encodes exactly this. Eye 0
keeps `mirror=true` (and the EyeController's built-in eye-0 X-flip); eye 1 is
`mirror=false`, so the two eyes track together toward the same face.

No new conflict with Serial1: the added pins (9, 8, 6) are all free, and the
shared SPI0 pins (11, 13) never touch pins 0/1.

---

## Quick Sanity Check Before First Flash

- Display VCC and sensor VCC both on 3.3V (not 5V)
- GND shared between Teensy, display, and sensor
- SEN0626 TX → Teensy pin 0 (not pin 1)
- SEN0626 RX → Teensy pin 1 (not pin 0)
- Display CS on pin 10 (not pin 0)
- Dual-eye only: second display CS on pin 9, DC on pin 8, RST on pin 6;
  SCK/MOSI shared with eye 0 on pins 13/11; `#define DUAL_EYE` uncommented
