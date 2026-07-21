<!-- Completed history only. Append new entries at the bottom. Forward-looking work + current bench status live in docs/ENGINEERING_LOG.md. -->

# CyclopsGaze Changelog

---

## CG-S1 (2026-07-03) — initial testbed

Standalone single-eye face-tracking testbed created. SEN0626 Modbus RTU (FC04 input
registers, device 0x72) driver + single GC9A01A 240×240 eye tracking on Teensy 4.0.
`SEN0626Sensor` shim designed to match the Useful Sensors Person Sensor (SEN-21231) interface
used by IRIS. Baud auto-detect (115200 → 9600). Files: `src/` (CyclopsGaze.ino, config.h,
sensors/, displays/, eyes/), `platformio.ini`, doc scaffolding (`00`–`07`, README, NOTES,
WIRING).

Follow-up fix (b84033d): gaze tracking corrected — store face center in both box edges (was a
fake cx±32 box that biased the recovered center near frame edges), remove the `/3.0f` bias,
add auto-move timeout.

## CG-S2 (2026-07-04) — Teensy 4.1 target + drop-in contract validated

- Migrated to Teensy 4.1 (T4.1 arrived 2026-07-03). Only `board = teensy41` in
  `platformio.ini` changed; wiring/pins identical to T4.0 (docs/WIRING.md).
- Clean build: SUCCESS 8.33s (FLASH code 74012 / data 361032; RAM1 free 414912; RAM2 free
  511872). No link warnings.
- Drop-in contract validated: `SEN0626Sensor`'s `person_sensor_face_t` matches IRIS
  `src/sensors/PersonSensor.h` byte-for-byte, and the public method surface matches
  (`isPresent/begin/read/enableID/setMode/enableLED/numFacesFound/faceDetails/
  timeSinceFaceDetectedMs`). Confirmed true drop-in.
- Status: REPO-ONLY — not flashed/bench-verified (no T4.1 enumerated on SuperMaster this
  session).
- Framed as future-proofing / public-launch replicability, not a fix for a current fault —
  live IRIS Person Sensor works fine. Bench-only on the spare non-installed T4.1; live PS
  stays source of truth until VERIFIED.

## CG-S3 (2026-07-04) — durable gaze-node repo + IRIS plan, then firmware audit + dual-eye

CG-S3 spans two work items on the same day: first the docs/architecture pass (IRIS HANDOFF C,
below), then the firmware audit + dual-eye pass (Opus handoff, further below).
`FIRMWARE_VERSION` is bumped CG-S2 → CG-S3 for the code work.

### Part A — durable gaze-node repo; IRIS integration plan (docs only)

- CyclopsGaze is now the durable gaze-node identity. The old ESP32-S3 + OV2640 approach
  (`../OGLE`) was tombstoned/archived — it never reached reliable end-to-end face tracking at
  IRIS's real bench distance/lighting; research favors T4.1 + SEN0626. Decision made with the
  operator.
- Integration path chosen: native Person-Sensor-bus drop-in (SEN0626 read directly by IRIS's
  own T4.1, standing in the dead PS slot), not OGLE's separate-node USB-CDC → Pi4
  `ogle_bridge.py` → UDP `GAZE:` bridge. Simplest to replicate publicly; keeps gaze on the
  Teensy's local loop.
- Added `docs/archive/09_IRIS_INTEGRATION_PLAN.md` — the full PS-bus drop-in plan for a later
  IRIS deploy session: the I2C→UART bus change, the enumerated IRIS-side edits
  (`main.cpp:14/116`, begin/re-probe guard, config), the behavioral-gap handling (`is_facing`
  non-blocking because IRIS `psFacingRequired` defaults false since S153c; confidence-scale
  bench-verify vs operator CONF=60; coordinate/flip verify; LED no-op), the bench-verify gate,
  and staged rollout + rollback.
- README refreshed to the T4.1 identity and to point at the integration plan.
- No IRIS deploy, no flash. CyclopsGaze remains REPO-ONLY pending the
  docs/ENGINEERING_LOG.md bench-verify checklist.

### Part B — firmware audit + dual-eye (Opus handoff)

