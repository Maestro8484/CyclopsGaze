# ESP32-S3 Migration: Feasibility Study

**Status: REPO-ONLY / PLANNING.** Nothing in this document has been built, flashed, or measured.
Every performance figure below is arithmetic or vendor spec, explicitly labeled as such. The one
number that decides the whole thing, achieved frame rate, is **unknown and unknowable without a
build**, and § 6 exists to get it cheaply before any refactoring is committed to.

Session: CG-S14 (2026-07-21). Driver: **BOM / public replicability** (operator, this session).

---

## 1. Why this is being considered

CyclopsGaze exists so the IRIS gaze path is publicly replicable after the Useful Sensors Person
Sensor was discontinued (see [CLAUDE.md](../CLAUDE.md), [README.md](../README.md)). Replicability has
a price ceiling as well as a sourcing one. The Teensy 4.1 is the single most expensive line item in
the current BOM, and it is not the part doing the hard work: the SEN0626 does its own inference.

This is **not** a response to any fault. Standalone tracking was bench-VERIFIED at CG-S8 and the
driver is live in IRIS. Nothing here changes that.

### This does not contradict the CG-S3 tombstone. Read that entry carefully.

[CHANGELOG.md](../CHANGELOG.md) CG-S3 Part A records that "the old ESP32-S3 + OV2640 approach
(`../OGLE`) was tombstoned/archived ... research favors T4.1 + SEN0626." A future session will hit
that line and this document and think one of them is wrong. Neither is. They are about different
machines doing different work:

| | OGLE (retired) | This study |
|---|---|---|
| What runs the face detection | **The ESP32-S3 itself** (OV2640 + on-chip esp-dl inference) | **The SEN0626**, on its own AI processor |
| The S3's job | Camera driver + neural network + tracking + link | Read 4 Modbus registers at ~6.7 Hz; draw eyes |
| Why it failed / whether that applies | Never reached reliable tracking at IRIS's real bench distance and lighting | **Does not apply.** That workload is gone. |

OGLE was retired because **on-chip vision on an S3 was not good enough**. This proposal asks the S3
to do no vision at all: the SEN0626 does its own inference and reports a face center, exactly as it
does for the Teensy today. The retired verdict is about a workload this design does not have.

What *is* new and unproven here is the render load (§ 5), which OGLE never carried. OGLE fed gaze to
a separate Teensy over USB-CDC and drew nothing. So the risk profile is not merely different from
OGLE's, it is close to disjoint.

## 2. BOM comparison

Prices verified 2026-07-21 by fetching the vendor pages this session. Not recalled.

| Line | Current (T4.1) | Proposed (S3 DualEye) |
|---|---|---|
| MCU | Teensy 4.1, **$31.50** (SparkFun, in stock) | |
| Display(s) | 1x GC9A01A module, price not verified this session (~$8-12 typical) | |
| MCU + 2 displays, integrated | | ESP32-S3-DualEye-LCD-1.28, **~$16-18** |
| Sensor | DFRobot SEN0626, **$14.90** | DFRobot SEN0626, **$14.90** |
| **Single-eye total** | **~$55-58** | **~$31-33** (and the second eye comes anyway) |
| **Dual-eye total** | **~$65-68** | **~$31-33** |

The dual-eye comparison is the honest one, because IRIS has two eyes and the integrated board has two
displays whether you use them or not. On that basis the S3 path is **roughly half the BOM**.

A premise worth recording as *checked and rejected*: before pricing this, the obvious objection was
"the AI camera dominates the BOM, so the MCU is the wrong lever." At **$14.90** the SEN0626 does not
dominate. The $31.50 Teensy does. The operator's instinct was right and the objection does not hold.

## 3. Board recommendation

### Most appropriate: **Waveshare ESP32-S3-DualEye-LCD-1.28**

ESP32-S3R8 (Xtensa LX7 dual-core @ 240 MHz), 512 KB internal SRAM, **8 MB PSRAM**, 16 MB flash, and
**two onboard 1.28in 240x240 GC9A01 round IPS panels**. ~$16-18.

Why it is the right part specifically for this project, not just a cheap S3:

