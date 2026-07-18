# CyclopsGaze — Engineering Log

> This is the full chronological engineering record (design decisions, the CG-S3 firmware
> audit, external research, bench findings, and status history). For focused reference, see the
> dedicated docs: **[SEN0626_PROTOCOL.md](SEN0626_PROTOCOL.md)** (register/protocol),
> **[BENCH_PROTOCOL.md](BENCH_PROTOCOL.md)** (first-flash procedure),
> **[WIRING.md](WIRING.md)**, and **[IRIS_INTEGRATION.md](IRIS_INTEGRATION.md)**. Where this log
> and a dedicated doc or the source disagree, the source and the newest CHANGELOG entry win —
> parts of this log predate the CG-S12 sync (raw-score gate + per-axis gain/bias).

## Session Notes

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
  facing from SEN0626 head-pose/gesture registers first (see IRIS_INTEGRATION.md §5).

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

### 3.7 Confidence gate — divergence fixed, then re-calibrated against the vendor's own spec (CG-S5).

CyclopsGaze main previously tracked ANY detected face with NO confidence gate.
CG-S3 added `PS_CONF_GATE` at **45**, borrowed from IRIS's unrelated
`psConfGate` constant (a different sensor's calibration, not the SEN0626's).
That mapped to an effective minimum SEN0626 score of just ~19/100.

**CG-S5: replaced with DFRobot's own documented threshold.** The SEN0626 setup
guide (wiki.dfrobot.com/sen0626/docs/23024) states outright: *"a score >=60 is
considered valid"*, and DFRobot's own sample code calls
`gfd.setFaceDetectThres(60)`. 19/100 was roughly a third of the vendor's own
validity floor — almost certainly accepting noisy/marginal detections, which
plausibly contributed to erratic tracking (see the lateral-tracking findings
below). `PS_CONF_GATE` is now **152** (`floor(60*255/100)-1`, so raw score 60
passes and 59 does not) — derived from spec, not guessed. Re-verify on the
bench: does `gate=PASS` now correlate with visibly stable tracking and
`gate=REJECT` with faces you wouldn't trust anyway? If DFRobot's own floor is
still too strict or too loose for this specific unit, adjust from measured
data, not intuition.

Note the lost-timer follows raw detection (shim resets it on any face in the
register, gated or not), same as before — a persistent sub-gate face holds
position rather than resuming autoMove.

### External research — SEN0626 real-world specs, and vs. the Person Sensor (2026-07-06)

Bench testing (see below) turned up a narrow effective detection envelope and
unstable lateral tracking below a certain distance. Before writing more
firmware, pulled the vendor's own documentation and forum history to check
whether these are known/expected characteristics rather than bugs:

- **Documented detection range: 0.5–3 m** (DFRobot wiki, `wiki.dfrobot.com/sen0626/`
  and the setup guide) for BOTH gesture and face recognition — no separate,
  tighter face-specific number is given. 0.5 m ≈ **19.7 inches**. The operator's
  bench observation of tracking degrading below ~15 inches is **below this
  documented floor** — that's out-of-spec operation, not a firmware bug. Bench
  step 9 (edge tracking) should be re-scoped to test at/above ~20 inches, not below.
- **Field of view: 85° diagonal only** — DFRobot does not publish separate
  horizontal/vertical FOV numbers. This is a real gap in their spec sheet (same
  gap we already flagged ourselves for NATIVE_H — DFRobot documents the 0-640 X
  coordinate range but never states the Y range or the H/V FOV split). No
  authoritative number exists to compute an exact degrees-per-pixel value for
  either axis.
  - CyclopsGaze does not have a confirmed vendor explanation for the reported
    lateral (X-axis)-specific jitter at close range vs. clean vertical (Y-axis)
    tracking. Working hypothesis, NOT vendor-confirmed: natural head-tracking
    behavior turns the head in **yaw** (left/right) more than **pitch**
    (up/down) when following something laterally, and yaw rotation changes a
    frontal-trained face detector's visible landmark set more than pitch does —
    at close range, where angular resolution per unit of physical head movement
    is coarsest, this could plausibly show up as X-specific centroid noise.
    This is reasoned inference, not something the datasheet or forum discusses
    — flagged for the next session to test empirically (see archive/11_HANDOFF_FABLE_LATERAL_TRACKING.md).