Full audit of the tracking chain against live code and the IRIS reference; fixes landed;
optional dual-eye added. Clean build (single-eye and `-DDUAL_EYE`), still REPO-ONLY (no T4.1
enumerated to flash). Full verdicts in docs/ENGINEERING_LOG.md §"CG-S3 Audit".

- Y-direction: confirmed correct, not inverted. The sign convention (targetX negated, targetY
  not) is byte-for-byte IRIS's production `main.cpp:594-595` on the identical EyeController —
  field-proven. The peer review's "Y inverted" hypothesis is rejected on the firmware side;
  any residual is the SEN0626 Y-axis orientation, a one-line bench flip if needed (NOTES bench
  step 5).
- autoMove freeze: verified fixed. The resume check runs as an `else if` for every no-face
  path (throttle, comms failure, sub-gate), so it can't stick off.
- Modbus timeout 300 → 100ms (`MODBUS_RESP_TIMEOUT_MS`): caps a stalled-read eye freeze at
  100ms while keeping ~4x margin over the ~25ms normal round trip.
- Baud detect hardened: 2s power-on settle floor before first probe (AI-model boot), 200ms
  per-baud line settle, and 3 full-sweep retries (`BOOT_SETTLE_MS`, `BAUD_ATTEMPTS`).
- Confidence gate added to the consumer loop (`PS_CONF_GATE`, default 45 to match IRIS
  S153c) — previously CyclopsGaze tracked any detection ungated. Effective min SEN0626 score
  ≈19/100; bench-tunable.
- NATIVE_H calibration aid: shim now exposes rawFaceX/Y/Score; main logs them under
  `#define CG_CALIB_RAW 1` so the operator can confirm 480 vs 640 from serial.
- GAZE_GAIN knob (default 1.0) for bench range-tuning; range verified to match IRIS
  (mapRadius=240, r≈77.3). Center-recovery and 4:3-vs-1:1 mapping confirmed correct (no drift;
  per-axis normalisation is intentional).
- Dual-eye (optional): `#define DUAL_EYE` in config.h drives a second GC9A01A. Resolved a T4.1
  pin conflict — IRIS's second bus (SPI1) collides with Serial1 on both pin 0 (SPI1 CS) and
  pin 1 (SPI1 MISO), so both eyes share SPI0 with separate CS (eye1: CS9/DC8/RST6).
  docs/WIRING.md gains a full dual-eye section.
- Tunables (PS_CONF_GATE, GAZE_GAIN, FACE_LOST_MS, CG_CALIB_RAW) centralised in config.h.
  docs/ENGINEERING_LOG.md gains the full 10-step first-flash bench protocol.

## CG-S4 (2026-07-06) — first real bench pass: DIP-switch fix, ungated calibration logging

First session with the T4.1 actually connected and flashed (COM6). Status moved from
REPO-ONLY to DEPLOYED (bench-verified in part; see docs/ENGINEERING_LOG.md for what's still
open).

- SEN0626 not found, root-caused: the sensor's onboard I2C/UART mode DIP switch was left on
  I2C. This firmware is UART-only with no fallback. Fixed by flipping the switch — no code
  change. Documented in docs/WIRING.md and the bench protocol so it's checked first next
  time, before re-wiring.
- Calibration logging ungated: CG-S3's raw/diagnostic serial line only printed inside the
  confidence-gated branch, so it vanished the instant a detection dropped below
  `PS_CONF_GATE` — exactly the boundary data needed for range/gate tuning. `main.cpp` now
  logs every raw detection (`rawScore`/`conf`/`gate=PASS|REJECT`/`rawX`/`rawY`) regardless of
  gate outcome; the eye still only moves on a `PASS`.
- FIRMWARE_VERSION CG-S3 → CG-S4.

## CG-S5 (2026-07-06) — power fault fixed, confidence gate re-derived from vendor spec, distance floor documented

Same bench session, continued. Bench testing surfaced a narrow detection envelope (1-2ft /
±10-20°) and unstable lateral tracking; before more firmware tuning, traced both to real,
sourced causes.

- Power fault found and fixed: SEN0626 VCC measured 2.6V under load vs. 3.25V at the Teensy's
  own 3.3V pin — the drop was in a bad connector along the run, not the Teensy regulator.
  Undervolting an active-inference sensor like this is a documented SEN0626 forum failure
  mode (random resets, frozen output, degraded detection). Fixed by reseating/direct-wiring;
  documented in docs/WIRING.md as a first-check item.
