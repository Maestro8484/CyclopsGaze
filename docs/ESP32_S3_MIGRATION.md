# ESP32-S3 Migration: Feasibility Study

**Status: REPO-ONLY / PLANNING.** Nothing here has been built, flashed, or run. No ESP32-S3
hardware has executed a line of this code. Figures below are either measured on the host
(simulation over the real data, or the linked Teensy binary) or vendor spec, and each is
labeled. The one number that decides the migration, achieved frame rate, is **unknown**, and
§ 6 exists to get it cheaply before any refactoring starts.

Session: CG-S14 (2026-07-21). Driver: **BOM cost and public replicability**.

Parts, vendors and build totals live in **[BOM.md](BOM.md)**. This document is the engineering
case, not the shopping list.

---

## 1. Why this is being considered

CyclopsGaze exists so the IRIS gaze path stays publicly replicable after the Useful Sensors
Person Sensor was discontinued. Replicability has a price ceiling and a sourcing one. The
Teensy is the most expensive part in the build, it is single-sourced from PJRC, and it is not
the part doing the hard work since the SEN0626 runs its own inference.

This is **not** a response to any fault. Standalone tracking was bench-VERIFIED at CG-S8 and
the driver is live in IRIS.

### This does not contradict the CG-S3 tombstone. Read that entry carefully.

[CHANGELOG.md](../CHANGELOG.md) CG-S3 Part A records that "the old ESP32-S3 + OV2640 approach
(`../OGLE`) was tombstoned/archived ... research favors T4.1 + SEN0626." A future session will
hit that and this document and think one is wrong. Neither is. Different machines, different
work:

| | OGLE (retired) | This study |
|---|---|---|
| What runs face detection | **The ESP32-S3 itself** (OV2640 + on-chip esp-dl inference) | **The SEN0626**, on its own AI processor |
| The S3's job | Camera driver + neural net + tracking + link | Read 4 Modbus registers at ~6.7 Hz; draw eyes |
| Does the retirement apply | Failed at IRIS's real bench distance and lighting | **No.** That workload does not exist here. |

OGLE was retired because on-chip vision on an S3 was not good enough. This proposal gives the
S3 no vision workload at all. What is new and unproven here is the render load, which OGLE
never carried since it fed gaze to a separate Teensy and drew nothing.

## 2. The correction that reshaped this study

**An earlier draft of this document claimed the lookup tables must be copied into internal
SRAM, and derived a hard "one eye design only" limit from that. Both were wrong.** The premise
was never checked. It has now been checked and it is false.

Reading the linked Teensy binary (`arm-none-eabi-size -A firmware.elf`):

```
.text.progmem     352136   1610622324     <- 0x60002574, external QSPI flash
.data               9920    536870912     <- 0x20000000, DTCM
.bss                2240    536880832     <- DTCM
.bss.dma           12416    538968064     <- 0x20200000, OCRAM
```

**The Teensy 4.1 already runs all 352 KB of lookup tables directly out of external QSPI flash
through its cache**, with only 12 KB of variables in RAM. The bench-VERIFIED build does
exactly the thing that was being treated as the S3's disqualifying risk. It is not a
categorical problem, it is the normal operating mode of the board this project already trusts.

Consequences:

- Tables in flash is the **baseline**, not a compromise. The S3 does what the Teensy does.
- SRAM residency is an **optimization the S3 has available and the Teensy does not**, to be
  spent only if measurement says it is needed.
- The "one eye design" limit is **void**. Two designs is 572 KB of flash against 8 or 16 MB.
- **PSRAM is not required.** The two 115,200-byte framebuffers total 225 KB inside 512 KB of
  SRAM. Buy PSRAM only because the no-PSRAM devkit is discontinued, not because this needs it.

## 3. Board recommendation

**ESP32-S3-DevKitC-1-N8R8. $15.00, 704 in stock at DigiKey.**

The deciding spec is **data cache size**, because this workload thrashes it (§ 5). The S3's
data cache is configurable to 32 or 64 KB, which is the good end of the measured range.

Order the plain `-1`, not `-1U`; the external-antenna variant showed zero stock and an 8 week
lead. The no-PSRAM `-N8` is discontinued, so `-N8R8` is the part regardless of § 2.

On the replicability argument specifically: the ESP32-S3-WROOM-1-N8 module is $5.66 in ones
and $3.84 in volume, second-sourced through dozens of devkit vendors, with mature Arduino and
PlatformIO support. That is the opposite of the single-vendor PJRC situation, which is the
actual complaint driving this study.

