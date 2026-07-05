# CyclopsGaze — Session Notes

## Purpose & Deployment Framing (operator, 2026-07-04)

CyclopsGaze is a **future-proofing / public-launch hardware change**, NOT a fix
for a current problem. IRIS's live Useful Sensors Person Sensor (SEN-21231) is
**working fine for the operator right now** — nothing about IRIS is broken and
nothing here changes live IRIS. This project exists because the Person Sensor is
**discontinued** [[person_sensor_irreplaceable]], so a drop-in replacement is
needed (a) as insurance if the live sensor ever dies, and (b) as the publicly-
**replicable** gaze path for the IRIS launch (buyers can't source a discontinued
part).

Deployment posture: **bench-only, on a spare NON-INSTALLED Teensy 4.1** — do NOT
pull the live Person Sensor or touch installed IRIS hardware to test this. Flash
+ verify happens on the standalone bench board at a later date, when the T4.1 is
connected. No timeline pressure; the live Person Sensor stays the source of
truth until CyclopsGaze is proven under load. Status stays REPO-ONLY until then.

## SEN0626 Protocol (from DFRobot GitHub, 2026-06-24)

Source: https://github.com/DFRobot/DFRobot_GestureFaceDetection

Protocol: Modbus RTU over UART (NOT raw binary packets as originally anticipated in handoff)
Device address: 0x72 (from DFRobot example detectGesture.ino)
Default baud rate: 9600 (factory default; 115200 also supported via configUart)
PID register: input reg 0x00 = 0x0272 (used for presence detection in begin())

Registers (input, FC04):
  0x00 = PID         (expect 0x0272)
  0x01 = VID
  0x04 = face_number (0-3, number of faces detected)
  0x05 = face_x      (center X, 0-640)
  0x06 = face_y      (center Y, 0-480 assumed VGA — unconfirmed, verify on bench)
  0x07 = face_score  (0-100)

No bounding box: SEN0626 returns face CENTER (X, Y), not a box. The DFRobot
register map (`DFRobot_GestureFaceDetection`) exposes only face_number, face_x,
face_y, face_score, plus gesture/hand registers — there are NO width/height or
box-edge registers to recover. Center-only is therefore the *best available*
data, not a shortcut. Shim stores the SAME center in BOTH edges
(box_left==box_right==cx, box_top==box_bottom==cy) so IRIS-side consumers recover
the exact target center even at frame edges. (Superseded the earlier cx±32
fake-box scheme — the b84033d "center box" fix; a fake width biased the recovered
center near the edges.)

No is_facing field: always set is_facing=1 in shim.
  KNOWN GAP: the real Person Sensor is_facing (is_looking_at) drives IRIS
  facing-gate logic. SEN0626 has no direct equivalent here; hardcoded true
  means "facing" is always asserted. Non-blocking for live IRIS because
  psFacingRequired defaults false since IRIS S153c. If ever re-enabled, derive
  facing from SEN0626 head-pose/gesture registers first (see 09_IRIS_INTEGRATION_PLAN §5).

Coordinate remap (matches SEN0626Sensor.cpp read()):
  NATIVE_W = 640
  NATIVE_H = 480 (ASSUMED — confirm on bench via rawFaceY())
  cx = faceX * 255 / 640   (clamped faceX<=640)
  cy = faceY * 255 / 480   (clamped faceY<=480)
  box_left = box_right = cx
  box_top  = box_bottom = cy
  box_confidence = score * 255 / 100   (score clamped 0-100)

Modbus multi-register read: registers 0x04-0x07 read in one FC04 transaction
(4 registers contiguous), ~22ms per read at 9600 baud. SAMPLE_TIME_MS = 150.

Deviations from handoff:
1. Protocol is Modbus RTU, not raw streaming packets.
2. No bounding box — center stored in both box edges.
3. No is_facing field — hardcoded true.
4. Native Y resolution unconfirmed (480 assumed). Verify on bench.
5. SAMPLE_TIME_MS = 150ms (4 Modbus reads + inter-frame delay ~42ms; 150ms margin).

## Build

Target: Teensy 4.1. platformio.ini board = teensy41.
CG-S3 clean build (2026-07-04):
  Single-eye (default): SUCCESS ~5.0s. FLASH code 74076 / data 361032.
    RAM1 free 414880; RAM2 free 511872. Only the benign C++17 `std::pair`
    argument-passing ABI note (present in the IRIS-verbatim EyeController;
    informational, not a warning).
  Dual-eye (`-DDUAL_EYE`): SUCCESS ~8.3s. FLASH code 74268. Builds clean too.

