# Attribution & Third-Party Code

CyclopsGaze is [MIT licensed](../LICENSE). It bundles and builds on the following third-party
work, each under its own license.

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