### Rejected: Raspberry Pi Pico 2 / RP2350

Tempting at $5 and arguably the most replicable board in existence, but wrong for this
workload. **RP2350's XIP cache is 16 KB**, which § 5's simulation puts at a 26% miss rate
against 4.9 to 8.6% for the S3, so roughly 3 to 5x the flash stalls per frame. It also runs
150 MHz against 240. And you cannot dodge it via SRAM: 350 KB of tables plus two 115 KB
framebuffers is 580 KB against 520 KB available. Does not fit for two eyes.

### Rejected: ESP32-P4

768 KB SRAM and 400 MHz would erase every constraint here, but Arduino support was still beta
as of April 2026 with no official PlatformIO support, only a community fork. For a project
whose deliverable is *someone else can build this*, an unofficial toolchain fork is a worse
tax than a tight memory budget. Its round-display boards are also 720x720 and 800x800 MIPI
panels, not 240x240 SPI, so the geometry tables would all need regenerating.

### Rejected: integrated dual-round-display boards

An earlier draft recommended the Waveshare ESP32-S3-DualEye-LCD-1.28. Withdrawn. It bundles
two displays into the board price, which is only a win if you do not already own displays.

## 4. Portability audit

Read against live source. Line counts are actual.

| Component | Lines | Verdict |
|---|---|---|
| `eyes/EyeController.h` | 600 | **Ports free.** Read in full: pure C++ plus `millis()`/`random()`. No Teensy API. |
| `eyes/eyes.h` | 213 | **Ports free**, minus the ARM-only `-Wpsabi` pragma at line 5. Guard it. |
| `eyes/240x240/*` table data | ~18k | **Ports free.** `PROGMEM` is a no-op on both; tables are dereferenced directly, not via `pgm_read_byte`. |
| `displays/Display.h` | 31 | **Ports free.** The seam that makes this tractable. |
| `displays/GC9A01A_Display.*` | 158 | **Rewrite** against a new backend, roughly 150 lines. |
| `GC9A01A_t3n` (vendor lib) | | **Unportable, must be replaced.** |
| `sensors/SEN0626Sensor.*` | 285 | **3 blockers**, below. Contract-sensitive, see § 7. |
| `main.cpp` | 194 | **Minor**: `analogRead(A0)` seeding, USB-CDC versus UART0 boot behavior. |

### The display library cannot be ported

`GC9A01A_t3n`'s `library.json` advertises `"platforms": "*"`. That is false. The header is
gated on `__IMXRT1062__` / `__IMXRT1052__` / `KINETISK`, includes Teensy's `DMAChannel.h`, and
drives `IMXRT_LPSPI_t` / `KINETISK_SPI_t` peripheral registers directly (29 sites in the
`.cpp`). Nothing survives on Xtensa.

**Prior art, do not hand-roll:** LovyanGFX (best S3 story, DMA plus partial-update support) or
TFT_eSPI. A hand-written SPI driver would be inventing over a mature library.

**One feature must survive the swap:** `GC9A01A_Display.cpp:22` calls
`updateChangedAreasOnly(true)`. Without an equivalent, every frame pushes the full
115,200-byte buffer, a hard 21 FPS ceiling at 20 MHz SPI from bus time alone.

### The three driver blockers

