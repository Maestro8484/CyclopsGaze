# SEN0626 Protocol Reference

How CyclopsGaze talks to the DFRobot SEN0626 Gravity AI vision camera, and how its output
maps into the Useful Sensors Person Sensor struct. Driver source:
[`../src/sensors/SEN0626Sensor.cpp`](../src/sensors/SEN0626Sensor.cpp).

Primary vendor reference: DFRobot's `DFRobot_GestureFaceDetection` library and the SEN0626
wiki (`wiki.dfrobot.com/sen0626`). Everything below is cross-checked against that library and
confirmed on the bench except where marked assumed/unconfirmed.

## Transport

- **Protocol:** Modbus RTU over UART (not raw binary packets).
- **Device address:** `0x72`.
- **Baud:** `9600` factory default; `115200` also supported. The driver auto-detects — tries
  115200 then 9600, up to 3 full sweeps, after a 2s power-on settle (the sensor loads an AI
  model into RAM before it answers Modbus).
- **Wiring:** sensor on the Teensy's `Serial1` — sensor TX → Teensy pin 0, sensor RX → Teensy
  pin 1. The breakout's I²C/UART DIP switch must be set to UART. (See [WIRING.md](WIRING.md).)

## Registers (FC04 — read input registers)

| Reg | Meaning | Notes |
|----:|---------|-------|
| `0x00` | PID | expect `0x0272` — used for presence detection in `begin()` |
| `0x01` | VID | |
| `0x04` | face_number | 0–3, number of faces detected |
| `0x05` | face_x | face center X, `0–640` |
| `0x06` | face_y | face center Y, `0–480` — assumed VGA, unconfirmed |
| `0x07` | face_score | `0–100` |

Registers `0x04`–`0x07` are contiguous and read in a single FC04 transaction (~22ms at 9600
baud). The driver self-throttles to `SAMPLE_TIME_MS = 150`.

## Key protocol facts (and how the driver handles them)

**No bounding box — the sensor reports a face center, not a box.** The register map exposes
only `face_number/x/y/score` (plus gesture/hand registers); there are no width/height/
box-edge registers. Center-only is the best available data, not a shortcut. The driver stores
the same center in both box edges:

```
box_left == box_right == cx      (so consumers recover the exact center, no edge drift)
box_top  == box_bottom == cy
```

Gotcha for consumers: any code that treats box **area** (`(right-left)*(bottom-top)`) as a
presence test will see area = 0 and silently discard every face. Area is a ranking key at
most, never a presence test — this is the exact bug the real IRIS swap hit, see
[IRIS_INTEGRATION.md](IRIS_INTEGRATION.md).

**No `is_facing` field.** The SEN0626 has no "looking at me" bit, so the driver hardcodes
`is_facing = 1`. Non-blocking for IRIS (its `psFacingRequired` defaults false). If ever
needed, derive facing from the sensor's head-pose/gesture registers.

**Confidence scale (CG-S12).** The driver emits the raw DFRobot score (0–100) as
`box_confidence`, and the consumer gates at `> 60` (DFRobot's own documented validity floor:
"a score ≥ 60 is considered valid"). Earlier revisions emitted `score*255/100` (0–255) and
gated at 152 — same effective threshold, different scale. See [CHANGELOG](../CHANGELOG.md)
CG-S11/S12.

## Coordinate remap (sensor → 0–255 struct space)

```
NATIVE_W = 640
NATIVE_H = 480        # ASSUMED — confirm on bench via rawFaceY() (BENCH_PROTOCOL step 6)
cx = clamp(face_x, 0..640) * 255 / 640
cy = clamp(face_y, 0..480) * 255 / 480
box_confidence = clamp(face_score, 0..100)     # raw score, CG-S12
```

X and Y are each normalized over their own native span so a face at either frame edge drives
the gaze target to full deflection on that axis — correct edge-to-edge mapping despite the
4:3 vs 1:1 aspect mismatch. Intentional, not a bug.

## Drop-in contract

`SEN0626Sensor` exposes the byte-for-byte Useful Sensors `person_sensor_face_t` struct and a
method surface (`begin/isPresent/read/enableID/setMode/enableLED/numFacesFound/faceDetails/
timeSinceFaceDetectedMs`) matching the Person Sensor, so Person-Sensor consumers compile and
run unchanged. The `raw*()` accessors and `detectedBaud()` are additive calibration-only
extras (never called by consumers) and don't break the contract.

## Known unconfirmed items

- **`NATIVE_H` = 480 is assumed.** DFRobot documents the X range (0–640) but not Y. Confirm
  the true max `face_y` via the raw serial logging (`CG_CALIB_RAW`) — see BENCH_PROTOCOL
  step 6.
- **No LED control register exists** (confirmed against DFRobot's map + library).
  `enableLED()` is a no-op stub; to disable the sensor's onboard LEDs, cover them physically.
