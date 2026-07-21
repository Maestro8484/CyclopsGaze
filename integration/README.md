# CyclopsGaze → IRIS drop-in integration kit

Ready-to-copy files that make the DFRobot SEN0626 (on a Teensy over UART/Modbus) stand in for
the discontinued Useful Sensors Person Sensor (SEN-21231) across both IRIS face-tracking
consumers. IRIS itself is edited only during a later deploy session, these files live in
CyclopsGaze (read-only reference to IRIS).

Full step-by-step + decisions: [`../docs/IRIS_INTEGRATION.md`](../docs/IRIS_INTEGRATION.md).

## The two consumers (different interfaces, one sensor family)

| | T4.1 eyes (gaze) | T4.0 servo (head pan) |
|---|---|---|
| IRIS driver replaced | `src/sensors/PersonSensor.{h,cpp}` (class) | `servo_teensy40/teensy40_base_mount/person_sensor.{h,cpp}` (free fns) |
| Interface | `PersonSensor` class methods | `setupPersonSensor()` / `pollPersonSensor()` → `PersonResult` |
| Axes used | X + Y (full gaze) | X only (`faceCenterX`, pans left/right) |
| Drop-in provided | `src/sensors/SEN0626Sensor.{h,cpp}` (already the class shim) | `integration/servo_teensy40_base_mount/person_sensor.{h,cpp}` (this kit) |

Both are UART now: SEN0626 over Modbus RTU, not I2C. The port is per-board, not universal.
CyclopsGaze's bench rig uses `Serial1` (RX 0 / TX 1), but the live IRIS eyes node uses
`Serial4` (RX 16 / TX 17) because pin 0 is its left-eye CS. Since CG-S13 it's a one-line
knob: `#define SEN0626_SERIAL …` in `../src/config.h`. Sensor D/T → the RX pin, sensor C/R →
the TX pin. See `../docs/WIRING.md` and `../docs/IRIS_INTEGRATION.md`.

## Eyes (T4.1)

1. Copy `../src/sensors/SEN0626Sensor.{h,cpp}` into IRIS `src/sensors/`.
2. `main.cpp`: `#include "sensors/SEN0626Sensor.h"` and change the object to
   `SEN0626Sensor personSensor(<a free UART>);` (keep the name `personSensor` so every call
   site is untouched). Neutralize the I2C bring-up / `psI2cBusRecover()` on this path. The
   existing `personSensor.isPresent()` probe loop works unchanged. `isPresent()` lazily runs
   `begin()` until the sensor answers.
3. Don't copy CyclopsGaze's gaze tuning verbatim (its `GAZE_*_DEFAULT` values). Those are
   single-eye / bench-mount specific. IRIS has two eyes and its own mount geometry. Bench-
   verify direction on IRIS and set its own per-axis gain/bias signs and offsets. See
   [`../docs/IRIS_INTEGRATION.md`](../docs/IRIS_INTEGRATION.md). (This already happened: the
   eyes port is live in IRIS. The two installs' tuning values now differ and have never been
   compared head-to-head. An open item in IRIS_INTEGRATION.md § "The tuning-value gap".)
4. Decide the confidence scale, the one real behavior choice (both now emit the raw 0–100
   score and gate at 60; see IRIS_INTEGRATION.md § divergence).

## Servo (T4.0)

1. Copy `../src/sensors/SEN0626Sensor.{h,cpp}` into `servo_teensy40/teensy40_base_mount/`.
2. Replace that folder's `person_sensor.{h,cpp}` with the two files in
   `integration/servo_teensy40_base_mount/`. The `.ino` is unchanged.
3. Wire SEN0626 to the T4.0's `Serial1`. Leave the `.ino`'s `Wire.begin()` (the paj7620
   gesture sensor still uses I2C); only the Person Sensor moves to UART.
4. Bench-verify pan direction in `updatePanFromFace()` (unchanged, downstream).

## Status

*(Checked against the IRIS repo + the live Pi4 on 2026-07-21.)*

The eyes drop-in has been deployed to live IRIS and is tracking faces (IRIS S212/b/c, now
running firmware S213 on `Serial4`); the real swap uncovered the center-only-box
presence-test bug and drove the per-axis gaze-shaping model, both documented in
[`../docs/IRIS_INTEGRATION.md`](../docs/IRIS_INTEGRATION.md). `SEN0626Sensor.{h,cpp}` are
byte-identical across the two repos; IRIS's gaze code hasn't changed since S212c.

The servo adapter here is still unapplied. IRIS's T4.0 servo node runs the original I2C
Person Sensor driver to this day. It's code-reviewed against the live interface, but
flash-verify it on the servo Teensy before relying on it.