- PS_CONF_GATE re-derived from DFRobot's own spec, not IRIS's: CG-S3's default (45) was
  borrowed from IRIS's unrelated `psConfGate` constant and mapped to an effective SEN0626
  score floor of only ~19/100. DFRobot's own setup guide states "a score >=60 is considered
  valid" and its sample code sets exactly that threshold. New default: `PS_CONF_GATE = 152`
  (`floor(60*255/100)-1`), so raw score 60 passes and 59 doesn't.
- Detection-range floor documented: DFRobot specs 0.5-3m (~19.7in-9.8ft) for both gesture and
  face recognition. The operator's bench observation of instability below ~15in is below this
  documented floor — out-of-spec operation, not a bug. Added to docs/WIRING.md (mounting
  guidance) and docs/ENGINEERING_LOG.md bench step 7b.
- Person Sensor comparison — honest gap flagged, not fabricated: no public FOV-in-degrees or
  minimum-working-distance spec was found for the Person Sensor to do an exact numeric
  comparison. Qualitatively, IRIS's own production history with the Person Sensor has never
  surfaced a "must stand back" complaint, while the SEN0626's 0.5m floor sits at typical close
  conversational distance — flagged in docs/archive/09_IRIS_INTEGRATION_PLAN.md §6 as a real
  consideration for the deploy decision, not just a tuning task.
- Open, handed off: lateral (X-axis) tracking instability at close range needs re-testing now
  that power and the gate are fixed, before concluding anything further is wrong. See
  `docs/archive/11_HANDOFF_FABLE_LATERAL_TRACKING.md`.
- FIRMWARE_VERSION CG-S4 → CG-S5.
- Status: DEPLOYED, not yet VERIFIED — sensor detection, power, and gate are bench-confirmed;
  direction (3.1), NATIVE_H (3.5/step 6), and lateral tracking are still open per
  docs/ENGINEERING_LOG.md.

## CG-S6/S7/S8 (2026-07-07) — tracking VERIFIED on bench; ready for IRIS integration

Full bench pass with T4.1 on COM6, operator live. Lateral-tracking investigation closed and
gaze tuned end-to-end. Status: VERIFIED.

- CG-S6 — lateral direction fixed. X was mirrored: `main.cpp` negated `targetX` for the IRIS
  "eye-0 of a pair" mirror convention, but `EyeController::renderFrame()` also unconditionally
  flips `eye.x` for `eyeIndex==0`, and CyclopsGaze's single eye is always index 0 — so the
  pair-mirror was applied with no pair to mirror against. Removed the negation in `main.cpp`
  (not the shared EyeController). Bench-confirmed correct L/R.
- CG-S7 — GAZE_GAIN 1.0 → 1.7. Bench data showed a natural side-to-side head movement only
  spanned rawX 151-427 (native 0-640), so targetX topped out near ±0.5. Raised gain from that
  measured ratio; X and downward Y now reach near full travel. Bench-confirmed.
- CG-S8 — Y_CENTER offset added. Sensor is mounted physically below the eye, so a face at
  true eye-level images near the frame top (bench-measured cy≈33, not the geometric center
  127.5). Old code treated 127.5 as neutral, so normal posture computed to y≈-1.27 (past the
  clamp) — eye pinned looking up. `Y_CENTER=33` recenters neutral to y≈0. Bench-confirmed:
  neutral now reads ~0, downward range good.
- Upward-gaze ceiling = hardware limit, not a bug. With neutral correctly at y≈0, deliberate
  max-up-gaze only moved rawY to ~45 (≈9 counts above neutral) — the sensor's below-eye
  mounting leaves almost no frame above eye-level. Not chased with asymmetric gain (would
  amplify quantization noise on the one axis already worst-off). Fix is physical: tilt/lower
  the sensor. Documented.
- LED not disableable in firmware — confirmed against source. DFRobot's own Modbus register
  map (wiki docs/23023) and their `DFRobot_GestureFaceDetection` library expose no LED
  register — only device addr, baud, and the detection/score thresholds. `enableLED(bool){}`
  stays a no-op stub. The white LED = onboard face-presence indicator (independent of
  PS_CONF_GATE); the brief blue = RGB gesture indicator (thumbs-up), incidental. To kill the
  LED for a build: physical cover only.
