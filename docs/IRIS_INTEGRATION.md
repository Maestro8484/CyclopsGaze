# IRIS Integration

How the CyclopsGaze SEN0626 driver drops into the parent IRIS robot-face project, what the
real swap uncovered, and the one deliberate way the IRIS copy diverges from this repo.

IRIS is a private project; this doc covers only what's needed to understand and reproduce the
drop-in. The ready-to-copy adapter files live in [`../integration/`](../integration/).

## The goal

IRIS drove its animated eyes' gaze (and a head-pan servo) from the Useful Sensors Person
Sensor (SEN-21231). Useful Sensors discontinued it. CyclopsGaze exists to prove the DFRobot
SEN0626 is a true drop-in so IRIS keeps working and stays publicly replicable. "Drop-in" here
is literal: `SEN0626Sensor` presents the byte-for-byte identical `person_sensor_face_t` struct
and the same method surface IRIS already calls, so consumer code is almost untouched.

## The two consumers (one sensor family, two interfaces)

The dead Person Sensor fed two separate Teensies in IRIS. Both are covered:

| | Eyes (gaze) | Head-pan servo |
|---|---|---|
| IRIS driver replaced | `PersonSensor` class | `person_sensor.{h,cpp}` free functions → `PersonResult` |
| Axes used | X + Y (full gaze) | X only (`faceCenterX`, pans left/right) |
| Drop-in provided | `../src/sensors/SEN0626Sensor.{h,cpp}` (the class shim) | `../integration/servo_teensy40_base_mount/person_sensor.{h,cpp}` |

Both move to UART: SEN0626 over Modbus RTU, not I²C. Same wiring recipe as CyclopsGaze
([WIRING.md](WIRING.md)) except the port, see below.

### The UART is not the same port on both boards

CyclopsGaze's bench rig uses `Serial1` (RX 0 / TX 1). The live IRIS eyes node can't: pin 0 is
IRIS's left-eye CS. IRIS therefore runs the sensor on `Serial4` (RX 16 / TX 17). IRIS
`src/config.h`, "Gaze sensor transport (S212)"; Serial2 and Serial8 were also ruled out, they
collide with the ILI9341 mouth's DC and MOSI. Verified against the Teensy core, not assumed.

Since CG-S13 the port is a one-line knob rather than a hardcoded constructor argument:
`#define SEN0626_SERIAL Serial1` in `../src/config.h`. Set it to whatever's free on the target
board. Either way: sensor D/T → the RX pin, sensor C/R → the TX pin.

### Eyes port

1. Copy `../src/sensors/SEN0626Sensor.{h,cpp}` into IRIS `src/sensors/`.
2. `#include "sensors/SEN0626Sensor.h"` and change the sensor object to
   `SEN0626Sensor personSensor(<free UART>);` (keep the name so call sites are untouched).
   Neutralize the old I²C bring-up on this path. The existing `isPresent()` probe loop works
   unchanged because `SEN0626Sensor::isPresent()` lazily runs `begin()` until the sensor answers.
3. Decide the confidence scale (see [Divergence](#the-one-deliberate-divergence-confidence-scale)).

### Servo port

1. Copy `../src/sensors/SEN0626Sensor.{h,cpp}` into the servo project alongside it.
2. Replace that project's `person_sensor.{h,cpp}` with the two files in
   `../integration/servo_teensy40_base_mount/`. The servo `.ino` is unchanged.
3. Wire SEN0626 to that Teensy's `Serial1`. Leave the `.ino`'s `Wire.begin()` (a gesture
   sensor still uses I²C); only the Person Sensor moves to UART.

## What the real swap uncovered

The SEN0626 was actually swapped into live IRIS. Two findings came out of it and were synced
back into this repo at CG-S12:

### The center-only-box bug (the big one)

IRIS's face-selection loop used box area as a "is there a face?" test (`size > maxSize`, with
`maxSize` starting at 0). But the SEN0626 reports a face center, so the shim stores
`box_left==box_right` and `box_top==box_bottom` → area is always 0, `0 > 0` is false, and
every face was silently discarded, before both the tracking update and the status report.
Symptom: sensor detected fine, zero tracking, "no face in view" forever.

Fix (IRIS side): area is a ranking key ("of the gated faces, pick the biggest"), never a
presence test. Gate on face count, not area. After the fix, IRIS tracked live (many
acquisitions over minutes).

This is the canonical gotcha for adapting any center-only sensor into a box-oriented
consumer. CyclopsGaze's own loop never hit it (it reads the center directly), but it's
documented here and in [SEN0626_PROTOCOL.md](SEN0626_PROTOCOL.md) so nobody re-discovers it
the hard way.

### Gaze shaping: per-axis signed gain + bias

The real bench (18–24in) showed the eyes mirrored L/R and only ~15° total travel. Both were
addressable without touching consumer logic once gaze used a `targetN = rawN * gain + bias`
model per axis:

- Mirror = direction, fixed by the gain's sign.
- ~15° travel isn't a bug. At 20in from an 85° FOV, a ±6in head move only crosses ~40% of
  frame ≈ 0.4 deflection. Fixed by the gain's magnitude (|gain| > 1 amplifies; the eye
  controller clamps to the unit circle so over-gain saturates gracefully).

CyclopsGaze adopted this same model at CG-S12 (`GAZE_X_GAIN_DEFAULT` / `GAZE_Y_GAIN_DEFAULT` /
`GAZE_X_BIAS_DEFAULT` / `GAZE_Y_BIAS_DEFAULT`).

### Live tuning: the `PS_CFG:` protocol (came back at CG-S13)