Drop-in contract (re-confirmed CG-S3): SEN0626Sensor's person_sensor_face_t
matches IRIS src/sensors/PersonSensor.h byte-for-byte; the Person Sensor method
surface matches. The CG-S3 raw* calibration accessors are ADDITIVE (IRIS never
calls them) so the contract is intact.

---

## CG-S3 Audit — findings & verdicts (2026-07-04)

Every item from the peer review (07) and Opus handoff (10) §3 was traced against
live code and the IRIS reference. Verdicts below; "confirmed OK" items are
recorded so they are not re-litigated next session.

### 3.1 Y-direction — VERDICT: CORRECT (not inverted). No code change.

The peer review (07 §Q4) hypothesised targetY needs negation. **Rejected on the
firmware-logic side.** CyclopsGaze main.cpp uses `targetX = -((cx/127.5)-1)`,
`targetY = ((cy/127.5)-1)` — X negated, Y **not**. IRIS's production main.cpp
(`src/main.cpp:594-595`) uses the *identical* sign convention against the
*identical* (IRIS-verbatim) EyeController. IRIS tracking is confirmed-correct in
the field, so the mapping/render chain is correct; the peer review's polar-map
step for Y was the error, not the code.

Residual risk is **sensor-side only**: whether the SEN0626 reports face_y with
0=top-of-frame (same as the Person Sensor) or 0=bottom (inverted). That is a
hardware property, resolved on the bench (protocol step 5). If vertical tracking
is inverted there, the fix is a *single* sign flip on targetY in main.cpp — but
do NOT pre-negate now, as that would break the match with the known-good IRIS
convention if the SEN0626 shares the PS orientation.

### 3.2 autoMove freeze — VERDICT: FIXED / complete. Verified.

The restructured loop evaluates the resume branch as an `else if` that runs
whenever there is no qualifying face — including when `sensor.read()` returns
false (throttle window) or returns true with a comms failure (readFaceData→0→
num_faces 0). `timeSinceFaceDetectedMs()` keeps climbing in every one of those
paths, so after `FACE_LOST_MS` autoMove resumes and can never stick OFF. Traced
all paths (sensor absent, present-but-silent, throttled, low-confidence) — no
stuck state. FACE_LOST_MS moved to a named config constant (was a bare 3000).

### 3.3 Center recovery — VERDICT: EXACT, no drift. No change.

box_left==box_right==cx → IRIS's `left+(right-left)/2` = cx exactly; box_top==
box_bottom==cy → `top+(bottom-top)/3` = cy exactly. CyclopsGaze main reads
box_left/box_top directly, which is the same value. No integer truncation drift
at any value including 0 and 255 (0+0/2=0, 255+0/2=255). Center-only is the best
available data (§Protocol: no box-dimension registers exist).

### 3.4 Aspect-ratio mapping — VERDICT: correct, not a bug. No change.

Independent per-axis normalisation (X over 640, Y over 480) maps each frame edge
to full gaze deflection on that axis. That is the DESIRED edge-to-edge behavior:
a face at the top edge should drive the eye fully up regardless of the 4:3 vs
1:1 aspect. A single uniform scale factor would UNDER-drive the vertical axis
and is the wrong fix. Matches how the Person Sensor already normalises per-axis
into 0-255. Documented inline in SEN0626Sensor.cpp read().

### 3.5 NATIVE_H = 480 unconfirmed — mitigation added.

Still assumed. Added raw register logging: shim now stores lastRawX/Y/Score and
exposes rawFaceX()/rawFaceY()/rawScore(); main.cpp prints them under
`#define CG_CALIB_RAW 1` (config.h, default ON). Bench step 6 reads the true max
faceY off serial with no scope. If max ≈480 keep NATIVE_H=480; if ≈640 set 640.

### 3.6 Modbus timeout — VERDICT: 300ms too long. Reduced to 100ms.

Normal round trip after flush ≈25ms (13.5ms wire for the 13-byte reply at 9600 +
sensor turnaround). Old 300ms meant a stalled/partial response froze renderFrame
for up to 300ms. New `MODBUS_RESP_TIMEOUT_MS = 100` keeps a ~4x margin against
false timeouts while capping the worst-case eye freeze at 100ms. Only bites on an
actual comms glitch, at most once per 150ms sample.

### 3.7 Confidence gate — divergence fixed + bench-calibration flagged.

CyclopsGaze main previously tracked ANY detected face with NO confidence gate,
unlike IRIS (`face.box_confidence > psConfGate`). Added `PS_CONF_GATE` (config.h,
default **45**, matching IRIS S153c). Threshold math with the shim's
score*255/100 map and strict `>`: score 18 → conf 45 (NOT >45, fails); score
19 → conf 48 (passes). So the effective minimum SEN0626 score is **19/100**.
BENCH CALIBRATION: at ~1 m frontal, watch the `conf=` field. If a clearly-
detected face reports conf ≤45 (raw score ≤18), lower PS_CONF_GATE until stable
frontal faces pass but noise doesn't. Note the lost-timer follows raw detection
(shim resets it on any face in the register), same as IRIS — a persistent
sub-gate face holds position rather than resuming autoMove.

