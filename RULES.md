# CyclopsGaze: Engineering Rules

Short, enforceable rules for anyone working in this repo. The longer "why" lives in
[CLAUDE.md](CLAUDE.md); this is the checklist form.

## Source of truth
- The **local repo** is authoritative; GitHub is the mirror.
- The parent **IRIS-Robot-Face repo is private and read-only** from here. Never write to it.
- Read the live file before every edit. Never write from memory; never guess file contents.

## Status terminology
- **REPO-ONLY**: committed, not flashed.
- **DEPLOYED**: flashed to the Teensy.
- **VERIFIED**: flashed *and* behavior observed on the bench.
- Nothing is "done" until VERIFIED. Say "changed, NOT yet verified" when you haven't observed it.

## Firmware discipline
- Bump `FIRMWARE_VERSION` in `src/config.h` before every flash.
- After flashing, confirm the version string prints on serial before claiming DEPLOYED.
- Never claim a tracking behavior works without watching it on the bench.

## Hardware first-checks (before blaming firmware)
- SEN0626 DIP switch on **UART** (not I²C). This firmware has no I²C path.
- Sensor's own **VCC ≈ 3.2–3.3 V under load**. Undervolting causes resets/freezes.
- Faces at least **~20 in (0.5 m)** away. Closer is out of the sensor's documented range.

## Code discipline
- Prior art before invention: prefer TeensyEyes / DFRobot / in-repo patterns over custom builds.
- One logical change per commit.
- Keep the `SEN0626Sensor` struct + method surface **byte-for-byte compatible** with the Useful
  Sensors `person_sensor_face_t`. That compatibility is the reason the project exists.
- Calibration/bench-only extras (the `raw*` accessors, `CG_CALIB_RAW`) must stay additive so the
  drop-in contract is never broken.

## Documentation discipline
- Update CHANGELOG.md / docs with **actual observed findings**, not intentions, before closing.
- Record the confirmed baud rate, packet format, and any wiring findings in docs, not memory.

## Scope
- This is a standalone testbed + reusable driver module. Do not add unrelated IRIS features
  (mouth, sleep, serial bridges) here.
