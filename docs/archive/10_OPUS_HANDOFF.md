# CyclopsGaze — Opus Planning & Implementation Handoff

**Model target:** Claude Opus or highest-capability model available
**Repo:** `C:\Users\SuperMaster\Documents\PlatformIO\CyclopsGaze`
**IRIS ref (read-only):** `C:\Users\SuperMaster\Documents\PlatformIO\IRIS-Robot-Face`

---

## 0. Creative License — Read This First

You are not here to rubber-stamp existing code. You are a senior embedded firmware
engineer with full authority to make this excellent. The codebase is small, the
hardware is fixed, and the goal is clear: a Teensy 4.1 that tracks faces and moves
an animated eye on a round display, reliably, with a sensor nobody else has driven
this way before.

That means:

- If the coordinate mapping is wrong, rewrite it correctly.
- If the Modbus timing is fragile, harden it.
- If the main loop architecture has a latent bug, fix the architecture.
- If there is a smarter way to derive gaze direction from what the SEN0626 actually
  returns versus what the fake-bounding-box shim pretends it returns, implement that.
- If the dual-eye config as spec'd has a problem on the T4.1 SPI bus, say so and
  propose the right solution.
- If anything in the existing peer review (07_PEER_REVIEW_HANDOFF.md) was wrong or
  incomplete, correct it.

You are not constrained to the existing file structure if a reorganization would be
cleaner. You are not constrained to the existing shim approach if a better one exists.
The only hard constraints are: Teensy 4.1, SEN0626 over UART1 Modbus RTU, GC9A01A
240x240 round display(s), PlatformIO, and the drop-in interface contract with IRIS
(person_sensor_face_t layout and method signatures must remain identical).

Everything else is on the table. Make it good.

---

## 1. Mission

CyclopsGaze is a Teensy 4.1 + DFRobot SEN0626 (Modbus RTU over UART1) + GC9A01A
240x240 round display gaze tracker. Its SEN0626Sensor shim is a byte-for-byte
drop-in replacement for the discontinued Useful Sensors Person Sensor (SEN-21231)
used in IRIS-Robot-Face. Firmware is at CG-S2: builds clean, never flashed.

Three jobs, in order:

1. **Audit and fix** -- find every bug and design weakness, fix them completely.
2. **Bench protocol** -- produce the exact verification sequence for first flash.
3. **Dual-eye expansion** -- add optional second GC9A01A support in config.h.

---

## 2. Read Order (do not skip, do not assume)

```
01_CYCLOPSGAZE_RULES.md
NOTES.md
CHANGELOG.md
07_PEER_REVIEW_HANDOFF.md
09_IRIS_INTEGRATION_PLAN.md

src/sensors/SEN0626Sensor.h
src/sensors/SEN0626Sensor.cpp
src/main.cpp
src/config.h
src/eyes/EyeController.h
src/eyes/eyes.h
src/displays/GC9A01A_Display.h
src/displays/GC9A01A_Display.cpp
src/displays/Display.h
src/eyes/240x240/nordicBlue.h
platformio.ini

IRIS-Robot-Face/src/main.cpp          (read-only -- extract setTargetPosition call)
IRIS-Robot-Face/src/sensors/PersonSensor.h  (read-only -- drop-in contract)
TeensyEyes-PersonSensor-Teensy4.0-RoundDisplay/src/config.h  (dual-eye pin reference)
```

---

## 3. Audit

For each item: find the truth, state it plainly, fix it if broken.
Produce complete corrected files -- no diffs, no stubs, no partial listings.

### 3.1 Y-direction

The peer review flagged Y as likely inverted: face at top of frame drives targetY
negative, which without Y-mirroring in renderFrame should push the eye down, not up.

Trace the full chain:
SEN0626 faceY -> cy remap -> face.box_top -> main.cpp targetY formula ->
setTargetPosition -> state.eyeNewY -> renderFrame (X mirrored for eyeIndex==0,
Y not mirrored) -> yPositionOverMap -> renderEye polar map sampling -> iris
direction on display.

Cross-check against IRIS main.cpp: does IRIS negate targetY? If IRIS works correctly
without negation, what does that tell you about how the polar map is oriented?

