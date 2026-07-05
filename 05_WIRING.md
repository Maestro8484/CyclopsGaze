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

---

## GC9A01A Display → Teensy 4.0

```
GC9A01A Pin    Teensy 4.0 Pin    Notes
-----------    --------------    -----
VCC            3.3V              3.3V ONLY — not 5V tolerant
GND            GND
SCK            13                SPI clock (hardware SPI)
MOSI (SDA)     11                SPI data (hardware SPI)
CS             10                Chip select — NOT pin 0 (conflicts with Serial1 RX)
DC (RS)        2                 Data/Command select
RST            3                 Reset
BLK (LED)      3.3V              Backlight — tie to 3.3V for always-on
                                 OR wire to a free pin for software control
```

**No MISO wire needed — display is write-only.**

---

## SEN0626 AI Camera → Teensy 4.0

```
SEN0626 Pin    Teensy 4.0 Pin    Notes
-----------    --------------    -----
VCC            3.3V              3.3V ONLY
GND            GND
TX             0                 Teensy Serial1 RX — sensor transmits, Teensy receives
RX             1                 Teensy Serial1 TX — Teensy transmits, sensor receives
```

**TX→RX and RX→TX — always cross the wires. TX on sensor goes to RX on Teensy.**

---

## Full Connection Summary (all wires)

```
Teensy 4.0                GC9A01A Display
----------                ---------------
3.3V        ----------->  VCC
GND         ----------->  GND
Pin 13      ----------->  SCK
Pin 11      ----------->  MOSI (SDA)
Pin 10      ----------->  CS
Pin  2      ----------->  DC (RS)
Pin  3      ----------->  RST
3.3V        ----------->  BLK (LED backlight)

Teensy 4.0                SEN0626 Camera
----------                --------------
3.3V        ----------->  VCC
GND         ----------->  GND
Pin  0      <-----------  TX   (sensor → Teensy)
Pin  1      ----------->  RX   (Teensy → sensor)
```

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

```
GC9A01A (eye 1) Pin   Teensy 4.1 Pin   Notes
-------------------   --------------   -----
VCC                   3.3V             3.3V ONLY
GND                   GND
SCK                   13               SHARED with eye 0 (SPI0 clock)
MOSI (SDA)            11               SHARED with eye 0 (SPI0 data)
CS                    9                Separate chip select (eye 0 uses 10)
DC (RS)               8                Separate data/command
RST                   6                Separate reset (or tie to 3.3V and set RST=-1)
BLK (LED)             3.3V             Backlight always-on
```

### Dual-eye pin map (both displays)

```
Signal        Eye 0 (primary)   Eye 1 (second)   Shared?
------        ---------------   --------------   -------
SCK           13                13               yes (SPI0)
MOSI          11                11               yes (SPI0)
CS            10                 9               no
DC             2                 8               no
RST            3                 6               no
mirror flag   true              false            (matches IRIS left/right)
```

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