- [`SEN0626Sensor.h:99`](../src/sensors/SEN0626Sensor.h#L99), `elapsedMillis` members.
  Teensy-core-only type.
- [`SEN0626Sensor.cpp:68`](../src/sensors/SEN0626Sensor.cpp#L68), `serial.begin(testBaud)`.
  The S3 routes UART through a pin matrix and needs `begin(baud, SERIAL_8N1, rx, tx)`.
- [`SEN0626Sensor.cpp:84`](../src/sensors/SEN0626Sensor.cpp#L84),
  `while (millis() < BOOT_SETTLE_MS) {}`. **A latent bug the port exposes rather than causes.**
  A 2-second bare spin with no yield is fine on bare-metal Teensy; under FreeRTOS on the S3 it
  starves the idle task and risks a task-watchdog reset during the sensor's AI boot wait,
  presenting as a boot loop before the eye ever draws. The portable fix (a `delay()`-based
  wait, which yields on ESP32 and is equivalent on Teensy) is correct on **both** platforms.
  → Worth flagging upstream to IRIS, which runs the same code on a T4.1. Harmless there, same
  shape. IRIS is read-only from here: flag, do not edit.

## 5. Measured render cost

Numbers below come from replaying `renderEye()`'s exact address arithmetic
([EyeController.h:326-445](../src/eyes/EyeController.h#L326)) over the real tables on the host,
at centred gaze in steady state. Simulation of the real access pattern over real data, not an
estimate. Not a hardware measurement either.

- **43,312 pixels drawn per frame** (not the full 57,600; the loop covers the eyelid aperture).
- **214,492 table reads per frame**, 4.95 per pixel.
- `drawAll` costs the **same** table reads as steady state. The extra area is flat eyelid fill.

Where the reads go, and how efficiently:

| Table | Size | Unique bytes used/frame | Fetched as 32 B lines | Waste |
|---|---|---|---|---|
| `disp_240_125` | 14,400 | 11,909 | 13,504 | 1.13x |
| `polarAngle_240` | 57,600 | 11,757 | 37,184 | **3.16x** |
| `polarDist_240_125_69_0` | 57,600 | 11,559 | 35,552 | **3.08x** |
| `eyeSclera` | 90,000 | 41,508 | 60,736 | 1.46x |
| `eyeIris` | 131,072 | 25,800 | 83,200 | **3.22x** |
| **total** | **350,672** | **102,533** | **230,176** | **2.24x** |

The polar tables and the iris texture are indexed with a stride of 240 bytes, so most of every
fetched cache line is discarded. Per frame the render **needs 100 KB and hauls 230 KB**.

Cache simulation (LRU, set-associative, single frame, cold start):

| Geometry | Miss rate | Misses/frame |
|---|---|---|
| 16 KB / 32 B / 4-way | 26.02% | 55,804 |
| 32 KB / 32 B / 8-way | 8.57% | 18,376 |
| 32 KB / 64 B / 8-way | 21.82% | 46,806 |
| 64 KB / 32 B / 8-way | **4.94%** | 10,594 |
| 64 KB / 64 B / 8-way | 5.35% | 11,479 |

**Counterintuitive and actionable: 64-byte lines are worse than 32-byte at 32 KB**, 21.8%
against 8.6%, because the striding wastes the extra bytes and halves effective capacity. The
S3's line size is a build setting, and the instinctive "bigger is better" choice is the wrong
one here. Configure 32 B lines and the largest cache available.

Across a sweep of 13 gaze positions and 3 pupil sizes, **79% of all table bytes get touched**
(269 KB of 342 KB). There is no small hot subset, so selective caching or partial SRAM
residency buys little. It is all-in or all-out.

The reproduction script is not committed; it parses the tables out of `src/eyes/240x240/` and
replays the loop. Regenerate it if these numbers ever need re-deriving.

## 6. The kill gate. Do this before anything else.

**Do not port the project and then discover the frame rate.** Two measurements, in order,
neither needing a new board.

1. **DONE at CG-S17 (2026-07-25). The Teensy baseline is 20 to 22 FPS.** Measured on a
   Teensy 4.1 driving one GC9A01A at **30 MHz SPI with async (DMA) updates**, reported over
   serial once a second, `nordicBlue`, tracking a live face. Two figures, because the same
   session measured both configurations:

   | Configuration | Measured | Full-frame bus ceiling |
   |---|---|---|
   | 10 MHz SPI, synchronous | **10 to 19 FPS**, mostly 10 to 12 | ~10.8 FPS |
   | 30 MHz SPI, async DMA | **20 to 22 FPS** | ~32 FPS |

   The first row lands exactly on its bus-time ceiling, which is the important structural
   finding for this document: **the render pushes near-full frames, so the display is
   bus-bound, not compute-bound**, because the eye aperture is 43,312 of 57,600 pixels and
   `updateChangedAreasOnly(true)` has little left to skip. The second row sits below its
   ceiling, so at 30 MHz the render itself starts to matter, which is where the § 6 table-read
   analysis finally bites.

   Consequence for the port: **an S3 must clear ~20 FPS, and SPI bandwidth is at least as
   decisive as cache behaviour.** A backend without an async/DMA path would lose roughly half
   the frame rate on its own, independent of how well the S3's cache handles the tables.
2. **Run the real inner loop over the real tables on an S3, record the number.** No sensor, no
   eyelids, no Display abstraction. Report it three ways: tables in flash, tables in SRAM,
   32 B versus 64 B cache lines.

**Gate: within a factor of two of the Teensy baseline, or reconsider.** If it misses badly the
honest answer is Teensy 4.0 at $23.80, which is a real cost reduction with zero port risk.

## 7. The drop-in contract, the thing most at risk

`SEN0626Sensor.{h,cpp}` exists to be byte-identical to IRIS's copy; CG-S13 verified that by
diff. All three § 4 blockers require editing that file, and drift is what this repo exists to
prevent.

**Do not fork the driver.** Two divergent copies destroy the property the repo is built on.

**Recommended:** a small `src/platform.h` shim providing `elapsedMillis` on non-Teensy targets
plus one `SEN0626_SERIAL_BEGIN(...)` macro. The driver body then stays visually identical to
IRIS's and the whole platform delta sits in one new file plus an include. Prior art for the
first half is the standalone `elapsedMillis` library on the PlatformIO registry; do not
reimplement it. (⚠ confirm it builds for Xtensa, not verified.)

**Decide deliberately:** if the bench moves to S3 while IRIS stays a Teensy 4.1, the bench no
longer validates on the same silicon as the deploy target, which is part of why CG-S8's
VERIFIED status meant something. Keep a Teensy build target alive in `platformio.ini` as the
contract reference. `[env:cyclopsgaze]` stays, `[env:cyclopsgaze_s3]` is added.

## 8. Staged plan

| Stage | Work | Gate |
|---|---|---|
| 0 | Teensy FPS baseline, then the S3 render spike (§ 6) | within 2x of baseline, or stop |
| 1 | `platform.h` shim; driver builds both targets; fix the § 4 busy-wait | Teensy behavior unchanged; `diff` vs IRIS still clean |
| 2 | Headless S3 build: sensor + `PS_CFG` + serial logging, no rendering | `[CG]` lines and `PS_CFG?` correct on S3 |
| 3 | New `Display` backend on LovyanGFX behind the existing CRTP seam | Single eye renders and tracks |
| 4 | Dual-eye, one eye per core | Full BENCH_PROTOCOL.md re-run, including step 10 |

Stage 2 has standalone value even if 3 and 4 never happen: it would let the outstanding
CG-S12/CG-S13 verification work (raw-score gate, per-axis gain/bias, `PS_CFG` parser, facing
gate, all still UNVERIFIED) run on an S3 while Teensy bench hardware is unavailable, since all
of it is observable from the serial lines with no rendering involved.

Note for stage 4: the S3 is dual-core and `renderFrame()` currently renders one eye per call
and alternates, so two eyes each run at half rate. One eye per core would claw back most of
the clock deficit exactly where two eyes need it. Reasoning, not measurement.

## 9. Open questions

- Frame rate on Teensy (never measured) and on S3 (never built). Everything else is bounded.
- Whether the `elapsedMillis` registry library builds for Xtensa.
- Whether `HardwareSerial::flush()` on ESP32 blocks until TX-complete. Modbus RTU framing at
  [`SEN0626Sensor.cpp:41`](../src/sensors/SEN0626Sensor.cpp#L41) depends on that semantic.
- Whether the S3 devkit's onboard 3.3V regulator has headroom for the SEN0626 under render
  load. It is smaller than the Teensy's, and CG-S5 was a sensor-undervolt failure.

## Sources

Prices and stock verified 2026-07-21 by fetching vendor pages. See [BOM.md](BOM.md).

- [DigiKey ESP32-S3-DevKitC-1-N8R8](https://www.digikey.com/en/products/detail/espressif-systems/ESP32-S3-DEVKITC-1-N8R8/15295894), $15.00, 704 in stock
- [DigiKey DFRobot SEN0626](https://www.digikey.com/en/products/detail/dfrobot/SEN0626/27526995), $14.90, 41 in stock
- [SparkFun Teensy 4.0](https://www.sparkfun.com/teensy-4-0.html) $23.80 / [Teensy 4.1](https://www.sparkfun.com/teensy-4-1.html) $31.50
- [Adafruit 6178 round GC9A01A](https://www.adafruit.com/product/6178), $17.50
- [Raspberry Pi Pico 2](https://www.raspberrypi.com/news/raspberry-pi-pico-2-our-new-5-microcontroller-board-on-sale-now/), $5, RP2350 with 16 KB XIP cache
