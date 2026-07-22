# Bill of Materials

What it costs to build an eye that watches people, and where to buy the parts.

If you are here from the animatronics/props side rather than the IRIS side: this is a
general-purpose **animatronic eye that tracks a real human face**, not a canned animation
loop. It suits Halloween props, cosplay helmets and masks, puppets, creature builds, and desk
pieces. The parts list is short, the wiring is seven wires per eye plus four for the camera,
and nothing needs a custom PCB.

**Prices verified 2026-07-21** by reading the vendor pages directly. They will drift. Stock
figures especially. Treat them as a snapshot, not a quote.

---

## Read this before you order

This repo uses **REPO-ONLY** (committed, not flashed), **DEPLOYED** (flashed), and
**VERIFIED** (flashed and behavior observed on a bench). That discipline applies to the BOM
too, so you know what you are buying into:

| Configuration | Status |
|---|---|
| Teensy 4.1 + **one** display + SEN0626 | **VERIFIED.** Direction, range and centering confirmed on a bench (CG-S8). |
| **Two** displays, any board | Compiles clean. **Never run on hardware.** BENCH_PROTOCOL.md step 10 has never been executed. |
| **ESP32-S3**, any configuration | **Nothing has been built.** Projected only. See [ESP32_S3_MIGRATION.md](ESP32_S3_MIGRATION.md). |

No frame rate has ever been measured on any board, because `SHOW_FPS` sits commented out at
`src/displays/Display.h:5` and no bench session has enabled it. So this document deliberately
does not promise you a smooth frame rate on anything. The Teensy single-eye build is known to
track a face correctly. That is the honest extent of it.

**If you want the build most likely to work today, buy the Teensy row.** If you want the
cheapest path and are willing to be the person who finds out whether the ESP32-S3 is fast
enough, buy the S3 row and please report back.

---

## Parts

### Microcontroller

| Part | Source | Price | Stock at check | Notes |
|---|---|---|---|---|
| **ESP32-S3-DevKitC-1-N8R8** | DigiKey | **$15.00** | 704 | Cheapest path. Dual core, 240 MHz. Unproven for this project. |
| Teensy 4.0 | SparkFun | $23.80 | in stock | Same silicon as the 4.1, identical pins for this build. |
| Teensy 4.1 | SparkFun | $31.50 | in stock | What the VERIFIED build used. |

Order the plain **`-1`**, not `-1U`. The `-1U` (external antenna) variant showed zero stock
and an 8 week lead. The no-PSRAM `-N8` devkit is discontinued, so `-N8R8` is simply the part
you buy; this project does not need the PSRAM.

Building your own board instead of using a devkit: the bare **ESP32-S3-WROOM-1-N8** module is
$5.66 at DigiKey in ones, $3.84 in volume.

### Display, one per eye

| Part | Source | Price | Notes |
|---|---|---|---|
| **Adafruit 6178**, 1.28" 240x240 round GC9A01A | Adafruit direct | **$17.50** ($15.75 at 10, $14.00 at 100) | Domestic, in stock. |
| Generic 1.28" 240x240 GC9A01A SPI module | Elecrow and many others | ~$8.60 | Offshore shipping. Functionally the same for this project. |

Any 1.28" 240x240 round GC9A01A SPI module works. Adafruit's carries an EYESPI connector,
microSD slot and level shifter that this project does not use, so you are paying for features
you will not touch. It is the convenient domestic option, not the cheap one.

**Do not substitute a different size or driver chip.** Every lookup table in `src/eyes/240x240/`
is generated for 240x240 round geometry. A different panel is a rebuild, not a swap.

### Sensor, one total regardless of eye count

| Part | Source | Price | Stock at check |
|---|---|---|---|
| **DFRobot SEN0626** Gravity AI vision camera | DigiKey (also DFRobot direct, same price) | **$14.90** | 41 |

One camera drives both eyes. It does its own face detection on-board, which is why a modest
microcontroller can keep up.

---

## Builds and totals

### Domestic sourcing

| Build | Board | Displays | Sensor | **Total** |
|---|---|---|---|---|
| Wandering eye, no face tracking | S3 $15.00 | 1x $17.50 | none | **$32.50** |
| One eye, tracks your face | S3 $15.00 | 1x $17.50 | $14.90 | **$47.40** |
| **Two eyes, track your face** | S3 $15.00 | 2x $35.00 | $14.90 | **$64.90** |
| *The VERIFIED build* | *T4.1 $31.50* | *1x $17.50* | *$14.90* | *$63.90* |
| Two eyes on known-good silicon | T4.0 $23.80 | 2x $35.00 | $14.90 | **$73.70** |

### With generic offshore displays

| Build | **Total** |
|---|---|
| Wandering eye, no tracking | **$23.60** |
| One eye, tracks your face | **$38.50** |
| Two eyes, track your face | **$47.10** |

Plus a USB-C cable and jumper wires you probably already own.

### Where the money actually goes

Domestically the **display is the dominant cost, not the microcontroller**. Two Adafruit
panels are $35.00, more than the board and camera combined at $29.90. If you are driving cost
down, display sourcing is a bigger lever than the choice of MCU. The camera at $14.90 has no
cheaper equivalent and is not worth optimizing.

---

## Ordering notes

**DigiKey covers the board and the camera but not the display.** Their only 1.28" round
GC9A01A listing is the Adafruit part (`1528-6178-ND`), marked obsolete and not stocked. So a
domestic build is two carts: DigiKey for the electronics, Adafruit for the panels.

DigiKey's "obsolete" status means "we stopped carrying it," not "it is dead." The Adafruit
6178 is marked obsolete there while Adafruit themselves have it in stock, and the same applies
to the `-N8` devkit while the `-N8R8` sits in stock at 704 units. Check the manufacturer
before believing a distributor's end-of-life flag.

---

## Two things that will eat your bench time

Both are drawn from this project's own bench history, not general advice. They are the only
two hardware faults that have actually cost this build a session.

1. **The SEN0626 has a physical I2C/UART DIP switch. Set it to UART.** This firmware is
   Modbus RTU over UART only with no I2C fallback. A board left in I2C mode reports "not
   found" with perfect wiring and correct code (CG-S4).
2. **Meter the camera's own VCC pin under load, not the board's 3.3V pin.** It should read
   about 3.2 to 3.3V. A bad connector once dropped it to 2.6V, which produces random resets
   and a mysteriously narrow detection range rather than an obvious failure (CG-S5).

Wiring tables, dual-eye pinout and power notes: **[WIRING.md](WIRING.md)**.
First-flash walkthrough: **[BENCH_PROTOCOL.md](BENCH_PROTOCOL.md)**.