### 3.8 mapRadius / tracking range — VERDICT: correct, matches IRIS. Knob added.

nordicBlue polar.mapRadius = 240 (confirmed: `{240, polarAngle_240, ...}` in the
eye initializer; PolarParams default is also 240). r = (240*2 − 240*π/2)*0.75 =
(480 − 376.99)*0.75 ≈ **77.3**. At xTarget=±1: eyeNewX = 240 ∓ 77.3. This is the
exact value the production IRIS eyes use (identical EyeController + nordicBlue),
so the range is field-proven — the iris reaches its intended travel without the
display cropping it. Added `GAZE_GAIN` (config.h, default 1.0) multiplying
targetX/targetY for bench range-tuning; setTargetPosition clamps to the unit
circle so gain >1 is safe.

### 3.9 Baud auto-detect robustness — hardened.

Old: 500ms settle then probe, per baud, no retry. The SEN0626 boots an AI model
into RAM and may not answer that early (IRIS waits ~2.5s for the Person Sensor's
ML boot). New begin(): (1) holds until `BOOT_SETTLE_MS`=2000ms from power-on
before the first probe; (2) per-baud line settle trimmed to 200ms (boot is now
covered by the floor); (3) retries the full 115200→9600 sweep `BAUD_ATTEMPTS`=3
times, 300ms apart. Logs the winning baud + attempt number. Worst case (sensor
absent) ≈ 2s floor + ~2s of sweeps.

### 3.10 Other findings.

- Confidence gate was entirely absent from the consumer loop (fixed in 3.7).
- Magic numbers (3000 lost-timeout, implicit no-gain) lifted into named config
  constants (FACE_LOST_MS, GAZE_GAIN, PS_CONF_GATE) for bench tuning.
- `src/CyclopsGaze.ino` is a one-line dummy ("code is in main.cpp"); PlatformIO
  compiles it harmlessly alongside main.cpp. Left as-is.
- Modbus CRC, frame layout, flush-before-write, CRC-before-length validation:
  re-reviewed, all correct (peer review §5 stands).
- Multi-face selection: SEN0626 shim returns ≤1 face; the "largest face" loop in
  IRIS is harmless dead code against this shim.

---

## Flash & Verify — Bench Protocol (first flash)

REPO-ONLY. Run this once a spare T4.1 + SEN0626 + display(s) are wired per
05_WIRING.md. An operator new to PlatformIO can follow it top to bottom. Each
step: action → expected serial → pass/fail. Serial monitor at **115200**.

Firmware version in repo: **CG-S3** (bump before flashing if you change code).

### 1. USB enumeration

Action: connect the T4.1 by USB, then:
```
pio device list
```
Expect a line naming a Teensy port (e.g. `COM# ... USB Serial (Teensy)`).
PASS: a Teensy port appears. FAIL: none → check cable/board (CG-S2 saw only
COM1 legacy + COM4/5 Bluetooth, i.e. no Teensy).

### 2. Flash + boot message

Action:
```
pio run -e cyclopsgaze -t upload
pio device monitor -b 115200
```
Expect:
```
[CG] CyclopsGaze CG-S3
```
PASS: version line matches config.h FIRMWARE_VERSION. FAIL: no line / wrong
version → confirm upload target and monitor baud.

### 3. SEN0626 detect (baud + PID)

Expect within ~4s of boot ONE of:
```
[CG] SEN0626 found at 9600 (attempt 1)
[CG] SEN0626 found at 115200 (attempt 1)
```
Record WHICH baud. FAIL:
```
[CG] SEN0626 NOT FOUND at 115200 or 9600 -- check wiring
```
→ check TX→pin0 / RX→pin1 cross, 3.3V, GND. (PID 0x0272 is validated internally;
a wrong PID reads as NOT FOUND.)

### 4. Face detect (serial format)

Action: sit ~1 m in front of the sensor, facing it. Expect (CG_CALIB_RAW on):
```
[CG] faces=1 conf=NNN x=+0.NN y=+0.NN | rawX=NNN rawY=NNN rawScore=NN
```
Meaning: `conf` = box_confidence 0-255 (=score*255/100); `x`,`y` = target in
[-1,+1] after negation/gain; `rawX/rawY` = sensor center 0-640 / 0-480(?);
`rawScore` = 0-100. PASS: faces=1 with plausible values when a face is present,
faces logging stops when you leave. FAIL: never faces=1 with a clear face → see
step 7 (confidence).

