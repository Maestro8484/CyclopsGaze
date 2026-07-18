# IRIS Integration

How the CyclopsGaze SEN0626 driver drops into the parent **IRIS** robot-face project, what the
real swap uncovered, and the one deliberate way the IRIS copy diverges from this repo.

> IRIS is a private project; this doc covers only what's needed to understand and reproduce the
> drop-in. The ready-to-copy adapter files live in [`../integration/`](../integration/).

## The goal

IRIS drove its animated eyes' gaze (and a head-pan servo) from the Useful Sensors **Person
Sensor (SEN-21231)**. Useful Sensors discontinued it. CyclopsGaze exists to prove the DFRobot
**SEN0626** is a true drop-in so IRIS keeps working and stays publicly replicable. "Drop-in" here
is literal: `SEN0626Sensor` presents the **byte-for-byte identical** `person_sensor_face_t` struct
and the same method surface IRIS already calls, so consumer code is (almost) untouched.

## The two consumers (one sensor family, two interfaces)

The dead Person Sensor fed **two** separate Teensies in IRIS. Both are covered:

| | Eyes (gaze) | Head-pan servo |
|---|---|---|
| IRIS driver replaced | `PersonSensor` class | `person_sensor.{h,cpp}` free functions → `PersonResult` |
| Axes used | X + Y (full gaze) | X only (`faceCenterX`, pans left/right) |
| Drop-in provided | `../src/sensors/SEN0626Sensor.{h,cpp}` (the class shim) | `../integration/servo_teensy40_base_mount/person_sensor.{h,cpp}` |

Both move to UART: SEN0626 on `Serial1` (sensor TX→pin 0, RX→pin 1), not I²C. Same wiring recipe
as CyclopsGaze ([WIRING.md](WIRING.md)).

### Eyes port

1. Copy `../src/sensors/SEN0626Sensor.{h,cpp}` into IRIS `src/sensors/`.
2. `#include "sensors/SEN0626Sensor.h"` and change the sensor object to
   `SEN0626Sensor personSensor(Serial1);` (keep the name so call sites are untouched).
   Neutralize the old I²C bring-up on this path — the existing `isPresent()` probe loop works
   unchanged because `SEN0626Sensor::isPresent()` lazily runs `begin()` until the sensor answers.
3. Decide the confidence scale (see [Divergence](#the-one-deliberate-divergence-confidence-scale)).

### Servo port

1. Copy `../src/sensors/SEN0626Sensor.{h,cpp}` into the servo project alongside it.
2. Replace that project's `person_sensor.{h,cpp}` with the two files in
   `../integration/servo_teensy40_base_mount/`. The servo `.ino` is unchanged.
3. Wire SEN0626 to that Teensy's `Serial1`. Leave the `.ino`'s `Wire.begin()` (a gesture sensor
   still uses I²C); only the Person Sensor moves to UART.

## What the real swap uncovered

The SEN0626 was actually swapped into live IRIS. Two findings came out of it and were synced
back into this repo at CG-S12:

### The center-only-box bug (the big one)

IRIS's face-selection loop used box **area** as a "is there a face?" test (`size > maxSize`, with
`maxSize` starting at 0). But the SEN0626 reports a face **center**, so the shim stores
`box_left==box_right` and `box_top==box_bottom` → area is always `0`, `0 > 0` is false, and
**every face was silently discarded** — before both the tracking update and the status report.
Symptom: sensor detected fine, zero tracking, "no face in view" forever.

Fix (IRIS side): area is a *ranking* key ("of the gated faces, pick the biggest"), never a
presence test — gate on face **count**, not area. After the fix, IRIS tracked live (many
acquisitions over minutes).

> This is the canonical gotcha for adapting any center-only sensor into a box-oriented consumer.
> CyclopsGaze's own loop never hit it (it reads the center directly), but it's documented here and
> in [SEN0626_PROTOCOL.md](SEN0626_PROTOCOL.md) so nobody re-discovers it the hard way.

### Gaze shaping: per-axis signed gain + bias

The real bench (18–24 in) showed the eyes **mirrored** L/R and only ~15° total travel. Both were
addressable without touching consumer logic once gaze used a `targetN = rawN * gain + bias` model
per axis:

- **Mirror = direction** → fixed by the gain's **sign**.
- **~15° travel is not a bug** → at 20 in from an 85° FOV, a ±6 in head move only crosses ~40% of
  frame ≈ 0.4 deflection. Fixed by the gain's **magnitude** (|gain| > 1 amplifies; the eye
  controller clamps to the unit circle so over-gain saturates gracefully).

CyclopsGaze adopted this same model at CG-S12 (`GAZE_X_GAIN`/`GAZE_Y_GAIN`/`GAZE_X_BIAS`/
`GAZE_Y_BIAS`).

## The one deliberate divergence: confidence scale

This is the **only** intended difference between this repo's driver and the IRIS copy:

- **IRIS** gates on a runtime `psConfGate` knob clamped to **0–100**. So the IRIS shim emits the
  **raw DFRobot score (0–100)** as `box_confidence`. (Emitting 0–255 there made every real
  detection clear any reachable gate, silently disabling the operator's confidence knob.)
- **CyclopsGaze** (as of CG-S12) now **also** emits the raw 0–100 score and gates at `60`, so the
  two are aligned. Earlier CyclopsGaze revisions emitted `score*255/100` and gated at `152` — the
  same effective threshold (152/255 ≈ 0.60), just a different scale.

**Do not** copy CyclopsGaze's single-eye/bench-mount tuning verbatim into IRIS without a bench
check — IRIS has two eyes and its own mount geometry, so its gain signs and biases are its own
call. The *struct and driver* are the drop-in; the *tuning numbers* are per-installation.

## Separable camera module (mounting)

The SEN0626's camera lens/sensor is attached to the Gravity board by a ribbon cable + tape, and
bench-tested working with the camera **detached** from the rest of the board — which would let it
mount in a footprint much closer to the tiny original Person Sensor. **Not yet verified:** that
the detached camera answers on the same register set / address / baud once separated. Confirm raw
register output attached-vs-detached before relying on it. (See [ENGINEERING_LOG.md](ENGINEERING_LOG.md)
CG-S10.)

## Status

- SEN0626 is **DEPLOYED and tracking live in IRIS** (eyes path).
- CyclopsGaze standalone tracking was bench-VERIFIED at CG-S8; the CG-S12 sync-back is
  **compile-verified only** — re-run [BENCH_PROTOCOL.md](BENCH_PROTOCOL.md) to re-confirm.
- The servo adapter is code-reviewed against the live interface but flash-verify it on the servo
  Teensy before relying on it.
