# CLAUDE.md — CyclopsGaze session anchor

Working context for Claude Code (or any AI agent) sessions on this repo. Read this first.

## What this project is

CyclopsGaze is a **face-tracking gaze demo** (TeensyEyes on a GC9A01A round display, driven by
a DFRobot SEN0626 AI camera) **and** the validated **drop-in replacement** for the discontinued
Useful Sensors Person Sensor (SEN-21231) used by the parent **IRIS** robot-face project. The
whole point of the `SEN0626Sensor` driver is that it presents the *exact* `person_sensor_face_t`
struct and method surface IRIS already consumes. See [README.md](README.md) for the full framing.

- **Local repo is the source of truth.** GitHub (`Maestro8484/CyclopsGaze`) is the public mirror.
- Parent project **IRIS-Robot-Face is private and READ-ONLY** from here — never write to it.

## Ground-truth discipline (non-negotiable)

1. **Read the live file before you edit it.** Never write from memory. Never guess file contents.
2. **Prior art before invention.** Use FastLED/TeensyEyes/DFRobot-provided mechanisms and the
   in-repo patterns before rolling your own. Justify any custom mechanism out loud.
3. **Claims of correctness require observation.** "works / fixed / verified" must come from
   *observing* it (built it, flashed it, watched the serial/bench) — never from reasoning about
   the code. If unobserved, say "changed, NOT yet verified" and name the check that would confirm.

## Status terminology (use these words, not "done")

- **REPO-ONLY** — written/committed locally, not flashed.
- **DEPLOYED** — flashed to the Teensy.
- **VERIFIED** — flashed *and* behavior observed on the bench.

## Firmware discipline

- Bump `FIRMWARE_VERSION` in `src/config.h` before every flash.
- After flashing, confirm the version string appears in the serial monitor before claiming DEPLOYED.
- The SEN0626 has a physical **I²C/UART DIP switch — it must be on UART**; this firmware is
  UART/Modbus only. Confirm the sensor's own VCC reads ~3.2–3.3 V under load. (See docs/WIRING.md.)
- Which UART is a config knob: `SEN0626_SERIAL` in `src/config.h` (default `Serial1` = RX 0 / TX 1).
  Live IRIS uses `Serial4` (RX 16 / TX 17) — pin 0 is its left-eye CS. Don't assume they match.
- Since CG-S13 the gate/gain/bias/timeout tunables are **runtime** values settable over serial with
  `PS_CFG:` (no reflash) — see docs/BENCH_PROTOCOL.md § Live tuning. `config.h` holds the `*_DEFAULT`
  seeds; RAM-only, so a proven value must be written back to `config.h` or it dies with the power.

## Build & flash commands

```bash
pio run -e cyclopsgaze              # build
pio run -e cyclopsgaze -t upload    # flash
pio device monitor -b 115200        # serial monitor
```

`pio` lives at `~/.platformio/penv/Scripts/pio` if not on PATH. Target board: `teensy41`
(pins identical on 4.0). A clean build emits one benign C++17 `std::pair` ABI *note* from the
IRIS-verbatim EyeController — informational, not a warning.

## Repo map

- `src/` — firmware. `main.cpp` (gaze loop), `config.h` (tunables + eye/display wiring),
  `sensors/SEN0626Sensor.{h,cpp}` (the driver/shim), `eyes/` + `displays/` (bundled TeensyEyes).
- `integration/` — ready-to-copy drop-in adapters for IRIS's two sensor consumers.
- `docs/` — WIRING, SEN0626_PROTOCOL, BENCH_PROTOCOL, IRIS_INTEGRATION, ENGINEERING_LOG,
  ATTRIBUTION; `docs/archive/` holds raw historical session handoffs; `docs/media/` holds photos.
- `CHANGELOG.md` — full history CG-S1 … CG-S13. `RULES.md` — engineering rules.

## Current state (read CHANGELOG.md + docs/ENGINEERING_LOG.md for detail)

- Standalone tracking was **bench-VERIFIED** at CG-S8 (direction, range, Y-center).
- The driver was **integrated into IRIS (S212) and is tracking live** (IRIS firmware S213 as of
  2026-07-21). Refinements were synced back here at **CG-S12** and again at **CG-S13**.
- **The two repos are in sync as of CG-S13** — verified by diff, not memory: `SEN0626Sensor.{h,cpp}`
  are byte-identical (comments aside), and IRIS's gaze code is frozen at S212c. If a future session
  is told "sync from IRIS", **diff first** — the answer may again be "already synced".
- ⚠ **UNVERIFIED on the standalone bench:** CG-S12's raw-score gate + per-axis gain/bias, *and*
  CG-S13's PS_CFG parser + facing gate. All compile clean; none re-observed. **Re-running
  docs/BENCH_PROTOCOL.md is the #1 priority** on the next flash with a spare SEN0626 + T4.1.
- ⚠ **Owed:** a head-to-head behavioral comparison of CyclopsGaze's tuning values vs live IRIS's
  (docs/IRIS_INTEGRATION.md § "The tuning-value gap"). IRIS's are untuned fallbacks, not proven
  numbers — do not adopt them on the assumption that "live = better".

## Session close checklist

- [ ] Clean build confirmed (`pio run`).
- [ ] `FIRMWARE_VERSION` bumped if code changed.
- [ ] CHANGELOG.md / docs updated with *actual observed* findings (not intentions).
- [ ] Firmware status stated in the correct term (REPO-ONLY / DEPLOYED / VERIFIED).
- [ ] Commit created (one logical change per commit).