- FIRMWARE_VERSION CG-S5 → CG-S8.
- Status: VERIFIED — ready for IRIS integration as a drop-in Person Sensor (SEN-21231)
  replacement. Direction, gain, and Y-center all bench-confirmed. Remaining knobs (upward-gaze
  mount tilt, optional CG_CALIB_RAW=0 for quiet serial) are deploy-time, not blockers. Unblocks
  the IRIS integration (docs/IRIS_INTEGRATION.md).

## CG-S9 (2026-07-07) — IRIS integration: code-review pass + both-consumer drop-ins

Code-reviewed the drop-in against IRIS's actual source (read-only) and closed the gaps. No
CyclopsGaze tracking behavior changed — tracking stays VERIFIED (CG-S8). Adapters are
REPO-ONLY for IRIS until a deploy session flashes them.

- Found a second consumer the eyes-only plan missed. The dead Person Sensor drove two nodes:
  T4.1 eyes (`PersonSensor` class, X+Y) and the T4.0 servo head-pan node
  (`setupPersonSensor()`/`pollPersonSensor()` → `PersonResult`, X-only). The class shim only
  covers the eyes.
- New servo adapter: `integration/servo_teensy40_base_mount/person_sensor.{h,cpp}` —
  SEN0626-backed reimplementation of the servo's `PersonResult` contract (Serial1,
  `faceCenterX=(box_left+box_right)/2` in 0-255, conf gate 152 = DFRobot score≥60, facing
  always-true). The servo `.ino` is unchanged.
- Eyes method-surface audit: IRIS calls only
  `isPresent/read/setMode/enableID/enableLED/faceDetails/numFacesFound/timeSinceFaceDetectedMs`
  — all present; the shim's missing `singleShot/labelNextID/persistIDs/eraseIDs` are never
  called. Struct byte-identical.
- Closed the `begin()` gap: the real `PersonSensor` has no `begin()`; IRIS probes via
  `isPresent()`. `SEN0626Sensor::isPresent()` now lazily runs `begin()` (UART bring-up + baud
  detect) so IRIS's probe loop works with no added call. Standalone unaffected (main.cpp calls
  begin() explicitly).
- Two IRIS-side warnings documented, not baked in: (1) do not propagate CG-S6 targetX
  negation-removal or CG-S8 `Y_CENTER` to IRIS — IRIS keeps its own negation + runtime
  `psYBias`; (2) confidence-scale mismatch — IRIS `psConfGate` is capped 0-100 but
  `box_confidence` is 0-255, so the eyes gate is currently inert; recommended fix (emit raw
  0-100) flagged as an operator decision requiring a CyclopsGaze re-verify.
- Added `integration/README.md`; expanded `docs/archive/09_IRIS_INTEGRATION_PLAN.md` (§2
  audit, §4 warnings, new §4b servo, §5 confidence math, §6 gate → IRIS-side).
- FIRMWARE_VERSION CG-S8 → CG-S9 (drop-in-only `isPresent()` change; tracking untouched →
  still VERIFIED).

## CG-S10 (2026-07-15) — camera module bench-confirmed physically separable from main PCB

Mechanical bench finding, no firmware change. The SEN0626's lens/sensor assembly attaches to
the Gravity board via ribbon cable + tape, not solder — bench-tested with the camera
disconnected from the main board (head-shoulder LED, gesture RGB LED, IR-adjacent components
all absent) and face detect/track still worked through the ribbon alone. Closes the
mounting-footprint gap that made SEN0626 bulkier than the tiny Person Sensor it's replacing,
for both the IRIS-insurance and public-replicability reasons this repo exists. Open before
VERIFIED: confirm the detached camera reports the same I2C/UART register set / device address
/ baud once separated — an unruled-out board-side signal-conditioning dependency could change
register reads post-separation. Detail: `docs/ENGINEERING_LOG.md` CG-S10,
`docs/IRIS_INTEGRATION.md` §separable-camera. Status: REPO-ONLY, not flashed or integrated.

## CG-S11 (2026-07-17) — the drop-in went live in IRIS, and refined there

