# CyclopsGaze

**A face-tracking gaze demo on a single round LCD "eye," and a drop-in replacement for the discontinued Useful Sensors Person Sensor.**

CyclopsGaze runs Chris Miller's [TeensyEyes](#credits) "uncanny eyes" on a 1.28"/240×240
GC9A01A round display and makes the eye look at your face, driven by a DFRobot **SEN0626**
AI vision camera over UART. Its real job: prove the SEN0626 can stand in, byte-for-byte, for
the now-discontinued Useful Sensors *Person Sensor* (SEN-21231) that drives the gaze "soul"
of the [IRIS](#relation-to-iris) robot face.

<!-- Hero shot: uncomment the line below once docs/media/cyclopsgaze_tracking.jpg exists.
     Left commented so the public README does not render a broken image.
     Shot list and file conventions: docs/media/README.md
![CyclopsGaze tracking a face](docs/media/cyclopsgaze_tracking.jpg)
-->

---

## Why this exists

TeensyEyes renders great animated eyes, but on its own the eye just wanders. To give it a
"soul" (to make it watch you) you need a face-detection sensor feeding a position into the
gaze target. In the parent [IRIS](#relation-to-iris) project that sensor was the Useful
Sensors **Person Sensor (SEN-21231)**: a tiny I²C module returning face bounding boxes. It
worked well, then got discontinued. That's a problem for a project meant to be publicly
replicable, since buyers can't source a dead part.

CyclopsGaze is the standalone testbed built to answer one question: can the DFRobot SEN0626
(Modbus RTU over UART) be a true drop-in for the Person Sensor: same struct, same API, same
gaze behavior, so the eyes keep their soul?

Answer: yes. The SEN0626 driver here presents the exact `person_sensor_face_t` struct and
method surface IRIS already consumes, so the parent project swapped it in with almost no
consumer-side edits. See [Status](#status).

So this repo is two things at once:

1. **A standalone animatronic eye**: one board + one round display + one camera = an eye that
   follows your face. Buildable and runnable entirely on its own, with no connection to IRIS.
2. **A hardware/driver module of IRIS**: the publicly-sourceable gaze-sensor path, validated
   here in isolation before touching the live robot.

If you came here for the first one, you do not need to care about the rest. A face-tracking
eye is useful well outside this project: Halloween props that follow trick-or-treaters,
cosplay helmets and masks, puppets and creature builds, art pieces and desk toys. The
difference from a typical "uncanny eyes" build is that this one **tracks a real face** rather
than wandering on a timer, and the face detection runs on the camera itself rather than
needing a Pi or a PC. Around **$47 in parts** for a single tracking eye, or **$32** if you
skip the camera and let it wander. Full parts list, sources and current prices:
**[docs/BOM.md](docs/BOM.md)**.

---

## Hardware

| Part | Role |
|---|---|
| **Teensy 4.1** (4.0 also works, identical pins) | MCU running TeensyEyes + the sensor driver |
| **GC9A01A 240×240 round LCD** (1.28", SPI) | the "eye" (one for single-eye, two for dual-eye) |
| **DFRobot SEN0626** Gravity AI vision camera (UART) | face detection → gaze target |

The gaze pipeline is portable: the SEN0626 driver and TeensyEyes engine are plain C++, so
the same approach ports to any board with a hardware UART + SPI and enough RAM for the eye
framebuffer. A cheaper ESP32-S3 build is being evaluated and is **not yet proven**, see
[docs/ESP32_S3_MIGRATION.md](docs/ESP32_S3_MIGRATION.md).

Part numbers, vendors, current prices and build totals: **[docs/BOM.md](docs/BOM.md)**.
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

Two things that'll eat your bench time if you skip them:
1. The SEN0626 breakout has a physical I²C/UART DIP switch. Set it to UART. This firmware
   is Modbus RTU over UART only, no I²C fallback.
2. Check the sensor's *own* VCC pin reads ~3.2–3.3V under load, not just the Teensy's 3.3V
   pin. An undervolted SEN0626 resets/freezes randomly, a known failure mode.

---

## Build paths

Four routes to a working eye, in descending order of how proven they are. Pick by what you
want and how much risk you are willing to carry. Costs are domestic parts only, verified
2026-07-21, itemised in [docs/BOM.md](docs/BOM.md).

### Path A: one eye that tracks a face. **VERIFIED, start here.**

The only configuration confirmed working on real hardware (CG-S8: direction, range and
centering all observed on a bench). Teensy 4.1 or 4.0, one GC9A01A, one SEN0626. **~$64.**

1. Wire it: [docs/WIRING.md](docs/WIRING.md), single-eye table.
2. Check the two gotchas first: SEN0626 DIP switch on **UART**, and the sensor's own VCC pin
   reading 3.2 to 3.3V under load. These are the only two hardware faults that have ever cost
   this project a bench session.
3. Build and flash: `pio run -e cyclopsgaze -t upload`.
4. Walk [docs/BENCH_PROTOCOL.md](docs/BENCH_PROTOCOL.md) top to bottom, steps 0 through 9.
5. Tune live over serial with `PS_CFG:` (no reflash), then write keepers back into
   `src/config.h`.

⚠ One caveat specific to right now: two rounds of code change since that bench pass (CG-S12,
CG-S13) have not been re-observed on hardware. They compile clean and are structurally
equivalent, but re-run the protocol before treating this as re-VERIFIED. See [Status](#status).

### Path B: an eye with no camera, wandering on its own. **Cheapest, least proven.**

Skip the SEN0626 entirely. The eye blinks and saccades autonomously with no face input.
Suitable for a prop that needs to look alive rather than look *at* you. **~$50** on Teensy,
**~$32** on ESP32-S3 once Path D lands. Same wiring as Path A minus the four sensor wires, and
nothing to configure. Not separately bench-confirmed as its own configuration, though idle
wander is exercised by BENCH_PROTOCOL step 8.

### Path C: two eyes. **Compiles, never run on hardware.**

Uncomment `#define DUAL_EYE` in `src/config.h`, wire the second display per the dual-eye table
in [docs/WIRING.md](docs/WIRING.md) (CS 9 / DC 8 / RST 6, sharing SCK 13 and MOSI 11). Both
eyes track the same face off the one camera. **~$81** on Teensy 4.1.

Both eyes share SPI0 because the SEN0626 owns pins 0 and 1, which collides with the Teensy's
SPI1. Per-eye refresh is roughly half single-eye as a result, which is expected rather than a
fault. **BENCH_PROTOCOL step 10 has never been executed**, so this path is unproven end to
end on any board.

### Path D: the cheap ESP32-S3 build. **Nothing built yet. Contributions welcome.**

Replaces the single-sourced $31.50 Teensy with a $15.00 commodity board, taking a two-eye
build to **~$65** domestic or **~$47** with generic displays. The engineering case, the
portability audit, the three known code blockers and the measured render cost are all written
up in [docs/ESP32_S3_MIGRATION.md](docs/ESP32_S3_MIGRATION.md).

It is a real port, not a recompile: the bundled `GC9A01A_t3n` display driver is Teensy-only
and has to be replaced (the `Display.h` abstraction already exists to make that a contained
change). Staged plan is in § 8 of that document, and stage 0 is a frame-rate measurement that
gates everything after it.

### Which to pick

| You want | Path | Status |
|---|---|---|
| It to work this weekend | **A** | VERIFIED |
| A prop that looks alive, cheapest | **B** | untested as a standalone config |
| A face, not a cyclops | **C** | compiles, never run |
| To cut cost and don't mind being first | **D** | not built |

---

## Mounting & enclosures

Physical mount designs (3D-printable and laser-cuttable) live in **[`hardware/`](hardware/)**:

- **[`hardware/stl/`](hardware/stl/)**: 3D-printable parts (mounts, brackets, enclosures).
- **[`hardware/lasercut/`](hardware/lasercut/)**: laser-cuttable 2D panels (SVG/DXF).

Scaffold only right now, both folders are empty aside from their own README placeholders.
No dimensions for the GC9A01A display, the SEN0626 Gravity board, or mounting-hole positions
have been measured yet. There's also an open design fork: the SEN0626's camera is physically
separable from the rest of the Gravity board (bench finding, [docs/ENGINEERING_LOG.md](docs/ENGINEERING_LOG.md)
"CG-S10"), which could allow a much smaller mount, but that finding itself is unverified
(register behavior post-separation hasn't been confirmed). See
[`hardware/README.md`](hardware/README.md) for what a first design needs to nail down, and
the same REPO-ONLY → DEPLOYED → VERIFIED discipline the firmware uses.

---

## How it works

```
 SEN0626 camera ──Modbus RTU/UART──► SEN0626Sensor (driver/shim) ──► main loop ──► TeensyEyes
 (detects a face,                    presents the Useful Sensors      gaze target    (renders the
  reports center X/Y + score)        person_sensor_face_t struct)      (x, y)          eye looking there)
```

- **`src/sensors/SEN0626Sensor.{h,cpp}`**: the driver. Talks Modbus RTU (FC04 input
  registers, device `0x72`) to the camera, auto-detects baud (115200 → 9600), and exposes the
  identical `person_sensor_face_t` struct + method surface as the Useful Sensors Person
  Sensor. That byte-for-byte match is the whole point: it's why IRIS's existing code
  consumes it unchanged. The SEN0626 reports a face *center* (not a box), so the driver
  stores that center in both box edges; consumers recover the exact target. Protocol
  reference: **[docs/SEN0626_PROTOCOL.md](docs/SEN0626_PROTOCOL.md)**.
- **`src/main.cpp`**: the gaze loop. Read the sensor, gate on confidence, map the face
  center to a gaze target with per-axis signed gain + bias, drive the eye. Falls back to
  idle wander when no face is present.
- **`src/eyes/`, `src/displays/`**: Chris Miller's TeensyEyes engine (MIT), bundled largely
  verbatim. See [Credits](#credits).

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
[CG] CyclopsGaze CG-S15
[CG] SEN0626 found at 9600 (attempt 1)
[CG] EYE sets=2 start=nordicBlue rotate=on every 20000ms
[CG] faces=1 rawScore=72 conf=72 gate=PASS | rawX=311 rawY=40
[CG]   -> raw=0.07,-0.03  target=0.12,-0.05  (gain 1.70/1.70 bias 0.00/1.26)
[CG] EYE set=1 name=hazel (auto)
```

First-flash walkthrough (enumerate → flash → detect → verify each tracking direction),
written for someone new to PlatformIO: **[docs/BENCH_PROTOCOL.md](docs/BENCH_PROTOCOL.md)**.

### Dual-eye (optional)

Uncomment `#define DUAL_EYE` in `src/config.h` to drive a second GC9A01A. Both eyes share
SPI0 with a separate CS each. The SEN0626 owns pins 0/1, which collides with the Teensy's
SPI1, so IRIS's two-bus layout can't be reused. Pin table in [docs/WIRING.md](docs/WIRING.md).

---

## Tunables: live over serial, no reflash (`PS_CFG:`)

Since CG-S13, tracking behavior tunes live over the same USB serial link you're already
watching, ported from IRIS's own runtime-tuning protocol. Type a command, watch the eye
change on the next sample:

| Command | What it does | Default |
|---|---|---|
| `PS_CFG:CONF=n` | Min DFRobot face score (0–100) to track. `60` = DFRobot's documented validity floor. | `60` |
| `PS_CFG:X_GAIN=f` / `PS_CFG:Y_GAIN=f` | Per-axis gaze range. Sign = direction (negate to un-mirror), magnitude = range. | `1.7` / `1.7` |
| `PS_CFG:X_BIAS=f` / `PS_CFG:Y_BIAS=f` | Per-axis center offset. `Y_BIAS` compensates for the camera mounting below the eye. | `0.0` / `1.26` |
| `PS_CFG:LOST_MS=n` | Time with no face before idle wander resumes. | `3000` |
| `PS_CFG:FACING=0/1` | Require the `is_facing` bit (currently inert, SEN0626 has no facing register). | `0` |
| `PS_CFG?` | Print all live values on one line. | n/a |

Values are RAM-only and reset with the board. Once a value proves out on the bench, write it
back into the matching `*_DEFAULT` constant in `src/config.h` so it survives a power cycle.
Full reference: [docs/BENCH_PROTOCOL.md](docs/BENCH_PROTOCOL.md) § Live tuning.

### Swapping the eye's look (`EYE:`)

Since CG-S15 the firmware carries two eye sets, **`nordicBlue`** and **`hazel`**, and rotates
between them every 20 seconds by default. The look is independent of gaze: switching it does
not disturb tracking, and rotation runs whether or not a face is present.

| Command | What it does |
|---|---|
| `EYE?` | List the available sets, the current one, and the rotation state |
| `EYE:next` | Advance to the next set now |
| `EYE:nordicBlue` / `EYE:hazel` | Select by name (case-insensitive). Also turns rotation **off**, so your pick isn't overwritten seconds later |
| `EYE:AUTO=0` / `EYE:AUTO=1` | Stop / start auto-rotation |
| `EYE:MS=n` | Rotation interval in ms (floored at 1000; each swap forces a full repaint) |

Defaults live in `src/config.h` (`EYE_ROTATE_MS_DEFAULT`, `EYE_AUTO_ROTATE_DEFAULT`).

`EYE:` is deliberately a separate command namespace from `PS_CFG:`. The latter is IRIS-verbatim
down to its ack wording so the two projects' `main.cpp` files stay diffable, and adding eye keys
to it would break that for a feature IRIS does not have.

**Adding more eye sets** is a config change, not a code change. TeensyEyes ships around 30 eyes
(`anime`, `cat`, `demon`, `dragon`, `doomRed`, `fizzgig`, `leopard`, `skull`, `snake`, ...).
Copy the eye's header plus whichever `polarDist_240_<radius>_<irisRadius>_0` table it includes
into `src/eyes/240x240/`, add the `#include`, and add a row to `eyeDefinitions` in
`src/config.h`. Every set must share `polar.mapRadius` (240 here), because the gaze position is
stored in mapRadius units and is not rescaled across a swap. Each additional eye costs roughly
280 KB of flash, against ~7.3 MB free on a Teensy 4.1.

The `targetN = rawN * gain + bias` shaping model, the confidence-scale choice, and the
`PS_CFG:` protocol itself are all ported from the parent project's proven tuning, see
[docs/IRIS_INTEGRATION.md](docs/IRIS_INTEGRATION.md). One other `config.h` knob worth
knowing: `SEN0626_SERIAL` selects which hardware UART the sensor is wired to (default
`Serial1`). IRIS's own install uses a different port because its pin 0 is already spoken
for by an eye's chip-select.

---

## Status

**Standalone gaze tracking: bench-VERIFIED** (direction, range, and center all confirmed
live on a Teensy 4.1 + SEN0626 + GC9A01A).

**Integrated into IRIS: yes, and tracking live.** The SEN0626 driver was swapped in for the
dead Person Sensor on IRIS's eyes and is confirmed detecting/tracking faces in the real robot.

One open item, and it's the top re-bench priority: two rounds of code change since the last
bench pass haven't been re-observed on hardware. CG-S12 synced this repo's driver + gaze math
to the version proven in IRIS (confidence gate on DFRobot's native 0–100 score scale, gate
`60` instead of `152/255`; gaze on per-axis signed gain + bias), and CG-S13 added the live
`PS_CFG:` tuning protocol + a facing gate on top of that. Both are algebraically/structurally
equivalent to the previously bench-VERIFIED behavior and the firmware compiles clean, but
neither's been reflashed and watched since. Re-flash and re-run
[docs/BENCH_PROTOCOL.md](docs/BENCH_PROTOCOL.md) before treating standalone tracking as
re-VERIFIED. Full history in [CHANGELOG.md](CHANGELOG.md).

Terminology used throughout this repo: **REPO-ONLY** (committed, not flashed) →
**DEPLOYED** (flashed) → **VERIFIED** (flashed *and* behavior observed on the bench). Nothing
is called "done" until VERIFIED.

---

## Relation to IRIS

**IRIS** is a private tabletop robot-face project (LLM assistant + animated eyes + head-pan
servos). CyclopsGaze is IRIS's gaze-sensor module, developed in this separate public repo so
the sensor swap could be proven in isolation without touching the live robot.

- CyclopsGaze validates the SEN0626 → gaze pipeline standalone.
- The validated driver (`src/sensors/SEN0626Sensor.{h,cpp}`) and the ready-to-copy adapters
  in **[`integration/`](integration/)** are what actually drop into IRIS's two sensor
  consumers (the eyes Teensy and the head-pan servo Teensy).
- Refinements discovered during the real IRIS swap have been synced back here.

Details of the integration, the deliberate driver divergence, and the bugs the swap uncovered:
**[docs/IRIS_INTEGRATION.md](docs/IRIS_INTEGRATION.md)**.

---

## Repository layout

```
CyclopsGaze/
├── src/                    firmware (driver, gaze loop, bundled TeensyEyes engine)
├── integration/            ready-to-copy drop-in adapters for IRIS's two consumers
├── hardware/                mounts & enclosures (STL + laser-cut), see below
│   ├── stl/                3D-printable parts
│   └── lasercut/           laser-cuttable panels (SVG/DXF)
├── docs/                   BOM, wiring, protocol, bench procedure, IRIS integration, eng. log
│   ├── archive/            raw session-by-session development handoffs (historical)
│   └── media/              photos & video
├── CHANGELOG.md            full development history (CG-S1 … CG-S15)
├── CLAUDE.md               working context for Claude Code sessions
├── RULES.md                engineering discipline for this repo
└── platformio.ini
```

---

## Authorship & originality

CyclopsGaze is a derivative work built on Chris Miller's MIT-licensed TeensyEyes, with an
original sensor + gaze layer on top. To be precise about who authored what:

**Original to this project** (© the CyclopsGaze author):
- `src/sensors/SEN0626Sensor.{h,cpp}`: the SEN0626 Modbus/UART driver, a clean
  reimplementation that replaces TeensyEyes' I²C Person Sensor entirely, matching the Person
  Sensor struct so consumers are unchanged.
- The `integration/` drop-in adapters, and the gaze/tracking logic in `src/main.cpp` +
  `src/config.h`.
- The **nordicBlue** eye, generated from the author's own iris/sclera artwork via
  TeensyEyes' image-conversion tooling.
- The eye-set rotation and the `EYE:` serial protocol.

**From TeensyEyes** (MIT © 2022 Chris Miller), bundled here:
- The eye-rendering engine (`EyeController`), the GC9A01A display driver, and the
  polar/displacement maps (`disp_*`, `polarAngle_*`), several files verbatim,
  `EyeController` modified.
- The eyelid geometry and the image-conversion tooling used to generate the eye above.
- The **hazel** eye in its entirety, artwork included, copied verbatim from upstream at CG-S15
  as a second selectable eye set.

File-level breakdown: **[docs/ATTRIBUTION.md](docs/ATTRIBUTION.md)**.

## Credits

- **TeensyEyes, the eye-rendering engine**, MIT © 2022 **Chris Miller**. CyclopsGaze bundles
  it (`src/eyes/`, `src/displays/`) largely verbatim. TeensyEyes is itself adapted from
  **Adafruit's Uncanny Eyes / M4_Eyes** (also MIT).
- **DFRobot**: SEN0626 Gravity AI vision camera and its `DFRobot_GestureFaceDetection`
  library / Modbus register documentation.
- **Useful Sensors**: the original *Person Sensor* (SEN-21231) whose interface this project
  reimplements.
- Display driver: **GC9A01A_t3n** by KurtE / mjs513.

Full third-party attributions: **[docs/ATTRIBUTION.md](docs/ATTRIBUTION.md)**.

## License

[MIT](LICENSE). Bundled TeensyEyes code remains under its original MIT license and Chris
Miller's copyright.