### 5. Direction verification (GROUND TRUTH from audit 3.1)

Move slowly and check the eye AND the serial signs:

| Stimulus (your position vs sensor) | Expected `x`/`y` sign | Expected eye |
|---|---|---|
| Face to camera's RIGHT | x more negative | eye looks RIGHT |
| Face to camera's LEFT  | x more positive | eye looks LEFT |
| Face toward TOP of frame | y toward −1 | eye looks UP |
| Face toward BOTTOM of frame | y toward +1 | eye looks DOWN |
| Face centered | x≈0 y≈0 | eye straight ahead |

PASS: eye follows you in all four directions. Horizontal is verified-correct by
design (matches IRIS). **If VERTICAL is inverted** (face up → eye down): that's
the SEN0626 Y-axis orientation differing from the Person Sensor — flip the sign
of `targetY` in main.cpp (`((cy/127.5f)-1.0f)` → `-((cy/127.5f)-1.0f)`), reflash,
and note it here. Do not touch targetX.

### 6. NATIVE_H calibration (480 vs 640)

Action: keep a face in frame; move to the very TOP then very BOTTOM of the
field, watching `rawY`. Note the MAXIMUM rawY you can produce at the bottom edge.
- Max rawY ≈ **480** → NATIVE_H=480 is correct, leave it.
- Max rawY ≈ **640** → set `NATIVE_H = 640` in SEN0626Sensor.h, reflash. (At
  present, Y > ~480 clamps and the eye saturates vertically before the true edge.)
Record the observed max here. Also sanity-check rawX maxes near 640.

### 7. Confidence calibration

Action: at ~1 m frontal, read the `conf` field (and rawScore).
Expected: a clear frontal face should comfortably clear conf > 45 (raw score ≥19).
- If a clear face reads conf ≤45 / it won't track: lower `PS_CONF_GATE` in
  config.h (e.g. to 20-30) until stable frontal faces pass but empty-frame noise
  does not, reflash. Record the value that works.
- If noise/empty frames produce faces=1: raise PS_CONF_GATE.

### 8. AutoMove resume (lost timeout)

Action: with the eye tracking you, leave the frame entirely; start a timer.
Expect: ~3 s (FACE_LOST_MS) after your last detection the eye stops holding and
begins idle wander (random saccades). Return to frame → it re-locks.
PASS: wander resumes ≈3 s after you leave and never earlier while you're tracked.
FAIL: eye freezes forever → regression in the loop's else-if (audit 3.2).

### 9. Edge tracking / flaky comms

Action: (a) move to the extreme corners of the field and hold. Expect the eye to
reach its travel limit and stay smooth — no freeze, no snap-back, no drift while
you hold. (b) Briefly interrupt the sensor TX wire. Expect no crash; the eye
holds last position, then wanders after ~3 s, and re-locks when comms return.
PASS: both. FAIL: freeze/crash → note timeout (audit 3.6) or wiring.

### 10. Dual-eye addendum (only if built with `#define DUAL_EYE`)

Action: wire the second display (05_WIRING dual-eye table: CS9/DC8/RST6, shared
SCK13/MOSI11), uncomment `#define DUAL_EYE`, reflash.
Expect: BOTH displays show an eye at boot; both track the same face together;
neither is blank or frozen. PASS: two coordinated eyes. FAIL: second display
blank → check CS9/DC8/RST6 wiring and that SCK/MOSI are shared to pins 13/11.
Note: per-eye refresh is ~half single-eye (shared bus) — expected, not a fault.

---

## Issues Found (open)

- Hardware never flashed/bench-verified (REPO-ONLY). No T4.1 was enumerated on
  SuperMaster during CG-S1/S2/S3. Connect the spare T4.1 before the next flash
  session and run the bench protocol above.

## Next Session (bench — needs T4.1 connected + operator at bench)

- [ ] Steps 1-3: enumerate, flash CG-S3, confirm boot + SEN0626 baud (record which).
- [ ] Step 4: confirm face-detect serial line.
- [ ] Step 5: confirm L/R/U/D directions; flip targetY sign ONLY if vertical inverted.
- [ ] Step 6: record max rawY → confirm NATIVE_H 480 vs 640.
- [ ] Step 7: confirm/tune PS_CONF_GATE vs real scores.
- [ ] Step 8: confirm ~3s autoMove resume.
- [ ] Step 9: edge + flaky-comms robustness.
- [ ] (optional) Step 10: dual-eye if a second display is wired.
- [ ] After verify: set CG_CALIB_RAW to 0 for quieter serial, update this file to
      VERIFIED, then unblock 09_IRIS_INTEGRATION_PLAN §6.
