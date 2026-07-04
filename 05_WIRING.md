# CyclopsGaze — Complete Wiring Reference

Hardware:
- Teensy 4.0
- GC9A01A 240x240 round LCD (SPI)
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
1. Tries 115200 — waits 500ms for data
2. Falls back to 9600 — waits 500ms for data
3. Prints confirmed baud rate to serial monitor
4. Document confirmed baud rate in NOTES.md

---

## Quick Sanity Check Before First Flash

- Display VCC and sensor VCC both on 3.3V (not 5V)
- GND shared between Teensy, display, and sensor
- SEN0626 TX → Teensy pin 0 (not pin 1)
- SEN0626 RX → Teensy pin 1 (not pin 0)
- Display CS on pin 10 (not pin 0)