- **Confidence: DFRobot's own recommended validity floor is score >=60/100**
  (previous section, 3.7) — used to derive the CG-S5 `PS_CONF_GATE`.
- **Known real-world failure modes** (DFRobot forum, "SEN0626 Gesture and Face
  Detection Module failures," dfrobot.com/forum/topic/401101): dominant causes
  are power-supply instability ("unstable or low voltage can cause random
  resets, frozen output, or no detection at all") and wiring/comms issues, NOT
  a generally-faulty sensor. This matches this session's own bench finding
  exactly — a ~0.65V sag on the sensor's VCC (3.25V at the Teensy pin down to
  2.6V at the sensor, traced to a bad connector, not the regulator) was found
  and fixed before the confidence-gate work above. No third-party review
  reporting on tracking *precision* (as opposed to detection failures) was found.
- **Person Sensor (SEN-21231) comparison — honest gap:** no public datasheet
  with FOV-in-degrees or a documented minimum working distance was found for
  the Person Sensor (the SparkFun/DigiKey "datasheet" PDF is a one-page product
  blurb, not a technical spec sheet). We cannot make a precise numeric
  side-by-side. **Qualitatively**, though, this matters for the IRIS drop-in
  use case specifically: IRIS is a tabletop/desktop robot face meant to be
  looked at from close, conversational range, and its production history with
  the Person Sensor has never surfaced a "must stand back ~20 inches" complaint
  — while the SEN0626's documented 0.5 m floor sits right at or inside typical
  close conversational distance. **If the operator's own IRIS interaction
  distance is regularly under ~20 inches, the SEN0626 is a real, sourced
  downgrade for that specific use case**, not just a tuning gap — this should
  weigh on the IRIS_INTEGRATION.md gate decision, not just be tuned around.

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
WIRING.md. An operator new to PlatformIO can follow it top to bottom. Each
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
a wrong PID reads as NOT FOUND.) **Check the onboard DIP switch first** — the
SEN0626 breakout has a physical I2C/UART mode switch; this firmware is
UART-only. Confirmed on bench (2026-07-06): board shipped/left in I2C mode
produced NOT FOUND with correct wiring and correct code — flipping the switch
to UART fixed it immediately, no code change. This is the most likely single
cause of a first-flash NOT FOUND and should be checked before re-wiring.

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
Expected (CG-S5): a clear frontal face should comfortably clear conf > 152
(raw score ≥60 — DFRobot's own documented validity floor, ENGINEERING_LOG.md "External
research").
- If a clear face reads conf ≤152 / it won't track: this means DFRobot's own
  recommended floor is too strict for this specific unit — lower `PS_CONF_GATE`
  in config.h incrementally, reflash, and record the value that stabilizes
  tracking without letting empty-frame noise through.
- If noise/empty frames produce faces=1 even at 152: raise PS_CONF_GATE further.

### 7b. Distance floor + lateral (X-axis) tracking (CG-S5, bench-observed 2026-07-06)

DFRobot's own documented detection range is **0.5–3 m (~19.7 in – ~9.8 ft)**
for both gesture and face recognition (ENGINEERING_LOG.md "External research"). Below
~20 inches is **out-of-spec operation** — instability there is expected, not a
firmware bug. Test at/above 20 inches, not below.

A power issue was also found and fixed this session (2.6V measured at the
sensor's VCC vs. 3.25V at the Teensy's own 3.3V pin — traced to a bad
connector in the VCC run, not the Teensy's regulator; reseating/direct-wiring
fixed it). If you haven't already, confirm the sensor's VCC reads ~3.2-3.3V
before re-running this step, since undervolting an active-inference sensor can
independently produce exactly this kind of instability.

Action: with VCC confirmed healthy and PS_CONF_GATE=152, stand at ~24-36
inches (safely inside the documented range) and move laterally (left/right)
while keeping the same distance. Watch `rawX` and the eye's horizontal
tracking specifically — the operator reported clean vertical (Y) tracking but
unstable lateral (X) tracking, worse the closer they stood.
- If `rawX` is steady while you hold still and only moves when you actually
  move: X tracking is fine at this distance: the problem was distance-related
  (floor violation) and/or the now-fixed power issue, not a firmware/mapping bug.
- If `rawX` still jitters rapidly while you hold still at 24-36": that's a
  genuine sensor-side X-axis noise issue independent of distance and power —
  see archive/11_HANDOFF_FABLE_LATERAL_TRACKING.md for the investigation plan (a
  smoothing/low-pass filter on targetX, modeled on IRIS's own panServo
  `filteredPan` alpha=0.15 pattern, is the leading candidate fix if this turns
  out to be sensor noise rather than a mapping bug).

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

Action: wire the second display (WIRING.md dual-eye table: CS9/DC8/RST6, shared
SCK13/MOSI11), uncomment `#define DUAL_EYE`, reflash.
Expect: BOTH displays show an eye at boot; both track the same face together;
neither is blank or frozen. PASS: two coordinated eyes. FAIL: second display
blank → check CS9/DC8/RST6 wiring and that SCK/MOSI are shared to pins 13/11.
Note: per-eye refresh is ~half single-eye (shared bus) — expected, not a fault.

---

## Issues Found

Resolved this session (2026-07-06), T4.1 connected on COM6:
- SEN0626 initially NOT FOUND — root cause was the sensor's onboard I2C/UART
  DIP switch left in I2C mode (this firmware is UART-only). Fixed by flipping
  the switch; documented in WIRING.md and bench step 3.
- SEN0626 VCC measured 2.6V vs 3.25V at the Teensy's own 3.3V pin — a bad
  connector in the VCC run (not the Teensy regulator, which tested healthy).
  Fixed by reseating/direct-wiring. Undervolting an active-inference sensor is
  a documented SEN0626 forum failure mode (ENGINEERING_LOG.md "External research") and
  plausibly contributed to the narrow detection envelope reported below.
- Confidence gate was tuned against IRIS's own unrelated constant (45), not
  the SEN0626's actual behavior — replaced with DFRobot's documented floor
  (score >=60 → PS_CONF_GATE=152, CG-S5, audit 3.7).

Resolved 2026-07-07 (CG-S6/S7/S8 bench pass, operator live on COM6):
- Lateral (X) tracking: root cause was a MIRROR, not noise — double X-flip
  (main.cpp negation + EyeController eyeIndex==0 flip on a single eye). Fixed
  CG-S6 by removing the main.cpp negation. Bench-VERIFIED correct L/R.
- Gaze range too small: GAZE_GAIN 1.0 → 1.7 (CG-S7), derived from measured rawX
  span 151-427. Bench-VERIFIED.
- Upward-gaze bias / eye pinned up: Y_CENTER=33 (CG-S8) — sensor mounts below
  the eye so eye-level images near frame top; old code used 127.5 as neutral.
  Bench-VERIFIED neutral now ≈0.

Open (non-blocking for IRIS):
- Upward-gaze travel is hardware-limited (~9 rawY counts above neutral) because
  the sensor sits below the eye. Fix is physical (tilt/lower sensor), not code.
- NATIVE_H (480 vs 640): DFRobot's register doc labels face_y "0-640" but the
  extraction was self-inconsistent; not trusted, NATIVE_H left at 480. Confirm
  via peak rawY on a deliberate look-down test if it ever matters.
- LED: no Modbus register exists (confirmed vs DFRobot wiki + library) — cannot
  disable in firmware, physical cover only. enableLED() stays a no-op stub.

## Status: VERIFIED (2026-07-07) — ready for IRIS integration

Tracking is bench-verified end-to-end (COM6, operator live). CyclopsGaze is
cleared as a drop-in Person Sensor (SEN-21231) replacement — unblocks
IRIS_INTEGRATION.md.

- [x] Steps 1-3: enumerate, flash, boot + SEN0626 baud — DONE.
- [x] Step 4: face-detect serial line confirmed (gate=PASS, rawX/rawY logging).
- [x] Step 5: L/R fixed (CG-S6 mirror fix) and U/D confirmed after Y_CENTER.
- [x] Step 7: PS_CONF_GATE=152 confirmed passing on live bench scores (60-74).
- [x] Step 7b: lateral tracking resolved — was a mirror bug, not noise/distance.

Deploy-time (not blockers):
- [ ] Upward gaze: tilt/lower sensor for more above-eye-level headroom (physical).
- [ ] Set CG_CALIB_RAW to 0 for quiet serial once done bench-logging.
- [ ] Step 6: confirm NATIVE_H 480 vs 640 only if Y precision ever matters.

## IRIS integration (CG-S9, 2026-07-07)

Code-reviewed both IRIS consumers of the dead Person Sensor and produced drop-ins.
Details in IRIS_INTEGRATION.md + integration/README.md. Summary:
- T4.1 eyes: `src/sensors/SEN0626Sensor.{h,cpp}` is the class drop-in; method
  surface + struct verified against live IRIS. `isPresent()` now lazy-begins so
  IRIS's probe loop needs no `begin()` edit.
- T4.0 servo: new `integration/servo_teensy40_base_mount/person_sensor.{h,cpp}`
  reimplements the servo's `PersonResult` interface on SEN0626 (Serial1).
- Two operator decisions flagged (NOT auto-applied): confidence scale (IRIS
  psConfGate capped 0-100 vs shim 0-255) and eyes direction/psYBias. Do NOT copy
  CyclopsGaze's CG-S6/CG-S8 tracking edits into IRIS.
- IRIS adapters are REPO-ONLY until an IRIS deploy session flashes/benches them.

## CG-S10 (2026-07-15) — camera module is physically separable from the main PCB

**Bench finding, not yet firmware/software related — a mechanical discovery.** The SEN0626's
camera lens/sensor assembly attaches to the Gravity breakout via a ribbon cable and
double-sided tape, not a fixed/soldered connection.

Bench-tested with the camera sensor **disconnected from the main SEN0626 board** (the
board's head-shoulder indicator LED, gesture RGB LED, and any IR-adjacent component were
not present/connected). Face identification and tracking output functioned correctly with
only the camera sensor + ribbon in place.

**Confirmed NOT required for face detect/track:**
- Head-shoulder indicator LED (white)
- Gesture indicator LED (RGB)
- Any IR-associated component mounted alongside the lens on the main board

**Why this matters:** the full Gravity board footprint has been the physical objection to
SEN0626 as a Person Sensor (SEN-21231) drop-in — the original is tiny, the Gravity breakout
is not. If the camera + ribbon alone is sufficient, SEN0626 can be mounted in a footprint
closer to the original sensor's, without carrying the rest of the board. This strengthens
both of CyclopsGaze's reasons for existing (§ Purpose above): insurance if IRIS's live
Person Sensor ever fails, and the public-launch replicability recipe (IRIS_INTEGRATION.md §8).

**Open before calling this VERIFIED:** confirm the detached camera sensor still reports
through the **same I2C/UART register set, same device address, same baud rate** once
separated from the main board. If any board-side circuitry (beyond the LEDs/IR components
already ruled out above) was doing signal conditioning for the camera link, register reads
could differ once relocated. Test raw register output side-by-side (attached vs. detached)
before finalizing the swap — same protocol as the existing `CG_CALIB_RAW` bench logging
(§ SEN0626 Protocol above).

**Status: REPO-ONLY bench finding — not yet flashed or integrated into firmware.**
