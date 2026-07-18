# CyclopsGaze

**A face-tracking gaze demo on a single round LCD "eye" — and a drop-in replacement for the discontinued Useful Sensors Person Sensor.**

CyclopsGaze runs Chris Miller's [TeensyEyes](#credits) realistic "uncanny eyes" on a
1.28"/240×240 GC9A01A round display and makes the eye **look at your face** — driven by a
DFRobot **SEN0626** AI vision camera over UART. Its real job is to prove that the SEN0626
can stand in, byte-for-byte, for the now-**discontinued** Useful Sensors *Person Sensor*
(SEN-21231) that drives the gaze "soul" of the [IRIS](#relation-to-iris) robot face.

![CyclopsGaze tracking a face](docs/media/cyclopsgaze_tracking.jpg)
<!-- photos live in docs/media/ — see docs/media/README.md -->

---

## Why this exists

TeensyEyes renders gorgeous animated eyes, but on its own the eye just wanders. To give it a
"soul" — to make it *watch you* — you need a face-detection sensor feeding a face position into
the eye's gaze target. In the parent [IRIS](#relation-to-iris) project that sensor was the
Useful Sensors **Person Sensor (SEN-21231)**: a tiny I²C module that returns face bounding
boxes. It worked well. Then Useful Sensors **discontinued it**, which is a problem for a
project meant to be publicly replicable — buyers can't source a dead part.

CyclopsGaze is the standalone testbed built to answer one question:

> **Can the DFRobot SEN0626 (Modbus RTU over UART) be a true drop-in for the Person Sensor —
> same struct, same API, same gaze behavior — so the eyes keep their soul?**

Answer, as of this repo: **yes.** The SEN0626 driver here presents the *exact*
`person_sensor_face_t` struct and method surface IRIS already consumes, so the parent project
swapped it in with (almost) no consumer-side edits. See [Status](#status).

This repo is therefore two things at once:

1. **A standalone demo** — Teensy + one round eye + one camera = an eye that follows your face.
   Buildable and runnable on its own; a clean, minimal example of driving TeensyEyes gaze from
   a real face sensor.
2. **A hardware/driver module of IRIS** — the surviving, publicly-sourceable gaze-sensor path,
   validated here in isolation before touching the live robot.

---

## Hardware

| Part | Role |
|---|---|
| **Teensy 4.1** (4.0 also works — identical pins) | MCU running TeensyEyes + the sensor driver |
| **GC9A01A 240×240 round LCD** (1.28", SPI) | the "eye" (one for single-eye, two for dual-eye) |
| **DFRobot SEN0626** Gravity AI vision camera (UART) | face detection → gaze target |

The gaze pipeline is portable: the SEN0626 driver and the TeensyEyes engine are plain C++,
so the same approach ports to other MCUs (any board with a hardware UART + SPI and enough RAM
for the eye framebuffer).

Full pin tables, wire colors, power notes, and the dual-eye layout: **[docs/WIRING.md](docs/WIRING.md)**.

### Quick wiring summary (single eye)

| GC9A01A | Teensy | | SEN0626 | Teensy |
|---|---|---|---|---|
| VCC | 3.3V | | VCC | 3.3V |
| GND | GND | | GND | GND |
| SCK | 13 | | TX ("D/T") | 0 (Serial1 RX) |
| MOSI | 11 | | RX ("C/R") | 1 (Serial1 TX) |
| CS | 10 | | | |
| DC | 2 | | | |
| RST | 3 | | | |

> ⚠ **Two gotchas that cost real bench time — check these first:**
> 1. The SEN0626 breakout has a physical **I²C/UART DIP switch. It must be set to UART** —
>    this firmware speaks Modbus RTU over UART only, no I²C fallback.
> 2. Confirm the sensor's **own VCC pin reads ~3.2–3.3 V under load** (not just the Teensy's
>    3.3 V pin). An undervolted SEN0626 randomly resets / freezes — a documented failure mode.

---

## How it works

```
 SEN0626 camera ──Modbus RTU/UART──► SEN0626Sensor (driver/shim) ──► main loop ──► TeensyEyes
 (detects a face,                    presents the Useful Sensors      gaze target    (renders the
  reports center X/Y + score)        person_sensor_face_t struct)      (x, y)          eye looking there)
```

- **`src/sensors/SEN0626Sensor.{h,cpp}`** — the driver. Talks Modbus RTU (FC04 input
  registers, device `0x72`) to the camera, auto-detects baud (115200 → 9600), and exposes the
  **identical `person_sensor_face_t` struct + method surface** as the Useful Sensors Person
  Sensor. That byte-for-byte match is the whole point — it's why IRIS's existing code consumes
  it unchanged. The SEN0626 reports a face *center* (not a box), so the driver stores that
  center in both box edges; consumers recover the exact target. Protocol reference:
  **[docs/SEN0626_PROTOCOL.md](docs/SEN0626_PROTOCOL.md)**.
- **`src/main.cpp`** — the gaze loop: read the sensor, gate on confidence, map the face center
  to a gaze target with per-axis signed gain + bias, and drive the eye. Falls back to idle
  wander when no face is present.
- **`src/eyes/`, `src/displays/`** — Chris Miller's **TeensyEyes** engine (MIT), bundled
  largely verbatim. See [Credits](#credits).

---

## Build & flash

Built with [PlatformIO](https://platformio.org/).

```bash
pio run -e cyclopsgaze              # build
pio run -e cyclopsgaze -t upload    # flash the Teensy
pio device monitor -b 115200        # serial monitor
```

Expected serial on boot:

```
[CG] CyclopsGaze CG-S12
[CG] SEN0626 found at 9600 (attempt 1)
[CG] faces=1 rawScore=72 conf=72 gate=PASS | rawX=311 rawY=40
[CG]   -> tracking x=0.12 y=-0.05
```

First-flash walk-through (enumerate → flash → detect → verify each tracking direction), written
so someone new to PlatformIO can follow it top to bottom: **[docs/BENCH_PROTOCOL.md](docs/BENCH_PROTOCOL.md)**.

### Dual-eye (optional)

Uncomment `#define DUAL_EYE` in `src/config.h` to drive a second GC9A01A. Both eyes share
SPI0 with a separate CS each (the SEN0626 owns pins 0/1, which collides with the Teensy's
SPI1, so IRIS's two-bus layout can't be reused). Pin table in [docs/WIRING.md](docs/WIRING.md).

---

## Tunables (`src/config.h`)

| Constant | Default | What it does |
|---|---|---|
| `PS_CONF_GATE` | `60` | Min DFRobot face score (0–100) to track. 60 = DFRobot's documented validity floor. |
| `GAZE_X_GAIN` / `GAZE_Y_GAIN` | `1.7` / `1.7` | Per-axis gaze range. **Sign = direction** (flip to un-mirror), **magnitude = range**. |
| `GAZE_X_BIAS` / `GAZE_Y_BIAS` | `0.0` / `1.26` | Per-axis center offset. `Y_BIAS` compensates for the camera mounting below the eye. |
| `FACE_LOST_MS` | `3000` | Time with no face before idle wander resumes. |
| `CG_CALIB_RAW` | `1` | Log raw sensor registers for bench calibration; set `0` for quiet serial. |

The `targetN = rawN * gain + bias` shaping model and the confidence-scale choice are ported
from the parent project's proven tuning — see [docs/IRIS_INTEGRATION.md](docs/IRIS_INTEGRATION.md).

---

## Status

**Standalone gaze tracking: bench-VERIFIED** (direction, range, and center all confirmed live
on a Teensy 4.1 + SEN0626 + GC9A01A).

**Integrated into IRIS: yes, and tracking live** — the SEN0626 driver was swapped in for the
dead Person Sensor on IRIS's eyes and is confirmed detecting/tracking faces in the real robot.

> ⚠ **One caveat, and it's the top re-bench priority.** The most recent code change (CG-S12)
> synced this repo's driver + gaze math to the refined version proven in IRIS: the confidence
> gate now uses DFRobot's native 0–100 score scale (gate `60` instead of `152/255`), and gaze
> uses per-axis signed gain + bias. These are algebraically the same as the previously
> bench-VERIFIED behavior and the firmware **compiles clean**, but they have **not been
> re-observed on the standalone bench since the change**. Re-flash + re-run
> [docs/BENCH_PROTOCOL.md](docs/BENCH_PROTOCOL.md) before treating standalone tracking as
> re-VERIFIED. Full history in [CHANGELOG.md](CHANGELOG.md).

Terminology used throughout this repo: **REPO-ONLY** (committed, not flashed) →
**DEPLOYED** (flashed) → **VERIFIED** (flashed *and* behavior observed on the bench).
Nothing is called "done" until VERIFIED.

---

## Relation to IRIS

**IRIS** is a private tabletop robot-face project (LLM assistant + animated eyes + head-pan
servos). CyclopsGaze is IRIS's **gaze-sensor module**, developed in this separate public repo
so the sensor swap could be proven in isolation without touching the live robot.

- CyclopsGaze validates the SEN0626 → gaze pipeline standalone.
- The validated driver (`src/sensors/SEN0626Sensor.{h,cpp}`) and the ready-to-copy adapters in
  **[`integration/`](integration/)** are what actually drop into IRIS's two sensor consumers
  (the eyes Teensy and the head-pan servo Teensy).
- The refinements discovered *during* the real IRIS swap have been synced back here.

Details of the integration, the deliberate driver divergence, and the bugs the swap uncovered:
**[docs/IRIS_INTEGRATION.md](docs/IRIS_INTEGRATION.md)**.

---

## Repository layout

```
CyclopsGaze/
├── src/                    firmware (driver, gaze loop, bundled TeensyEyes engine)
├── integration/            ready-to-copy drop-in adapters for IRIS's two consumers
├── docs/                   wiring, protocol, bench procedure, IRIS integration, eng. log
│   ├── archive/            raw session-by-session development handoffs (historical)
│   └── media/              photos & video
├── CHANGELOG.md            full development history (CG-S1 … CG-S12)
├── CLAUDE.md               working context for Claude Code sessions
├── RULES.md                engineering discipline for this repo
└── platformio.ini
```

---

## Credits

- **TeensyEyes — the eye-rendering engine**, MIT © 2022 **Chris Miller**. CyclopsGaze bundles
  it (`src/eyes/`, `src/displays/`) largely verbatim. TeensyEyes is itself adapted from
  **Adafruit's Uncanny Eyes / M4_Eyes** (also MIT).
- **DFRobot** — SEN0626 Gravity AI vision camera and its `DFRobot_GestureFaceDetection`
  library / Modbus register documentation.
- **Useful Sensors** — the original *Person Sensor* (SEN-21231) whose interface this project
  reimplements.
- Display driver: **GC9A01A_t3n** by KurtE / mjs513.

Full third-party attributions: **[docs/ATTRIBUTION.md](docs/ATTRIBUTION.md)**.

## License

[MIT](LICENSE). Bundled TeensyEyes code remains under its original MIT license and Chris
Miller's copyright.
