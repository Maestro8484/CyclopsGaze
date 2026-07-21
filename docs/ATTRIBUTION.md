# Attribution & Third-Party Code

CyclopsGaze is [MIT licensed](../LICENSE). It bundles and builds on the following third-party
work, each under its own license.

## What's original in this repo vs. bundled

CyclopsGaze is a derivative work of Chris Miller's TeensyEyes. The table below is the
file-level split, verified by diffing against pristine `github.com/chrismiller/TeensyEyes`.

| Component | Origin |
|---|---|
| `src/sensors/SEN0626Sensor.{h,cpp}` | **Original** — the SEN0626 Modbus/UART driver, replacing TeensyEyes' I²C Person Sensor entirely |
| `integration/**` | **Original** — drop-in adapters for the two IRIS consumers |
| `src/main.cpp` | **Original logic** — the gaze/tracking loop (built on the TeensyEyes API) |
| `src/config.h` | **Mostly original** — tunables + this project's wiring/eye setup |
| `nordicBlue` eye **texture** (`nordicBlue.h` iris/sclera data) | **Original art** — generated from the author's own iris/sclera image via TeensyEyes' image-conversion tooling |
| `nordicBlue.h` **eyelid geometry** | TeensyEyes — identical to its stock 240×240 eyelid shape |
| `polarDist_240_125_69_0.*`, `disp_240_125.*`, `polarAngle_240.*` | TeensyEyes tooling — generated maps (verbatim / for a chosen iris radius) |
| `src/eyes/EyeController.h` | TeensyEyes — **modified** (~305 of ~700 lines changed) |
| `src/eyes/eyes.h`, `src/displays/*` | TeensyEyes — largely / entirely **verbatim** |

In plain terms: the eye-rendering engine, display driver, and eyelid/polar maps are Chris
Miller's (MIT — some copied byte-for-byte). The SEN0626 face-tracking driver, the
gaze-integration layer, and the `nordicBlue` eye's texture art are the CyclopsGaze author's
original work. The combined work is distributed under MIT, preserving Chris Miller's
copyright and license for his portions.

## Bundled in this repo (`src/`)

| Component | Path | Author / License |
|---|---|---|
| **TeensyEyes** — "Uncanny Eyes" eye-rendering engine (EyeController, eye definitions, polar maps) | `src/eyes/` | **Chris Miller**, MIT © 2022. Bundled largely verbatim. |
| **GC9A01A display wrapper** | `src/displays/` | Thin wrapper over the GC9A01A driver, in the TeensyEyes lineage (MIT). |

TeensyEyes is itself adapted from **Adafruit's Uncanny Eyes** and **M4_Eyes** projects (both MIT).
Those upstream copyrights and license terms are preserved.

## Build-time dependencies (fetched by PlatformIO, not vendored)

| Library | Used for | Source |
|---|---|---|
| **GC9A01A_t3n** | driving the 240×240 round LCD on Teensy | KurtE / mjs513 (see `platformio.ini` `lib_deps`) |

## Hardware & interfaces referenced

- **DFRobot SEN0626** Gravity AI vision camera — the `DFRobot_GestureFaceDetection` library and
  the SEN0626 wiki (`wiki.dfrobot.com/sen0626`) were the reference for the Modbus register map.
  CyclopsGaze's driver is an independent reimplementation, not a copy of DFRobot's library.
- **Useful Sensors Person Sensor (SEN-21231)** — the `person_sensor_face_t` struct and method
  surface reproduced here match this (now discontinued) product's public interface so existing
  consumers work unchanged.

If you believe any attribution here is incomplete or incorrect, please open an issue.