The SEN0626 driver validated here was actually swapped into the parent IRIS project for the
dead Useful Sensors Person Sensor (IRIS sessions S212 / S212b / S212c). This is the outcome
the whole repo was building toward, and the real swap surfaced refinements — captured here so
CyclopsGaze and IRIS don't drift. (All work below happened in the private IRIS repo;
CyclopsGaze source was synced to match in CG-S12.)

- Deliberate driver divergence — confidence scale. IRIS gates on a 0–100 `psConfGate` knob,
  so the shim's original `box_confidence = score*255/100` (0–255) made every real detection
  (raw 60–90 → 153–229) clear any reachable gate, silently rendering the operator's confidence
  knob inert. IRIS emits the raw DFRobot score (0–100) instead, so the gate means what
  DFRobot documents: a score ≥ 60 is a valid face. Same effective threshold as CyclopsGaze's
  152/255 (≈0.60), just the vendor's native scale.
- Integration bug found + fixed (S212b) — center-only box vs an area presence test. IRIS's
  face-selection loop used box area as an "is there a face?" test (`size > maxSize`, `maxSize`
  starting at 0). The SEN0626 reports a face center, so the shim stores `box_left==box_right`
  and `box_top==box_bottom` → area is always 0, `0 > 0` is false, and every face was silently
  discarded before both the report and the gaze update. Sensor detected, zero tracking. Fixed
  on the IRIS side (area is a ranking key, not a presence test). CyclopsGaze's own consumer
  never had this bug — it reads the center directly — but it's the canonical gotcha for
  anyone adapting a center-only sensor into a box-oriented consumer, so it's documented in
  `docs/IRIS_INTEGRATION.md`. After the fix: live tracking, FACE:1 ×9 in 10 min.
- Gaze shaping generalized (S212c) — per-axis signed gain + bias. IRIS previously had a
  single gaze tunable. The real bench (18–24in) showed L/R mirrored and only ~15° total
  travel. Root causes: (1) mirror = a direction issue, fixed by the gain's sign; (2) ~15° is
  not a bug — at 20in from an 85° FOV a ±6in head move only crosses ~40% of frame ≈ 0.4
  deflection, fixed by the gain's magnitude. New model `targetN = rawN * gain + bias` per
  axis; the X-gain default is transport-conditional (+1.0 for SEN0626, −1.0 for the Person
  Sensor rollback path).
- Status: SEN0626 is DEPLOYED + tracking live in IRIS. CyclopsGaze standalone tracking remains
  bench-VERIFIED as of CG-S8 (unchanged by the IRIS work itself).

## CG-S12 (2026-07-18) — sync refinements back to CyclopsGaze + public-release reorganization

Two things: (a) bring the IRIS-proven refinements back into this repo's own source, and (b)
restructure the repo for a public GitHub release.

