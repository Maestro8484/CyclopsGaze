# CyclopsGaze → IRIS drop-in integration kit

Ready-to-copy files that make the DFRobot SEN0626 (on a Teensy over UART/Modbus)
stand in for the discontinued Useful Sensors Person Sensor (SEN-21231) across
**both** IRIS face-tracking consumers. IRIS itself is edited only during a later
deploy session — these files live in CyclopsGaze (read-only reference to IRIS).

Full step-by-step + decisions: [`../09_IRIS_INTEGRATION_PLAN.md`](../09_IRIS_INTEGRATION_PLAN.md).

## The two consumers (different interfaces, one sensor family)

| | T4.1 eyes (gaze) | T4.0 servo (head pan) |
|---|---|---|
| IRIS driver replaced | `src/sensors/PersonSensor.{h,cpp}` (class) | `servo_teensy40/teensy40_base_mount/person_sensor.{h,cpp}` (free fns) |
| Interface | `PersonSensor` class methods | `setupPersonSensor()` / `pollPersonSensor()` → `PersonResult` |
| Axes used | X + Y (full gaze) | X only (`faceCenterX`, pans left/right) |
| Drop-in provided | `src/sensors/SEN0626Sensor.{h,cpp}` (already the class shim) | `integration/servo_teensy40_base_mount/person_sensor.{h,cpp}` (this kit) |

Both are UART now: SEN0626 on `Serial1` (sensor TX→pin 0, sensor RX→pin 1), not
I2C. Same wiring recipe as CyclopsGaze (`../05_WIRING.md`).

## Eyes (T4.1)

1. Copy `../src/sensors/SEN0626Sensor.{h,cpp}` into IRIS `src/sensors/`.
2. `main.cpp`: `#include "sensors/SEN0626Sensor.h"` and change the object to
   `SEN0626Sensor personSensor(Serial1);` (keep the name `personSensor` so every
   call site is untouched). Neutralize the I2C bring-up / `psI2cBusRecover()` on
   this path. The existing `personSensor.isPresent()` probe loop works unchanged
   — `isPresent()` lazily runs `begin()` until the sensor answers.
3. **Do NOT copy CyclopsGaze's CG-S6 targetX change or its `Y_CENTER`.** Those
   are single-eye / bench-mount specific. IRIS keeps its own `targetX` negation
   and its runtime `psYBias`. Bench-verify direction on IRIS independently;
   trim `psYBias` for the sensor's mount height. See plan §4 / §5.
4. Decide the confidence scale (plan §5) — this is the one real behavior choice.

## Servo (T4.0)

1. Copy `../src/sensors/SEN0626Sensor.{h,cpp}` into
   `servo_teensy40/teensy40_base_mount/`.
2. Replace that folder's `person_sensor.{h,cpp}` with the two files in
   `integration/servo_teensy40_base_mount/`. The `.ino` is unchanged.
3. Wire SEN0626 to the T4.0's `Serial1`. Leave the `.ino`'s `Wire.begin()` (the
   paj7620 gesture sensor still uses I2C); only the Person Sensor moves to UART.
4. Bench-verify pan direction in `updatePanFromFace()` (unchanged, downstream).

## Status

CyclopsGaze tracking is **VERIFIED** (CG-S8). These adapters are code-reviewed
against the live IRIS interfaces but are **REPO-ONLY** for IRIS until an IRIS
deploy session flashes and bench-verifies them. See plan §6.
