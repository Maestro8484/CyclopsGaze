# Attribution & Third-Party Code

CyclopsGaze is [MIT licensed](../LICENSE). It bundles and builds on the following third-party
work, each under its own license.

## What's original in this repo vs. bundled

CyclopsGaze is a derivative work of Chris Miller's TeensyEyes. The table below is the
file-level split, verified by diffing against pristine `github.com/chrismiller/TeensyEyes`.

| Component | Origin |
|---|---|
| `src/sensors/SEN0626Sensor.{h,cpp}` | **Original**: the SEN0626 Modbus/UART driver, replacing TeensyEyes' I²C Person Sensor entirely |
| `integration/**` | **Original**: drop-in adapters for the two IRIS consumers |
| `src/main.cpp` | **Original logic**: the gaze/tracking loop (built on the TeensyEyes API) |
| `src/config.h` | **Mostly original**: tunables + this project's wiring/eye setup |
| `nordicBlue` eye **texture** (`nordicBlue.h` iris/sclera data) | **Original art**: generated from the author's own iris/sclera image via TeensyEyes' image-conversion tooling |
| `nordicBlue.h` **eyelid geometry** | TeensyEyes, identical to its stock 240×240 eyelid shape |
| `hazel.h` (CG-S15), and `bigBlue.h`, `cat.h`, `demon.h`, `doe.h`, `doomRed.h`, `dragon.h`, `fish.h`, `skull.h` (CG-S16) | **TeensyEyes, verbatim.** Nine of Chris Miller's eyes in their entirety, artwork included, copied unmodified from upstream as selectable eye sets. Not original to this project in any part. Verified byte-identical to upstream by `diff` at CG-S16. |
| `polarDist_240_*.*`, `disp_240_1{20,25,30}.*`, `polarAngle_240.*`, `noeyelids_120.*` | TeensyEyes tooling, generated maps, verbatim. One `polarDist` variant per iris radius: `_69_0` is for `nordicBlue`, the other eight came in with the eyes that need them (CG-S15, CG-S16). |
| `src/eyes/EyeController.h` | TeensyEyes, **modified** (~305 of ~700 lines changed) |
| `src/eyes/eyes.h`, `src/displays/*` | TeensyEyes, largely / entirely **verbatim**. One deliberate divergence: `GC9A01A_Display::update()` reports the `SHOW_FPS` counter over serial instead of drawing it on the eye (CG-S17b). |

In plain terms: the eye-rendering engine, display driver, eyelid/polar maps, and **nine of the
ten bundled eyes** are Chris Miller's (MIT; much of it copied byte-for-byte). By volume of
source, the artwork in this repo is overwhelmingly his. The SEN0626 face-tracking driver, the
gaze-integration layer, the eye-set selection and `EYE:` protocol, and the `nordicBlue` eye's
texture art are the CyclopsGaze author's original work. The combined work is distributed under
MIT, preserving Chris Miller's copyright and license for his portions.

### Bundled eyes, and the ones deliberately not bundled

Upstream ships 23 eyes for 240x240. Ten are bundled here (`nordicBlue` plus the nine above),
chosen for spread of character rather than completeness. The 13 not bundled are `anime`,
`blueFlame1`, `blueFlame2`, `brown`, `doomSpiral`, `firebox`, `fizzgig`, `flame`, `hypnoRed`,
`leopard`, `newt`, `spikes` and `toonstripe`. Adding one is a copy plus two uncomments, and
costs nothing in flash until enabled: see the header comment in `src/config.h`. Anyone doing
that is copying Chris Miller's artwork and should keep this attribution accurate.

## Bundled in this repo (`src/`)

| Component | Path | Author / License |
|---|---|---|
| **TeensyEyes**, "Uncanny Eyes" eye-rendering engine (EyeController, eye definitions, polar maps) | `src/eyes/` | **Chris Miller**, MIT © 2022. Bundled largely verbatim. |
| **GC9A01A display wrapper** | `src/displays/` | Thin wrapper over the GC9A01A driver, in the TeensyEyes lineage (MIT). |

TeensyEyes is itself adapted from **Adafruit's Uncanny Eyes** and **M4_Eyes** projects (both MIT).
Those upstream copyrights and license terms are preserved.

## Build-time dependencies (fetched by PlatformIO, not vendored)

| Library | Used for | Source |
|---|---|---|
| **GC9A01A_t3n** | driving the 240×240 round LCD on Teensy | KurtE / mjs513 (see `platformio.ini` `lib_deps`) |

## Hardware & interfaces referenced

- **DFRobot SEN0626** Gravity AI vision camera: the `DFRobot_GestureFaceDetection` library and
  the SEN0626 wiki (`wiki.dfrobot.com/sen0626`) were the reference for the Modbus register map.
  CyclopsGaze's driver is an independent reimplementation, not a copy of DFRobot's library.
- **Useful Sensors Person Sensor (SEN-21231)**: the `person_sensor_face_t` struct and method
  surface reproduced here match this (now discontinued) product's public interface so existing
  consumers work unchanged.

If you believe any attribution here is incomplete or incorrect, please open an issue.