Code sync (unverified on the standalone bench — re-bench is the #1 priority next flash):
- `SEN0626Sensor.cpp` now emits the raw DFRobot score (0–100) as `box_confidence`, matching
  the IRIS divergence (was `score*255/100`).
- `PS_CONF_GATE` 152 → 60 and the servo adapter's `PS_SERVO_CONF_GATE` 152 → 60 to stay on the
  raw 0–100 scale. Same effective vendor-floor threshold (score ≥ 60), just the native scale.
- `main.cpp` gaze math replaced the single `GAZE_GAIN` + `Y_CENTER` with IRIS's per-axis
  signed gain + bias (`GAZE_X_GAIN`/`GAZE_Y_GAIN`/`GAZE_X_BIAS`/`GAZE_Y_BIAS`). The defaults
  are algebraically identical to the bench-VERIFIED CG-S6/S7/S8 behavior (X un-mirrored at
  +1.7, Y range 1.7, `Y_BIAS`=1.26 reproduces the CG-S8 `Y_CENTER`=33 below-eye-mount
  compensation).
- `FIRMWARE_VERSION` CG-S9 → CG-S12. Build compiles clean (Teensy 4.1, single-eye and
  dual-eye); runtime tracking not yet re-observed — treat standalone as DEPLOYED-pending-verify.

Repo reorganization (public release):
- Added LICENSE (MIT, with bundled TeensyEyes / Chris Miller attribution), CLAUDE.md (session
  anchor), RULES.md (engineering discipline), and rewrote README.md to frame the project
  accurately for a public audience (standalone gaze demo + IRIS drop-in module).
- Curated the flat pile of numbered session-handoff docs into a clean `docs/` tree (`WIRING`,
  `SEN0626_PROTOCOL`, `BENCH_PROTOCOL`, `IRIS_INTEGRATION`, `ENGINEERING_LOG`, `ATTRIBUTION`);
  moved the raw historical handoffs to `docs/archive/`; added `docs/media/` for photos.

## CG-S13 (2026-07-21) — second sync from live IRIS: transport knob + PS_CFG live tuning

Session goal was "bring IRIS's refined firmware back". Diffing both repos and reading the
running Pi4 first showed most of it was already here, which changed what the work actually was.

Already in sync (measured, not assumed):
- `SEN0626Sensor.{h,cpp}` differ from IRIS's copies by comment text only — zero code
  difference.
- IRIS's gaze code is frozen at S212c. Its last `main.cpp`/`config.h` commit is S213
  (2026-07-18) and changed only `PROTOCOL_VERSION`. CG-S12 had already synced the S212c model,
  so there was no un-synced IRIS code change to transfer.

What actually came back:
- `SEN0626_SERIAL` transport knob (`src/config.h`). Live IRIS runs the sensor on `Serial4`
  (RX 16 / TX 17), not `Serial1` — pin 0 is IRIS's left-eye CS. Two docs here asserted
  `Serial1`; both were wrong and are corrected. Default stays `Serial1` (this rig's
  bench-VERIFIED wiring).
- The `PS_CFG:` runtime-tuning protocol (IRIS S141 + S212c) — the real polish. `CONF`,
  `FACING`, `LOST_MS`, `X_GAIN`, `Y_GAIN`, `X_BIAS`, `Y_BIAS`, `LED` are now runtime variables
  retunable over serial with no reflash. Parser, key set, `[DBG] PS_CFG KEY=value` ack wording
  and the S212c false-ack guard are IRIS-verbatim, and the `ps*` variable names match IRIS's
  exactly so the two `main.cpp` files stay diffable. Makes the outstanding CG-S12 re-bench a
  tune-live exercise instead of edit-reflash-repeat. Adds `PS_CFG?` (readback) — needed only
  here, since IRIS reads live values back from the Pi4's `ps_config.json`; values are RAM-only
  on this board.
- Facing gate (`psFacingRequired`, default false) for surface parity; inert on SEN0626.
- Gaze math is now IRIS's verbatim expression (box-derived center, incl. the `/3` term). Zero
  behavior change — the center-only box makes both delta terms 0. Algebra, not bench.
- `config.h` tunables became `*_DEFAULT` seeds for the runtime variables.

Deliberately not adopted — IRIS's live values. Observed on the wire 2026-07-21 (firmware
S213): `CONF=25 LOST_MS=8500 X_GAIN=1.0 Y_GAIN=1.0 X_BIAS=0.0 Y_BIAS=0.0`, and IRIS is
tracking with them (79 `FACE:1` that day). But they are not tuned: `/home/pi/ps_config.json`
predates the SEN0626 swap (dated 07-15 vs swap 07-16) and carries no gain/bias keys at all, so
those fall back to compile-time defaults; `CONF=25` is Person-Sensor-era 0–255-scale carryover
flagged in IRIS's own S212 comment as an unmade decision. CyclopsGaze keeps its measured
CG-S7/CG-S8 values. Open/owed: a head-to-head behavioral comparison of the two value sets —
now a cheap `PS_CFG:` test. See docs/ENGINEERING_LOG.md CG-S13 and docs/IRIS_INTEGRATION.md §
"The tuning-value gap".

- `FIRMWARE_VERSION` CG-S12 → CG-S13. Both builds clean (single-eye + `-DDUAL_EYE`).
- Status: REPO-ONLY — nothing flashed, nothing observed running. No Teensy enumerated this
  session (`pio device list`: COM1 legacy + COM4/5 Bluetooth only). The PS_CFG parser, facing
  gate and IRIS-form gaze math are unverified on hardware, on top of CG-S12's unverified gate
  + gain/bias. Re-running docs/BENCH_PROTOCOL.md remains the #1 priority.