Give a definitive verdict. If it is wrong, fix it and explain why.

### 3.2 autoMove freeze

Prior bug: timeSinceFaceDetectedMs() check was gated inside sensor.read() success,
so autoMove could get stuck OFF when the sensor stopped responding.

CG-S2 restructured the loop. Verify the fix is complete: confirm the timeout check
and setAutoMove(true) execute when sensor.read() returns false due to throttle or
comms failure. If there is still a path where autoMove stays stuck, fix it.

### 3.3 Center-only bounding box -- IRIS compatibility

The shim stores face center in all four box edges:
box_left == box_right == cx, box_top == box_bottom == cy.

IRIS main.cpp computes target from:
  targetX using (box_left + (box_right - box_left) / 2.0f)
  targetY using (box_top + (box_bottom - box_top) / 3.0f)

With identical edges, both box-span terms collapse to zero. Verify the recovered
center matches the original center exactly at all values including 0 and 255.
Confirm no rounding or integer truncation introduces drift.

Then ask: is center-only the best approach, or is there useful information in the
SEN0626 output that this approach throws away? The SEN0626 registers include
face_score and face_number. Are there any other registers in the DFRobot library
that provide bounding dimensions rather than just center? Check
`DFRobot_GestureFaceDetection` register definitions if accessible via the repo or
NOTES.md. If better data is available, use it.

### 3.4 Coordinate mapping -- aspect ratio mismatch

The SEN0626 field of view is 640x480 (4:3). The display is 240x240 (1:1).
The current mapping stretches both axes independently to 0-255:
  cx = faceX * 255 / 640
  cy = faceY * 255 / 480

A face moving the same angular distance horizontally and vertically in the real world
produces different-sized coordinate deltas in each axis after remapping. This makes
the eye track with different sensitivity horizontally vs vertically.

Decide: is this acceptable (the eye is roughly calibrated to the room, not
pixel-perfect), or does it produce a noticeable tracking artifact that should be
corrected with a uniform scale factor? If correction is needed, implement it.

### 3.5 NATIVE_H = 480 -- unconfirmed

NOTES.md states the SEN0626 Y resolution is assumed 480, not confirmed. The only
documented coordinate range is X: 0-640.

Design the bench test to definitively detect this (see section 5). In the meantime,
consider whether the firmware should log raw faceY alongside the remapped value so
the operator can read the maximum observed without a scope. If not currently logged,
add it to the boot-face-detect serial output and flag it as a one-time calibration
diagnostic that can be compiled out later.

### 3.6 Modbus response timeout during eye animation

At 9600 baud, normal round-trip is ~22ms. The timeout is 300ms. If the sensor
stalls mid-response, the firmware blocks for 300ms before declaring failure. During
that block, renderFrame() does not run and the eye freezes.

How often could this realistically happen? What is the worst-case eye freeze duration
in normal operation? If 300ms is too long, what is the minimum safe timeout at 9600
baud given the SEN0626's response characteristics (per NOTES.md)? Implement the
tighter value if warranted, or explain why 300ms is acceptable.

### 3.7 Confidence gate calibration

The shim maps SEN0626 score (0-100) to box_confidence (0-255) via score * 255 / 100.
IRIS gates on box_confidence > 45 (psConfGate default).

The real Person Sensor returned box_confidence on 0-255. The gate was calibrated
against that scale. Confirm the shim's rescaling preserves the gate semantics: a
SEN0626 score of 18 maps to ~46, which clears the gate. A score of 17 maps to ~43,
which does not.

Is a score of 18/100 a reasonable minimum detection threshold for the SEN0626, or
does the SEN0626 regularly report scores in the 15-25 range for a clear frontal face
at 1m? If the sensor scores lower than the real PS for equivalent detections,
the gate may need lowering. Flag this as a bench calibration task with explicit
instructions for what to observe and how to adjust.

### 3.8 nordicBlue polar.mapRadius and tracking range

Read nordicBlue.h. Extract polar.mapRadius. Compute the actual gaze range r:
  r = (mapRadius * 2 - screenWidth * PI/2) * 0.75