- **It is the deliverable's shape.** Two round eyes driven by one MCU is exactly what CyclopsGaze's
  existing `DUAL_EYE` mode builds and what IRIS is. No adapter, no carrier, no harness.
- **It deletes the failure mode this repo actually keeps hitting.** Every bench failure in
  [ENGINEERING_LOG.md](ENGINEERING_LOG.md) has been *wiring*, not code: the I2C/UART DIP switch
  (CG-S4), and a bad connector sagging sensor VCC to 2.6 V (CG-S5). Replication instructions that say
  "wire ten display pins correctly" are where third parties will fail. Integrated panels remove that
  entire surface for the display half; only the 4-wire SEN0626 run is left to get wrong.
- **The onboard display topology appears to match the code already written.** Waveshare's docs list
  `LCD_CS2`, `LCD_RST2`, `LCD_BL2` alongside a single `LCD_DC`, i.e. shared SPI bus with per-panel CS,
  which is precisely the topology [config.h:142](../src/config.h#L142) already implements for
  `DUAL_EYE`, and for the same reason (a second SPI bus was not available). ⚠ **Exact GPIO numbers
  were not obtainable from the vendor docs this session** and are bring-up task #1.
- **The 8 MB PSRAM is load-bearing here**, for a specific reason. See § 5.

### Cheapest viable: the **ESP32-S3-DevKitC-1 N16R8 you already own**

For the *prototype* the answer is the spare on your shelf: it is the same ESP32-S3R8 silicon and the
same 8 MB PSRAM as the DualEye board. Everything in § 5 and § 6, the memory-allocation question and
the frame-rate kill gate, which are the only two things that can sink this, can be answered on the
N16R8 with a bare GC9A01A module wired up, at **zero additional spend**.

**Recommendation: prove it on the N16R8 you have, commit to the DualEye board only after § 6 passes.**
Do not buy anything yet.

## 4. Portability audit

Read this session against live source, not recalled.

| Component | Lines | Verdict |
|---|---|---|
| `eyes/EyeController.h` | 600 | **Ports free.** Read in full: pure C++ + `millis()`/`random()`. No Teensy API. |
| `eyes/eyes.h` | 213 | **Ports free**, minus the ARM-only `-Wpsabi` pragma at line 5 (guard it). |
| `eyes/240x240/*` LUT data | ~18k | **Ports free.** `PROGMEM` is a no-op on both; code dereferences directly, not via `pgm_read_byte`. |
| `displays/Display.h` | 31 | **Ports free.** This is the seam that makes the whole migration tractable. |
| `displays/GC9A01A_Display.*` | 158 | **Rewrite** against a new backend. ~150 lines. |
| `GC9A01A_t3n` (vendor lib) | | **Unportable, must be replaced.** See below. |
| `sensors/SEN0626Sensor.*` | 285 | **3 blockers**, all listed below. Contract-sensitive, see § 7. |
| `main.cpp` | 194 | **Minor**: `analogRead(A0)` seeding, USB-CDC-vs-UART0 boot behavior. |

### The display library cannot be ported

`GC9A01A_t3n`'s `library.json` advertises `"platforms": "*"`. That is false. The header is gated on
`__IMXRT1062__` / `__IMXRT1052__` / `KINETISK`, includes Teensy's `DMAChannel.h`, and drives
`IMXRT_LPSPI_t` / `KINETISK_SPI_t` peripheral register structs directly (29 such sites in the `.cpp`).
Nothing survives on Xtensa.

**Prior art, do not hand-roll:** LovyanGFX (best S3 story, with DMA and sprite/partial-update support)
or TFT_eSPI. Both have GC9A01A support. A hand-written SPI driver would be inventing over a mature
library for no reason.

**One feature must not be lost in the swap:** `GC9A01A_Display.cpp:22` calls
`updateChangedAreasOnly(true)`. Without an equivalent, every frame pushes the full 115,200-byte
buffer. At 20 MHz SPI that is a hard 21 FPS ceiling from the bus alone (~46 ms/frame); at 80 MHz it
is ~11.5 ms, so an S3 at full SPI clock survives even the naive case, but only because the S3 clocks
SPI faster, not because the cost went away.

### The three driver blockers

- [`SEN0626Sensor.h:99`](../src/sensors/SEN0626Sensor.h#L99), `elapsedMillis` members. Teensy-core-only type.
- [`SEN0626Sensor.cpp:68`](../src/sensors/SEN0626Sensor.cpp#L68), `serial.begin(testBaud)`. The S3 routes
  UART through a pin matrix; needs `begin(baud, SERIAL_8N1, rx, tx)`.
- [`SEN0626Sensor.cpp:84`](../src/sensors/SEN0626Sensor.cpp#L84), `while (millis() < BOOT_SETTLE_MS) {}`.
  **A latent bug the port exposes rather than causes.** A 2-second bare spin with no yield is fine on
  bare-metal Teensy; under FreeRTOS on the S3 it starves the idle task and risks a task-watchdog reset
  during the sensor's AI-model boot wait. The portable fix (a `delay()`-based wait, which yields on
  ESP32 and is equivalent on Teensy) is correct on **both** platforms.
  → **This one is worth flagging upstream to IRIS**, which runs the same code on a T4.1 today. It is
  harmless there, but it is the same latent shape. IRIS is READ-ONLY from here: flag, do not edit.

## 5. The memory architecture, the actual engineering problem

The render inner loop ([EyeController.h:364-443](../src/eyes/EyeController.h#L364)) performs **up to 5
random-access table reads per pixel**, over up to 57,600 pixels per frame:

| Table | Size | Access |
|---|---|---|
| `polarAngle_240[240*240]` | 57,600 B | random per pixel |
| `polarDist_240_125_69_0[240*240]` | 57,600 B | random per pixel |
| `disp_240_125[120*120]` | 14,400 B | 2 reads per pixel |
| `eyeIris[512*128]` uint16 | 131,072 B | random per pixel (iris band) |
| `eyeSclera[600*75]` uint16 | 90,000 B | random per pixel (sclera band) |
| **Total** | **~351 KB** | |

Plus a 240x240x16bpp framebuffer = **115,200 B per eye**.

On the Teensy 4.1 (600 MHz M7, generous cache) this is a solved problem. It is running today. On the
S3 (240 MHz LX7) the tables would default to flash `.rodata` behind the MMU cache, and ~351 KB of
*random* access against a cache measured in tens of KB is the single biggest risk in this migration.

**The allocation that makes it work (arithmetic, NOT measured):**

- LUTs + textures (~342 KB) → **copy into internal SRAM at boot**. 512 KB internal is the only fast
  memory on the part, and this is the data that needs it.
- Framebuffers (2 x 115,200 = 225 KB) → **PSRAM**. Sequential, DMA'd out to the panels, which is the
  access pattern PSRAM is actually good at.

This is exactly why the recommendation is an **R8 part and not a bare N8/no-PSRAM devkit**: PSRAM does
not fix the LUT problem (it is off-chip and cached through the same path, so putting LUTs there could
be *worse* than flash), but it frees the internal SRAM that does.

⚠ **The tight part:** 342 KB of 512 KB leaves ~170 KB for stack, heap, and the Arduino/IDF core, and
not all 512 KB is available to the application. This may not fit. **Fallback if it doesn't:** keep the
two large textures (221 KB) in flash, since their access has real spatial locality (`tx` tracks angle
and `ty` tracks distance, so adjacent pixels frequently hit adjacent texels) and put only the three
small hot LUTs (129,600 B) in SRAM. That is a comfortable fit.

**Upside not available on Teensy:** the S3 is dual-core. `renderFrame()` currently renders one eye per
call and alternates. Both eyes could render concurrently, pinned per core, with sensor/serial on the
other. That could recover a meaningful share of the 600 → 240 MHz gap. Option, not plan.

## 6. The kill gate. Do this before anything else.

**Do not port the project and then discover the frame rate.** The whole migration hinges on one number
nobody can reason to. Build a throwaway spike first:

1. N16R8 + any GC9A01A module (hardware you already own).
2. Link in the **real** `polarAngle_240`, `polarDist_240_125_69_0`, `disp_240_125`, `eyeIris`,
   `eyeSclera` tables, unmodified, from this repo.
3. Run the **real** `renderEye()` inner loop over a full 240x240 frame with the real access pattern.
   No sensor, no eyelids, no blink logic, no Display abstraction. Just the loop and the tables.
4. Time it. Report frames/sec three ways: tables in flash; tables in SRAM; tables in PSRAM.

That is perhaps a day's work, it answers the only question that matters, and it is cheap to be wrong
about. If it comes back at 25+ FPS the migration is a straightforward exercise. If it comes back in
single digits, the answer is to stop, and that is a *good* outcome for a day spent.

**Gate: >=20 FPS single-eye in the spike, or the migration does not proceed as scoped.**

## 7. The drop-in contract, the thing most at risk

`SEN0626Sensor.{h,cpp}` exists to be byte-identical to IRIS's copy. CG-S13 verified that by diff.
Every one of § 4's three blockers requires editing that file, and drift is precisely what this repo
exists to prevent.

**Do not fork the driver.** Two divergent copies would destroy the property the repo is built on.

**Recommended:** a small `src/platform.h` shim providing `elapsedMillis` on non-Teensy targets and one
`SEN0626_SERIAL_BEGIN(...)` macro. The driver body then stays *visually identical* to IRIS's, and the
entire platform delta is confined to one new file plus an `#include`. Prior art for the first half:
the standalone `elapsedMillis` library on the PlatformIO registry. Do not re-implement it. (⚠ confirm
it builds for Xtensa; not verified this session.)

**Second-order consequence to decide deliberately, not by accident:** if the standalone bench moves to
S3 while IRIS stays a Teensy 4.1, the bench no longer validates on the same silicon as the deploy
target. That property is a real part of why CG-S8's VERIFIED status meant something. Either accept it
explicitly, or keep a Teensy build target alive in `platformio.ini` as the contract reference. The
second is cheap (`[env:cyclopsgaze]` stays, `[env:cyclopsgaze_s3]` is added) and is recommended.

## 8. Staged plan

| Stage | Work | Gate |
|---|---|---|
| 0 | Frame-rate spike (§ 6) on the N16R8 you own | **>=20 FPS or stop** |
| 1 | `platform.h` shim; driver builds for both targets; fix the § 4 busy-wait | Teensy build byte-identical in behavior; `diff` vs IRIS still clean |
| 2 | Headless S3 build: sensor + `PS_CFG` + serial logging, no rendering | `[CG]` lines and `PS_CFG?` readback correct on S3 |
| 3 | New `Display` backend on LovyanGFX behind the existing CRTP seam | Single eye renders and tracks |
| 4 | Memory allocation per § 5; dual-eye; DualEye board bring-up | Full BENCH_PROTOCOL.md re-run |

Stage 2 has standalone value regardless of whether stages 3-4 ever happen: it would let the
outstanding CG-S12/CG-S13 verification work (raw-score gate, per-axis gain/bias, `PS_CFG` parser,
facing gate, all still UNVERIFIED) run on an S3 while the Teensy bench hardware is unavailable, since
every one of those is observable from the serial lines alone with no eye rendering involved.

## 9. Open questions

- Exact GPIO map for the DualEye board's two panels. Not obtainable from vendor docs this session.
- Whether ~342 KB of static tables actually fits alongside the Arduino/IDF core in 512 KB SRAM.
- Whether the `elapsedMillis` registry library builds for Xtensa.
- Whether `HardwareSerial::flush()` on ESP32 blocks until TX-complete. Modbus RTU framing in
  [`SEN0626Sensor.cpp:41`](../src/sensors/SEN0626Sensor.cpp#L41) depends on that semantic.
- Bare GC9A01A module price, for an accurate non-integrated comparison.

## Sources

- [SparkFun Teensy 4.1](https://www.sparkfun.com/teensy-4-1.html), $31.50, in stock
- [DFRobot SEN0626](https://www.dfrobot.com/product-2914.html), $14.90
- [Waveshare ESP32-S3-DualEye-LCD-1.28](https://www.waveshare.com/esp32-s3-dualeye-touch-lcd-1.28.htm)
- [ESP32-S3-DualEye-Touch-LCD-1.28 docs](https://docs.waveshare.com/ESP32-S3-DualEye-Touch-LCD-1.28)
- [Waveshare ESP32-S3-LCD-1.28](https://www.waveshare.com/esp32-s3-lcd-1.28.htm), single-panel sibling
