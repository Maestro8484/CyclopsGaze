# CyclopsGaze → IRIS drop-in integration kit

Ready-to-copy files that make the DFRobot SEN0626 (on a Teensy over UART/Modbus)
stand in for the discontinued Useful Sensors Person Sensor (SEN-21231) across
**both** IRIS face-tracking consumers. IRIS itself is edited only during a later
deploy session — these files live in CyclopsGaze (read-only reference to IRIS).

Full step-by-step + decisions: [`../docs/IRIS_INTEGRATION.md`](../docs/IRIS_INTEGRATION.md).

## The two consumers (different interfaces, one sensor family)

| | T4.1 eyes (gaze) | T4.0 servo (head pan) |
|---|---|---|
| IRIS driver replaced | `src/sensors/PersonSensor.{h,cpp}` (class) | `servo_teensy40/teensy40_base_mount/person_sensor.{h,cpp}` (free fns) |
| Interface | `PersonSensor` class methods | `setupPersonSensor()` / `pollPersonSensor()` → `PersonResult` |
| Axes used | X + Y (full gaze) | X only (`faceCenterX`, pans left/right) |
| Drop-in provided | `src/sensors/SEN0626Sensor.{h,cpp}` (already the class shim) | `integration/servo_teensy40_base_mount/person_sensor.{h,cpp}` (this kit) |

Both are UART now: SEN0626 on `Serial1` (sensor TX→pin 0, sensor RX→pin 1), not
I2C. Same wiring recipe as CyclopsGaze (`../docs/WIRING.md`).

## Eyes (T4.1)

1. Copy `../src/sensors/SEN0626Sensor.{h,cpp}` into IRIS `src/sensors/`.
2. `main.cpp`: `#include "sensors/SEN0626Sensor.h"` and change the object to
   `SEN0626Sensor personSensor(Serial1);` (keep the name `personSensor` so every
   call site is untouched). Neutralize the I2C bring-up / `psI2cBusRecover()` on
   this path. The existing `personSensor.isPresent()` probe loop works unchanged
   — `isPresent()` lazily runs `begin()` until the sensor answers.
3. **Do NOT copy CyclopsGaze's gaze tuning verbatim** (its `GAZE_*_GAIN`/`GAZE_*_BIAS`
   defaults). Those are single-eye / bench-mount specific. IRIS has two eyes and its own
   mount geometry — bench-verify direction on IRIS and set its own per-axis gain/bias signs
   and offsets. See [`../docs/IRIS_INTEGRATION.md`](../docs/IRIS_INTEGRATION.md).
4. Decide the confidence scale — the one real behavior choice (both now emit the raw
   0–100 score and gate at 60; see IRIS_INTEGRATION.md § divergence).

## Servo (T4.0)

1. Copy `../src/sensors/SEN0626Sensor.{h,cpp}` into
   `servo_teensy40/teensy40_base_mount/`.
2. Replace that folder's `person_sensor.{h,cpp}` with the two files in
   `integration/servo_teensy40_base_mount/`. The `.ino` is unchanged.
3. Wire SEN0626 to the T4.0's `Serial1`. Leave the `.ino`'s `Wire.begin()` (the
   paj7620 gesture sensor still uses I2C); only the Person Sensor moves to UART.
4. Bench-verify pan direction in `updatePanFromFace()` (unchanged, downstream).

## Status

The **eyes** drop-in has been **deployed to live IRIS and is tracking faces** (IRIS S212/b/c);
the real swap uncovered the center-only-box presence-test bug and drove the per-axis gaze-shaping
model — both documented in [`../docs/IRIS_INTEGRATION.md`](../docs/IRIS_INTEGRATION.md). The
**servo** adapter here is code-reviewed against the live interface but flash-verify it on the
servo Teensy before relying on it.