At the maximum face position (cx=255 or 0 in 0-255 space), the eye hits
setTargetPosition with xTarget = +/-1. Compute eyeNewX at that extreme. Does the
eye physically reach the edge of the iris's visible travel range, or does the
display crop the iris before it reaches center? If the tracking range is too wide
or too narrow relative to the sensor's field of view, correct the scaling in
setTargetPosition or add a configurable scale factor in config.h.

### 3.9 baud auto-detect robustness

tryBaud() opens the serial port, waits 500ms, sends a Modbus PID read, and waits
300ms for a response. Total worst-case per baud: ~800ms. With two bauds tried:
~1.6s before begin() returns false.

Is 500ms settle time before the first query sufficient for the SEN0626 to be ready
after a cold start? NOTES.md says the device enumerates but give it scrutiny: the
SEN0626 boots an AI model into RAM. Could it take longer than 500ms to answer
Modbus? If so, what is the correct minimum delay? Find any mention of boot time in
the DFRobot documentation or NOTES.md history.

### 3.10 Anything else

Read every file. If you find anything not covered above that is wrong, fragile, or
could be better, fix it and document what you changed and why.

---

## 4. Dual-Eye Expansion

After audit is complete, add dual-eye support as a compile-time option.

Requirements:
- No #define needed for single-eye -- existing default behavior unchanged.
- #define DUAL_EYE in config.h enables the second display.
- Read TeensyEyes-PersonSensor-Teensy4.0-RoundDisplay/src/config.h for the
  reference two-display pin assignments. Use those as the starting point, then
  verify they do not conflict with the SEN0626 UART1 pins (Serial1 RX=0, TX=1)
  already in use. If there is a conflict, resolve it and document the resolution.
- EyeController<2, GC9A01A_Display> when DUAL_EYE, <1, ...> when single.
- Second display mirrored on X.
- main.cpp works unchanged for both.
- 05_WIRING.md gets a complete "Dual-Eye (optional)" section with pin table.

Produce: complete updated src/config.h and complete updated 05_WIRING.md.

---

## 5. Bench Verification Protocol

Produce a numbered bench procedure for first flash. Make it unambiguous -- an
operator who has never used PlatformIO can follow it with a copy of this document.

Cover in order:
1. USB enumeration check (pio device list)
2. Flash and confirm boot message
3. SEN0626 detect -- baud confirmed, PID confirmed
4. Face detect -- serial output format, what numbers mean
5. Direction verification -- face left/right/up/down, expected eye response (use
   your Y-direction verdict from 3.1 to state ground truth)
6. NATIVE_H calibration -- exact procedure to confirm 480 vs 640 from serial alone
7. Confidence calibration -- what conf values to expect at 1m, how to adjust gate
8. AutoMove resume -- how to confirm the 3s timeout is working
9. Edge tracking -- face at extreme corners, confirm no freeze or drift
10. Dual-eye addendum (if DUAL_EYE built) -- both displays initialize, both track

Each step: action, expected serial output in a code block, pass/fail criterion.

This protocol replaces the placeholder in NOTES.md section "Flash & Verify".
Produce the complete updated NOTES.md.

---

## 6. IRIS Integration Gate

Read 09_IRIS_INTEGRATION_PLAN.md section 6. Update the gate checklist to reflect
what this session's code fixes have unblocked. Produce the complete updated file.

---

## 7. Session Close

1. Run `pio run -e cyclopsgaze`. Show full output. Build must be clean.
2. Bump FIRMWARE_VERSION to CG-S3 in src/config.h if any code changed.
3. Commit each logical change separately with a message that states what and why.
4. NOTES.md updated with all findings, including non-bugs confirmed as such.
5. CHANGELOG.md CG-S3 entry if code changed.
6. State final status: REPO-ONLY until operator physically flashes.

---

## 8. Hard Constraints

- 01_CYCLOPSGAZE_RULES.md governs this session. Read it.
- Read every file before editing it.
- Do not touch IRIS-Robot-Face repo.
- Drop-in interface contract (person_sensor_face_t layout + method signatures)
  is inviolable -- everything else can change.
- REPO-ONLY until physically flashed and serial output confirmed.