The single most useful thing the IRIS install had that CyclopsGaze didn't. Every gate/gain/
bias/timeout is a runtime variable settable over serial with no reflash: `PS_CFG:X_GAIN=2.5`
and the eye's travel changes on the next sample. IRIS added it at S141 and extended it with
the per-axis keys at S212c; CyclopsGaze ported the parser verbatim at CG-S13, including:

- the same key set (`CONF`, `FACING`, `LOST_MS`, `X_GAIN`, `Y_GAIN`, `X_BIAS`, `Y_BIAS`, `LED`),
- the same `[DBG] PS_CFG KEY=value` ack wording, kept even though this repo's own log prefix
  is `[CG]`, so IRIS's tooling (`iris_post.py` scrapes `\[DBG\] PS_CFG (\w+)=(\S+)`) drives a
  CyclopsGaze board unmodified,
- the S212c false-ack guard: an unimplemented key must answer `UNKNOWN key` and must not
  match that ack regex. Before the guard, S212b happily echoed `PS_CFG X_GAIN=1.0` while
  having no `psXGain` variable at all, a false confirmation in the WebUI that also made the
  config-drift check report no drift.
- the same runtime variable names (`psConfGate`, `psXGain`, `psYBias`, …) so the two
  `main.cpp` files stay directly diffable.

The one addition CyclopsGaze makes is `PS_CFG?`, a one-line readback of the live values. IRIS
doesn't need it: its Pi4 holds the authoritative copy in `ps_config.json` and renders it in
the WebUI. But a standalone board has no such store, so there's otherwise no way to read
values back mid-bench. It's deliberately not in the ack shape, so it can't be mistaken for one.

Values are RAM-only on the standalone board and revert on reset. IRIS persists via the Pi4 and
re-pushes on serial open; here, a proven value must be written back into `config.h`.

## The one deliberate divergence: confidence scale

This is the only intended difference between this repo's driver and the IRIS copy:

- IRIS gates on a runtime `psConfGate` knob clamped to 0–100. So the IRIS shim emits the raw
  DFRobot score (0–100) as `box_confidence`. (Emitting 0–255 there made every real detection
  clear any reachable gate, silently disabling the operator's confidence knob.)
- CyclopsGaze (as of CG-S12) now also emits the raw 0–100 score and gates at 60, so the two
  are aligned. Earlier CyclopsGaze revisions emitted `score*255/100` and gated at 152, the
  same effective threshold (152/255 ≈ 0.60), just a different scale.

Don't copy CyclopsGaze's single-eye/bench-mount tuning verbatim into IRIS without a bench
check. IRIS has two eyes and its own mount geometry, so its gain signs and biases are its own
call. The struct and driver are the drop-in; the tuning numbers are per-installation.

## The tuning-value gap (open, CG-S13)

The rule above cuts both ways, and the numbers currently differ:

| Knob | CyclopsGaze default | Live IRIS (observed 2026-07-21) |
|---|---|---|
| `X_GAIN` / `Y_GAIN` | `1.7` / `1.7` | `1.0` / `1.0` |
| `X_BIAS` / `Y_BIAS` | `0.0` / `1.26` | `0.0` / `0.0` |
| `CONF` | `60` | `25` |
| `LOST_MS` | `3000` | `8500` |

CyclopsGaze's numbers are measured (CG-S7 took the 1.7 from a measured `rawX` span of
151–427; CG-S8 took `Y_BIAS` from the measured below-eye mount where eye level images at
`cy≈33`).

IRIS's are not tuned: the Pi4's `/home/pi/ps_config.json` is dated 2026-07-15 (before the
SEN0626 swap) and contains no `X_GAIN`/`Y_GAIN`/`X_BIAS` keys at all, so all three fall back
to the firmware's compile-time defaults of `1.0`/`1.0`/`0.0`. `CONF=25` is a Person-Sensor-era
value on the old 0–255 scale (~10%), now being applied to the SEN0626's raw 0–100 score, well
under DFRobot's documented 60 floor. IRIS's own S212 source comment flags raising it as an
unmade operator decision. IRIS does track this way (79 face acquisitions on 2026-07-21), so
it's not broken, just un-tuned.

CyclopsGaze therefore keeps its own measured defaults. Open item: run the two value sets
head-to-head on one rig and record which actually gazes better: CyclopsGaze's amplified/
biased shaping or IRIS's identity mapping. Until then neither set is established as better;
only CyclopsGaze's has bench data behind it. Now cheap to test: both sets are `PS_CFG:`
one-liners.

## Separable camera module (mounting)

The SEN0626's camera lens/sensor is attached to the Gravity board by a ribbon cable + tape,
and bench-tested working with the camera detached from the rest of the board, which would
let it mount in a footprint much closer to the tiny original Person Sensor. Not yet verified:
that the detached camera answers on the same register set / address / baud once separated.
Confirm raw register output attached-vs-detached before relying on it. (See
[ENGINEERING_LOG.md](ENGINEERING_LOG.md) CG-S10.)

## Status

*(Verified against the IRIS repo and the live Pi4 on 2026-07-21.)*

- SEN0626 is DEPLOYED and tracking live in IRIS (eyes path), firmware S213, on `Serial4`.
- IRIS's gaze code is frozen at S212c. Its last touch of `main.cpp`/`config.h` was S213, which
  changed only `PROTOCOL_VERSION`. `SEN0626Sensor.{h,cpp}` are byte-identical between the two
  repos (comment text aside); the driver hasn't drifted.
- The servo node in IRIS is still running the original I²C Person Sensor driver. The adapter
  in `../integration/` has never been applied there. It's code-reviewed against the live
  interface; flash-verify it on the servo Teensy before relying on it.
- CyclopsGaze standalone tracking was bench-VERIFIED at CG-S8; the CG-S12 sync-back and the
  CG-S13 `PS_CFG:` port are compile-verified only, re-run [BENCH_PROTOCOL.md](BENCH_PROTOCOL.md).
