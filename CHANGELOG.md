<!-- Completed history only. Append new entries at the bottom. Forward-looking work + current bench status live in docs/ENGINEERING_LOG.md. -->

# CyclopsGaze Changelog

---

## CG-S1 (2026-07-03) - initial testbed

Standalone single-eye face-tracking testbed created. SEN0626 Modbus RTU (FC04 input
registers, device 0x72) driver + single GC9A01A 240×240 eye tracking on Teensy 4.0.
`SEN0626Sensor` shim designed to match the Useful Sensors Person Sensor (SEN-21231) interface
used by IRIS. Baud auto-detect (115200 → 9600). Files: `src/` (CyclopsGaze.ino, config.h,
sensors/, displays/, eyes/), `platformio.ini`, doc scaffolding (`00` through `07`, README,
NOTES, WIRING).

Follow-up fix (b84033d): gaze tracking corrected. Store face center in both box edges (was a
fake cx±32 box that biased the recovered center near frame edges), remove the `/3.0f` bias,
add auto-move timeout.

## CG-S2 (2026-07-04) - Teensy 4.1 target + drop-in contract validated

- Migrated to Teensy 4.1 (T4.1 arrived 2026-07-03). Only `board = teensy41` in
  `platformio.ini` changed; wiring/pins identical to T4.0 (docs/WIRING.md).
- Clean build: SUCCESS 8.33s (FLASH code 74012 / data 361032; RAM1 free 414912; RAM2 free
  511872). No link warnings.
- Drop-in contract validated: `SEN0626Sensor`'s `person_sensor_face_t` matches IRIS
  `src/sensors/PersonSensor.h` byte-for-byte, and the public method surface matches
  (`isPresent/begin/read/enableID/setMode/enableLED/numFacesFound/faceDetails/
  timeSinceFaceDetectedMs`). Confirmed true drop-in.
- Status: REPO-ONLY. Not flashed/bench-verified (no T4.1 enumerated on SuperMaster this
  session).
- Framed as future-proofing / public-launch replicability, not a fix for a current fault:
  live IRIS Person Sensor works fine. Bench-only on the spare non-installed T4.1; live PS
  stays source of truth until VERIFIED.

## CG-S3 (2026-07-04) - durable gaze-node repo + IRIS plan, then firmware audit + dual-eye

CG-S3 spans two work items on the same day: first the docs/architecture pass (IRIS HANDOFF C,
below), then the firmware audit + dual-eye pass (Opus handoff, further below).
`FIRMWARE_VERSION` is bumped CG-S2 → CG-S3 for the code work.

### Part A - durable gaze-node repo; IRIS integration plan (docs only)

- CyclopsGaze is now the durable gaze-node identity. The old ESP32-S3 + OV2640 approach
  (`../OGLE`) was tombstoned/archived. It never reached reliable end-to-end face tracking at
  IRIS's real bench distance/lighting; research favors T4.1 + SEN0626. Decision made with the
  operator.
- Integration path chosen: native Person-Sensor-bus drop-in (SEN0626 read directly by IRIS's
  own T4.1, standing in the dead PS slot), not OGLE's separate-node USB-CDC → Pi4
  `ogle_bridge.py` → UDP `GAZE:` bridge. Simplest to replicate publicly; keeps gaze on the
  Teensy's local loop.
- Added `docs/archive/09_IRIS_INTEGRATION_PLAN.md`, the full PS-bus drop-in plan for a later
  IRIS deploy session: the I2C→UART bus change, the enumerated IRIS-side edits
  (`main.cpp:14/116`, begin/re-probe guard, config), the behavioral-gap handling (`is_facing`
  non-blocking because IRIS `psFacingRequired` defaults false since S153c; confidence-scale
  bench-verify vs operator CONF=60; coordinate/flip verify; LED no-op), the bench-verify gate,
  and staged rollout + rollback.
- README refreshed to the T4.1 identity and to point at the integration plan.
- No IRIS deploy, no flash. CyclopsGaze remains REPO-ONLY pending the
  docs/ENGINEERING_LOG.md bench-verify checklist.

### Part B - firmware audit + dual-eye (Opus handoff)

Full audit of the tracking chain against live code and the IRIS reference; fixes landed;
optional dual-eye added. Clean build (single-eye and `-DDUAL_EYE`), still REPO-ONLY (no T4.1
enumerated to flash). Full verdicts in docs/ENGINEERING_LOG.md §"CG-S3 Audit".

- Y-direction: confirmed correct, not inverted. The sign convention (targetX negated, targetY
  not) is byte-for-byte IRIS's production `main.cpp:594-595` on the identical EyeController,
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
  S153c). Previously CyclopsGaze tracked any detection ungated. Effective min SEN0626 score
  ≈19/100; bench-tunable.
- NATIVE_H calibration aid: shim now exposes rawFaceX/Y/Score; main logs them under
  `#define CG_CALIB_RAW 1` so the operator can confirm 480 vs 640 from serial.
- GAZE_GAIN knob (default 1.0) for bench range-tuning; range verified to match IRIS
  (mapRadius=240, r≈77.3). Center-recovery and 4:3-vs-1:1 mapping confirmed correct (no drift;
  per-axis normalisation is intentional).
- Dual-eye (optional): `#define DUAL_EYE` in config.h drives a second GC9A01A. Resolved a T4.1
  pin conflict: IRIS's second bus (SPI1) collides with Serial1 on both pin 0 (SPI1 CS) and
  pin 1 (SPI1 MISO), so both eyes share SPI0 with separate CS (eye1: CS9/DC8/RST6).
  docs/WIRING.md gains a full dual-eye section.
- Tunables (PS_CONF_GATE, GAZE_GAIN, FACE_LOST_MS, CG_CALIB_RAW) centralised in config.h.
  docs/ENGINEERING_LOG.md gains the full 10-step first-flash bench protocol.

## CG-S4 (2026-07-06) - first real bench pass: DIP-switch fix, ungated calibration logging

First session with the T4.1 actually connected and flashed (COM6). Status moved from
REPO-ONLY to DEPLOYED (bench-verified in part; see docs/ENGINEERING_LOG.md for what's still
open).

- SEN0626 not found, root-caused: the sensor's onboard I2C/UART mode DIP switch was left on
  I2C. This firmware is UART-only with no fallback. Fixed by flipping the switch, no code
  change. Documented in docs/WIRING.md and the bench protocol so it's checked first next
  time, before re-wiring.
- Calibration logging ungated: CG-S3's raw/diagnostic serial line only printed inside the
  confidence-gated branch, so it vanished the instant a detection dropped below
  `PS_CONF_GATE`, exactly the boundary data needed for range/gate tuning. `main.cpp` now
  logs every raw detection (`rawScore`/`conf`/`gate=PASS|REJECT`/`rawX`/`rawY`) regardless of
  gate outcome; the eye still only moves on a `PASS`.
- FIRMWARE_VERSION CG-S3 → CG-S4.

## CG-S5 (2026-07-06) - power fault fixed, confidence gate re-derived from vendor spec, distance floor documented

Same bench session, continued. Bench testing surfaced a narrow detection envelope (1-2ft /
±10-20°) and unstable lateral tracking; before more firmware tuning, traced both to real,
sourced causes.

- Power fault found and fixed: SEN0626 VCC measured 2.6V under load vs. 3.25V at the Teensy's
  own 3.3V pin. The drop was in a bad connector along the run, not the Teensy regulator.
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
  documented floor, out-of-spec operation, not a bug. Added to docs/WIRING.md (mounting
  guidance) and docs/ENGINEERING_LOG.md bench step 7b.
- Person Sensor comparison, honest gap flagged, not fabricated: no public FOV-in-degrees or
  minimum-working-distance spec was found for the Person Sensor to do an exact numeric
  comparison. Qualitatively, IRIS's own production history with the Person Sensor has never
  surfaced a "must stand back" complaint, while the SEN0626's 0.5m floor sits at typical close
  conversational distance. Flagged in docs/archive/09_IRIS_INTEGRATION_PLAN.md §6 as a real
  consideration for the deploy decision, not just a tuning task.
- Open, handed off: lateral (X-axis) tracking instability at close range needs re-testing now
  that power and the gate are fixed, before concluding anything further is wrong. See
  `docs/archive/11_HANDOFF_FABLE_LATERAL_TRACKING.md`.
- FIRMWARE_VERSION CG-S4 → CG-S5.
- Status: DEPLOYED, not yet VERIFIED. Sensor detection, power, and gate are bench-confirmed;
  direction (3.1), NATIVE_H (3.5/step 6), and lateral tracking are still open per
  docs/ENGINEERING_LOG.md.

## CG-S6/S7/S8 (2026-07-07) - tracking VERIFIED on bench; ready for IRIS integration

Full bench pass with T4.1 on COM6, operator live. Lateral-tracking investigation closed and
gaze tuned end-to-end. Status: VERIFIED.

- CG-S6, lateral direction fixed. X was mirrored: `main.cpp` negated `targetX` for the IRIS
  "eye-0 of a pair" mirror convention, but `EyeController::renderFrame()` also unconditionally
  flips `eye.x` for `eyeIndex==0`, and CyclopsGaze's single eye is always index 0, so the
  pair-mirror was applied with no pair to mirror against. Removed the negation in `main.cpp`
  (not the shared EyeController). Bench-confirmed correct L/R.
- CG-S7, GAZE_GAIN 1.0 → 1.7. Bench data showed a natural side-to-side head movement only
  spanned rawX 151-427 (native 0-640), so targetX topped out near ±0.5. Raised gain from that
  measured ratio; X and downward Y now reach near full travel. Bench-confirmed.
- CG-S8, Y_CENTER offset added. Sensor is mounted physically below the eye, so a face at
  true eye-level images near the frame top (bench-measured cy≈33, not the geometric center
  127.5). Old code treated 127.5 as neutral, so normal posture computed to y≈-1.27 (past the
  clamp), eye pinned looking up. `Y_CENTER=33` recenters neutral to y≈0. Bench-confirmed:
  neutral now reads ~0, downward range good.
- Upward-gaze ceiling = hardware limit, not a bug. With neutral correctly at y≈0, deliberate
  max-up-gaze only moved rawY to ~45 (≈9 counts above neutral). The sensor's below-eye
  mounting leaves almost no frame above eye-level. Not chased with asymmetric gain (would
  amplify quantization noise on the one axis already worst-off). Fix is physical: tilt/lower
  the sensor. Documented.
- LED not disableable in firmware, confirmed against source. DFRobot's own Modbus register
  map (wiki docs/23023) and their `DFRobot_GestureFaceDetection` library expose no LED
  register, only device addr, baud, and the detection/score thresholds. `enableLED(bool){}`
  stays a no-op stub. The white LED = onboard face-presence indicator (independent of
  PS_CONF_GATE); the brief blue = RGB gesture indicator (thumbs-up), incidental. To kill the
  LED for a build: physical cover only.
- FIRMWARE_VERSION CG-S5 → CG-S8.
- Status: VERIFIED, ready for IRIS integration as a drop-in Person Sensor (SEN-21231)
  replacement. Direction, gain, and Y-center all bench-confirmed. Remaining knobs (upward-gaze
  mount tilt, optional CG_CALIB_RAW=0 for quiet serial) are deploy-time, not blockers. Unblocks
  the IRIS integration (docs/IRIS_INTEGRATION.md).

## CG-S9 (2026-07-07) - IRIS integration: code-review pass + both-consumer drop-ins

Code-reviewed the drop-in against IRIS's actual source (read-only) and closed the gaps. No
CyclopsGaze tracking behavior changed, tracking stays VERIFIED (CG-S8). Adapters are
REPO-ONLY for IRIS until a deploy session flashes them.

- Found a second consumer the eyes-only plan missed. The dead Person Sensor drove two nodes:
  T4.1 eyes (`PersonSensor` class, X+Y) and the T4.0 servo head-pan node
  (`setupPersonSensor()`/`pollPersonSensor()` → `PersonResult`, X-only). The class shim only
  covers the eyes.
- New servo adapter: `integration/servo_teensy40_base_mount/person_sensor.{h,cpp}`, a
  SEN0626-backed reimplementation of the servo's `PersonResult` contract (Serial1,
  `faceCenterX=(box_left+box_right)/2` in 0-255, conf gate 152 = DFRobot score≥60, facing
  always-true). The servo `.ino` is unchanged.
- Eyes method-surface audit: IRIS calls only
  `isPresent/read/setMode/enableID/enableLED/faceDetails/numFacesFound/timeSinceFaceDetectedMs`,
  all present; the shim's missing `singleShot/labelNextID/persistIDs/eraseIDs` are never
  called. Struct byte-identical.
- Closed the `begin()` gap: the real `PersonSensor` has no `begin()`; IRIS probes via
  `isPresent()`. `SEN0626Sensor::isPresent()` now lazily runs `begin()` (UART bring-up + baud
  detect) so IRIS's probe loop works with no added call. Standalone unaffected (main.cpp calls
  begin() explicitly).
- Two IRIS-side warnings documented, not baked in: (1) do not propagate CG-S6 targetX
  negation-removal or CG-S8 `Y_CENTER` to IRIS, IRIS keeps its own negation + runtime
  `psYBias`; (2) confidence-scale mismatch, IRIS `psConfGate` is capped 0-100 but
  `box_confidence` is 0-255, so the eyes gate is currently inert; recommended fix (emit raw
  0-100) flagged as an operator decision requiring a CyclopsGaze re-verify.
- Added `integration/README.md`; expanded `docs/archive/09_IRIS_INTEGRATION_PLAN.md` (§2
  audit, §4 warnings, new §4b servo, §5 confidence math, §6 gate → IRIS-side).
- FIRMWARE_VERSION CG-S8 → CG-S9 (drop-in-only `isPresent()` change; tracking untouched, still
  VERIFIED).

## CG-S10 (2026-07-15) - camera module bench-confirmed physically separable from main PCB

Mechanical bench finding, no firmware change. The SEN0626's lens/sensor assembly attaches to
the Gravity board via ribbon cable + tape, not solder. Bench-tested with the camera
disconnected from the main board (head-shoulder LED, gesture RGB LED, IR-adjacent components
all absent) and face detect/track still worked through the ribbon alone. Closes the
mounting-footprint gap that made SEN0626 bulkier than the tiny Person Sensor it's replacing,
for both the IRIS-insurance and public-replicability reasons this repo exists. Open before
VERIFIED: confirm the detached camera reports the same I2C/UART register set / device address
/ baud once separated. An unruled-out board-side signal-conditioning dependency could change
register reads post-separation. Detail: `docs/ENGINEERING_LOG.md` CG-S10,
`docs/IRIS_INTEGRATION.md` §separable-camera. Status: REPO-ONLY, not flashed or integrated.

## CG-S11 (2026-07-17) - the drop-in went live in IRIS, and refined there

The SEN0626 driver validated here was actually swapped into the parent IRIS project for the
dead Useful Sensors Person Sensor (IRIS sessions S212 / S212b / S212c). This is the outcome
the whole repo was building toward, and the real swap surfaced refinements, captured here so
CyclopsGaze and IRIS don't drift. (All work below happened in the private IRIS repo;
CyclopsGaze source was synced to match in CG-S12.)

- Deliberate driver divergence, confidence scale. IRIS gates on a 0–100 `psConfGate` knob,
  so the shim's original `box_confidence = score*255/100` (0–255) made every real detection
  (raw 60–90 → 153–229) clear any reachable gate, silently rendering the operator's confidence
  knob inert. IRIS emits the raw DFRobot score (0–100) instead, so the gate means what
  DFRobot documents: a score ≥ 60 is a valid face. Same effective threshold as CyclopsGaze's
  152/255 (≈0.60), just the vendor's native scale.
- Integration bug found + fixed (S212b), center-only box vs an area presence test. IRIS's
  face-selection loop used box area as an "is there a face?" test (`size > maxSize`, `maxSize`
  starting at 0). The SEN0626 reports a face center, so the shim stores `box_left==box_right`
  and `box_top==box_bottom` → area is always 0, `0 > 0` is false, and every face was silently
  discarded before both the report and the gaze update. Sensor detected, zero tracking. Fixed
  on the IRIS side (area is a ranking key, not a presence test). CyclopsGaze's own consumer
  never had this bug, it reads the center directly, but it's the canonical gotcha for
  anyone adapting a center-only sensor into a box-oriented consumer, so it's documented in
  `docs/IRIS_INTEGRATION.md`. After the fix: live tracking, FACE:1 ×9 in 10 min.
- Gaze shaping generalized (S212c), per-axis signed gain + bias. IRIS previously had a
  single gaze tunable. The real bench (18–24in) showed L/R mirrored and only ~15° total
  travel. Root causes: (1) mirror = a direction issue, fixed by the gain's sign; (2) ~15° is
  not a bug, at 20in from an 85° FOV a ±6in head move only crosses ~40% of frame ≈ 0.4
  deflection, fixed by the gain's magnitude. New model `targetN = rawN * gain + bias` per
  axis; the X-gain default is transport-conditional (+1.0 for SEN0626, −1.0 for the Person
  Sensor rollback path).
- Status: SEN0626 is DEPLOYED + tracking live in IRIS. CyclopsGaze standalone tracking remains
  bench-VERIFIED as of CG-S8 (unchanged by the IRIS work itself).

## CG-S12 (2026-07-18) - sync refinements back to CyclopsGaze + public-release reorganization

Two things: (a) bring the IRIS-proven refinements back into this repo's own source, and (b)
restructure the repo for a public GitHub release.

Code sync (unverified on the standalone bench, re-bench is the #1 priority next flash):
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
  dual-eye); runtime tracking not yet re-observed. Treat standalone as DEPLOYED-pending-verify.

Repo reorganization (public release):
- Added LICENSE (MIT, with bundled TeensyEyes / Chris Miller attribution), CLAUDE.md (session
  anchor), RULES.md (engineering discipline), and rewrote README.md to frame the project
  accurately for a public audience (standalone gaze demo + IRIS drop-in module).
- Curated the flat pile of numbered session-handoff docs into a clean `docs/` tree (`WIRING`,
  `SEN0626_PROTOCOL`, `BENCH_PROTOCOL`, `IRIS_INTEGRATION`, `ENGINEERING_LOG`, `ATTRIBUTION`);
  moved the raw historical handoffs to `docs/archive/`; added `docs/media/` for photos.

## CG-S13 (2026-07-21) - second sync from live IRIS: transport knob + PS_CFG live tuning

Session goal was "bring IRIS's refined firmware back". Diffing both repos and reading the
running Pi4 first showed most of it was already here, which changed what the work actually was.

Already in sync (measured, not assumed):
- `SEN0626Sensor.{h,cpp}` differ from IRIS's copies by comment text only, zero code
  difference.
- IRIS's gaze code is frozen at S212c. Its last `main.cpp`/`config.h` commit is S213
  (2026-07-18) and changed only `PROTOCOL_VERSION`. CG-S12 had already synced the S212c model,
  so there was no un-synced IRIS code change to transfer.

What actually came back:
- `SEN0626_SERIAL` transport knob (`src/config.h`). Live IRIS runs the sensor on `Serial4`
  (RX 16 / TX 17), not `Serial1`, pin 0 is IRIS's left-eye CS. Two docs here asserted
  `Serial1`; both were wrong and are corrected. Default stays `Serial1` (this rig's
  bench-VERIFIED wiring).
- The `PS_CFG:` runtime-tuning protocol (IRIS S141 + S212c), the real polish. `CONF`,
  `FACING`, `LOST_MS`, `X_GAIN`, `Y_GAIN`, `X_BIAS`, `Y_BIAS`, `LED` are now runtime variables
  retunable over serial with no reflash. Parser, key set, `[DBG] PS_CFG KEY=value` ack wording
  and the S212c false-ack guard are IRIS-verbatim, and the `ps*` variable names match IRIS's
  exactly so the two `main.cpp` files stay diffable. Makes the outstanding CG-S12 re-bench a
  tune-live exercise instead of edit-reflash-repeat. Adds `PS_CFG?` (readback), needed only
  here, since IRIS reads live values back from the Pi4's `ps_config.json`; values are RAM-only
  on this board.
- Facing gate (`psFacingRequired`, default false) for surface parity; inert on SEN0626.
- Gaze math is now IRIS's verbatim expression (box-derived center, incl. the `/3` term). Zero
  behavior change, the center-only box makes both delta terms 0. Algebra, not bench.
- `config.h` tunables became `*_DEFAULT` seeds for the runtime variables.

Deliberately not adopted: IRIS's live values. Observed on the wire 2026-07-21 (firmware
S213): `CONF=25 LOST_MS=8500 X_GAIN=1.0 Y_GAIN=1.0 X_BIAS=0.0 Y_BIAS=0.0`, and IRIS is
tracking with them (79 `FACE:1` that day). But they are not tuned: `/home/pi/ps_config.json`
predates the SEN0626 swap (dated 07-15 vs swap 07-16) and carries no gain/bias keys at all, so
those fall back to compile-time defaults; `CONF=25` is Person-Sensor-era 0–255-scale carryover
flagged in IRIS's own S212 comment as an unmade decision. CyclopsGaze keeps its measured
CG-S7/CG-S8 values. Open/owed: a head-to-head behavioral comparison of the two value sets,
now a cheap `PS_CFG:` test. See docs/ENGINEERING_LOG.md CG-S13 and docs/IRIS_INTEGRATION.md §
"The tuning-value gap".

- `FIRMWARE_VERSION` CG-S12 → CG-S13. Both builds clean (single-eye + `-DDUAL_EYE`).
- Status: REPO-ONLY, nothing flashed, nothing observed running. No Teensy enumerated this
  session (`pio device list`: COM1 legacy + COM4/5 Bluetooth only). The PS_CFG parser, facing
  gate and IRIS-form gaze math are unverified on hardware, on top of CG-S12's unverified gate
  + gain/bias. Re-running docs/BENCH_PROTOCOL.md remains the #1 priority.

## CG-S14 (2026-07-21) - ESP32-S3 migration feasibility study (docs only, no code)

Exploratory study of moving off the Teensy 4.1, driven by BOM cost / public replicability
(operator, this session). **No source file was touched**; `FIRMWARE_VERSION` stays CG-S13.
Full study: **[docs/ESP32_S3_MIGRATION.md](docs/ESP32_S3_MIGRATION.md)**.

Portability audit, measured against live source this session:
- `EyeController.h` (600 lines, read in full), `eyes.h`, and all ~18k lines of LUT data are
  **portable unchanged**. No Teensy API in any of them; `PROGMEM` is a no-op on both targets and
  the tables are dereferenced directly, not via `pgm_read_byte`. The render engine was expected
  to be the blocker and is not.
- `GC9A01A_t3n` is **unportable and must be replaced**. Its `library.json` claims
  `"platforms": "*"`, which is false: the header is gated on `__IMXRT1062__`/`__IMXRT1052__`/
  `KINETISK`, includes Teensy's `DMAChannel.h`, and drives `IMXRT_LPSPI_t`/`KINETISK_SPI_t`
  registers directly (29 sites in the `.cpp`). Replacement is LovyanGFX or TFT_eSPI, not a
  hand-rolled driver.
- The port is tractable because `src/displays/Display.h` (CRTP, 5 methods) is already the seam.
  Swapping backends is one new ~150-line class selected in `config.h`, not a render rewrite.
- Three driver blockers, all in the contract-sensitive `SEN0626Sensor.{h,cpp}`: `elapsedMillis`
  members (`.h:99`), `serial.begin(baud)` needing S3 pin-matrix args (`.cpp:68`), and
  `while (millis() < BOOT_SETTLE_MS) {}` (`.cpp:84`).
- **Latent bug found, not introduced by the port.** That 2-second bare spin at `.cpp:84` has no
  yield: harmless on bare-metal Teensy, but under FreeRTOS on an S3 it starves the idle task and
  risks a task-watchdog reset. The portable fix (delay-based wait) is correct on both platforms.
  ⚠ **IRIS runs this same code on its T4.1.** Flagged for IRIS, not edited (read-only from here).

**A premise was checked and found false, which reshaped the study.** An earlier draft this
session claimed the lookup tables must be copied into internal SRAM on the S3, and derived a
hard "one eye design only" limit from it. Reading the linked Teensy binary
(`arm-none-eabi-size -A`) disproved it: `.text.progmem` is 352,136 bytes at 0x60002574, which
is external QSPI flash, with only `.data` 9,920 and `.bss` 2,240 in RAM. **The bench-VERIFIED
Teensy build already streams all 352 KB of tables out of flash through its cache.** So tables
in flash is the baseline, SRAM residency is an optimization the S3 has and the Teensy does
not, the one-design limit is void, and PSRAM is not required (two framebuffers are 225 KB
inside 512 KB of SRAM).

Render cost measured by replaying `renderEye()`'s exact address arithmetic over the real
tables on the host (simulation of the real pattern over real data, not hardware):
- **43,312 pixels and 214,492 table reads per frame**, 4.95 reads per pixel.
- Per frame the render **needs 100 KB of unique bytes but fetches 230 KB**, a 2.24x
  amplification. The polar tables and iris texture stride 240 bytes and waste ~3.1x.
- Across a sweep of gaze positions and pupil sizes **79% of all table bytes get touched**, so
  there is no small hot subset and selective residency buys little.
- Cache simulation: 64 KB/32 B/8-way misses 4.94%, 32 KB/32 B 8.57%, 16 KB/4-way 26.02%.
  **64-byte lines are worse than 32-byte** (21.8% vs 8.6% at 32 KB) because the striding
  wastes the extra bytes. The S3's line size is a build setting and the instinctive choice is
  the wrong one.

Board recommendation, revised: **bare ESP32-S3-DevKitC-1-N8R8, $15.00, 704 in stock at
DigiKey.** Cache size is the deciding spec and the S3's is configurable to 32/64 KB. Rejected
Raspberry Pi Pico 2 despite $5: RP2350's XIP cache is 16 KB, the 26% row, and 350 KB of tables
plus two framebuffers is 580 KB against 520 KB so SRAM cannot rescue it. Rejected ESP32-P4
(768 KB SRAM, 400 MHz) on toolchain maturity: Arduino support still beta, no official
PlatformIO, and its round boards are 720x720 MIPI rather than 240x240 SPI. Also withdrew this
session's earlier recommendation of the Waveshare DualEye board, which only wins if you do not
already own displays.

New: **docs/BOM.md**, a sourced parts list with domestic vendors, current prices and build
totals, written for the wider animatronics/props/cosplay audience as well as IRIS. Verified
2026-07-21: S3 devkit $15.00 (DigiKey, 704), SEN0626 $14.90 (DigiKey, 41), Adafruit 6178 round
GC9A01A $17.50, Teensy 4.0 $23.80, Teensy 4.1 $31.50. Two-eye S3 build lands at $64.90
domestic or $47.10 with generic displays, against $63.90 for the currently VERIFIED
single-eye Teensy build. Notable: **domestically the display dominates, not the MCU** (two
panels $35.00 vs board plus camera $29.90), and **DigiKey cannot complete the cart** since its
only round GC9A01A listing is marked obsolete, so a domestic build is two vendors.

README gained the standalone-animatronic framing, links to the BOM, and a new **Build paths**
section: four routes (A single tracking eye VERIFIED, B no-camera wandering prop, C dual-eye
compiles-but-never-run, D the unbuilt ESP32-S3 path) each with cost, steps, doc links and an
honest status, plus a pick-by-what-you-want table.

Two repo defects fixed:
- The README's hero image pointed at `docs/media/cyclopsgaze_tracking.jpg`, which has never
  existed (introduced 3ec8496, CG-S12), so the public GitHub page rendered a broken image.
  The line is now commented out with instructions to restore it, and `docs/media/README.md`
  says so.
- **BENCH_PROTOCOL.md gained step 11, a frame-rate baseline.** `SHOW_FPS` has sat commented
  out at `src/displays/Display.h:5` since the engine was bundled, so this project's frame rate
  has never been measured on any board. That missing number is what blocks the ESP32-S3
  evaluation from being judged at all. The step documents the one-line change, notes the
  counter renders on the eye rather than to serial, and flags that the reading is slightly
  pessimistic because drawing it costs time. The protocol header now calls out steps 10 and 11
  as the two that have never been executed.

- Recommendation: **buy nothing yet.** Two cheap measurements gate everything. First uncomment
  `SHOW_FPS` (`src/displays/Display.h:5`) and record the Teensy's frame rate, which **has
  never been measured on any board**, so "fast enough" currently has no baseline. Then run the
  real inner loop on the N16R8 already on hand.
- Status: REPO-ONLY, docs only. Nothing built for S3, nothing flashed, no frame rate observed
  anywhere. Two-eye rendering has never run on hardware on any board. Re-running
  docs/BENCH_PROTOCOL.md on the Teensy remains the standing #1 priority.

## CG-S15 (2026-07-21) - second eye set (hazel) + rotation + the `EYE:` protocol

The firmware carried exactly **one** eye set (`nordicBlue`) since CG-S1. It now carries two and
switches between them on a timer or on command.

- Added **`hazel`**, copied verbatim from upstream TeensyEyes along with the
  `polarDist_240_125_60_0` table it needs (`hazel` uses iris radius 60; `nordicBlue` uses 69,
  so the existing `_69_0` table could not be reused). `polarAngle_240` and `disp_240_125` were
  already here and are shared. **`hazel` is Chris Miller's work in its entirety, artwork
  included** -- docs/ATTRIBUTION.md and the README credits updated to say so plainly, since
  until now the only bundled eye's texture art was original to this project.
- **Prior art, not invention:** upstream TeensyEyes already solves eye switching with an outer
  array of eye sets plus `EyeController::updateDefinitions()`, driven by an index
  (`src/main.cpp:75-76` upstream, `std::array<std::array<EyeDefinition,2>,13>` in its
  config.h). CyclopsGaze already had `updateDefinitions()` and the same
  `std::array<std::array<EyeDefinition,N>,1>` shape, just trimmed to a single set. This change
  restores the upstream mechanism rather than adding a new one.
- **`EYE:` serial protocol** (`EYE?`, `EYE:next`, `EYE:<name>`, `EYE:AUTO=0/1`, `EYE:MS=n`).
  Deliberately a **separate command namespace from `PS_CFG:`**: PS_CFG's parser, key set and
  `[DBG] PS_CFG k=v` ack wording are IRIS-verbatim so the two main.cpp files stay diffable, and
  folding eye keys into it would break that for a feature IRIS does not have. IRIS's
  iris_post.py ack regex cannot match an `EYE:` line.
- Auto-rotation every **20 s** by default (`EYE_ROTATE_MS_DEFAULT`, `EYE_AUTO_ROTATE_DEFAULT`
  in config.h; runtime-tunable, RAM-only like the `ps*` values). Selecting a set by name turns
  rotation off, so an operator's explicit pick is not silently overwritten seconds later.
  Interval is floored at 1000 ms because each swap sets `drawAll` and forces a full repaint.
- Set names are read from `EyeDefinition::name` rather than a parallel string table, so the
  serial interface cannot drift out of sync with the actual set list.
- Constraint documented in config.h and the README: **all sets must share
  `polar.mapRadius`** (240 here). The gaze position in EyeController is stored in mapRadius
  units and is not rescaled across a definition swap. Verified both eyes are 240 before
  shipping this.
- Adding further eyes is now a config edit: copy the header plus its `polarDist_*` table, add
  the include, add a row. Upstream ships ~30 eyes. Cost is ~280 KB flash each.

- `FIRMWARE_VERSION` CG-S13 → **CG-S15** (CG-S14 was docs-only and did not bump).
- Both builds clean. Single-eye FLASH code 119468 / data 646808, RAM1 free 375872;
  `-DDUAL_EYE` code 119660 / data 647832, RAM1 free 374848. Flash data grew 362056 → 646808,
  i.e. **+284,752 bytes**, matching the predicted +279,632 for hazel's textures plus the extra
  polarDist table. ~7.35 MB of flash still free.
- Status: **REPO-ONLY.** Compiles for both configurations; **nothing was flashed and no eye
  was observed changing.** Checks that would confirm are listed in docs/BENCH_PROTOCOL.md
  § "Eye sets": boot line reads `sets=2 start=nordicBlue`, the look visibly changes ~20 s
  later, `EYE:hazel` switches immediately and reports `auto=0`, `EYE?` lists both, a typo'd
  name changes nothing, and gaze tracking survives a swap without a jump or freeze.

## CG-S16 (2026-07-25) - ten selectable eyes, hazel diagnosed, demo-video captions

Three things: the eye catalogue went from 2 designs to 10 with one-line selection, the
operator's "hazel looks off" was traced to authored parameters rather than a bug, and the
39-second demo clip got its captions. `FIRMWARE_VERSION` CG-S15 → **CG-S16**.

### Eye artwork: 10 bundled, 1 enabled, disabled ones cost nothing

- Bundled 8 more of Chris Miller's eyes (`bigBlue`, `cat`, `demon`, `doe`, `doomRed`, `dragon`,
  `fish`, `skull`) plus the 7 `polarDist_240_*`, 2 `disp_240_*` and `noeyelids_120` tables they
  need. With `nordicBlue` and `hazel` that is **10 selectable designs**. All verified
  byte-identical to upstream by `diff`.
- **Prior art, not invention:** selection is upstream's own mechanism, a block of `#include`
  lines with a matching block of array rows, commented in pairs. Upstream's `src/config.h` does
  exactly this with 23 eyes. Adopted verbatim in shape.
- **A disabled eye costs zero flash, measured not assumed.** Controlled A/B with one variable:
  `hazel.h` included but its row commented out gave FLASH code 83,836 / data 362,056, and
  commenting the `#include` too gave *the same two figures*. Both equal the CG-S13 baseline from
  before any second eye existed. The Teensy builder passes `-fdata-sections` and
  `-Wl,--gc-sections`, so unreferenced tables are dropped at link. Bundling is therefore nearly
  free: 8 headers and 10 table `.cpp` files added ~3.4 MB of repo and 0 bytes of firmware.
- **The CG-S15 `mapRadius` constraint is not binding.** All 23 upstream eyes set
  `polar.mapRadius` to 240, checked positionally in every header, same as `nordicBlue`. Nothing
  in the catalogue can trip it. `config.h` and the README said otherwise and are corrected.
  Eyeball `radius` may differ freely between sets (120/125/130 all bundled) because
  `EyeController` reads `definition->radius` per frame and caches nothing derived from it.
- **The hand-maintained array count is gone.** `eyeDefinitions` is now an array of
  `std::array<EyeDefinition, N>` whose extent the compiler counts, exposed as
  `EYE_SET_COUNT` via `std::size`. Enabling an eye is two uncomments and no arithmetic.
  main.cpp's 10 `.at(i)` / `.size()` call sites moved to `[i]` / `EYE_SET_COUNT`.
- **A C++17 trap found and avoided, worth recording.** The first attempt used class template
  argument deduction (`std::array eyeDefinitions{ std::array{nordicBlue::eye} }`). With two or
  more rows that deduces correctly; with exactly **one** row it matches the *copy* deduction
  candidate and collapses to the inner array, so `eyeDefinitions[i][0]` stops compiling. It
  would have worked for every multi-eye build and broken only the shipped single-eye default.
- Default ships `nordicBlue` alone, matching the operator's own revert: with one set nothing
  rotates, which is what a face-tracking demo wants. Rotation and `EYE:` are untouched and
  activate as soon as a second row is uncommented.
- New **[docs/EYE_ARTWORK.md](docs/EYE_ARTWORK.md)**: a survey of where eye art comes from,
  researched from source repos rather than recalled. There is **no third-party ecosystem** of
  TeensyEyes eyes; the only public fork (`MichaelMeissner/TeensyEyes`) has an identical eye list.
  But **six MIT-licensed Adafruit designs were never ported to TeensyEyes at all**
  (`terminatorEye`, `goatEye`, `naugaEye`, `noScleraEye`, `defaultEye` from `Uncanny_Eyes`, and
  `reflection` from `M4_Eyes`), and the doc records the real blocker: `config.eye` is two
  incompatible schemas, upstream's nested/hex-colour/PNG-lid form versus M4_Eyes'
  flat/RGB-triple/`eyelidIndex` form. Also records that the generator's input is just an iris
  image, which is how `nordicBlue` exists.
- Attribution rebalanced honestly: nine of ten bundled eyes are Chris Miller's artwork, so by
  volume the art here is mostly his. docs/ATTRIBUTION.md and the README say so, and list the 13
  upstream eyes deliberately not bundled.

### hazel does not look wrong, it looks different (operator bench report)

The operator reported hazel looking "off" versus `nordicBlue` when it was the active set. Traced
without touching hardware, and **nothing is broken**:

- `hazel.h` and every table it uses are byte-identical to upstream (`diff`).
- `backColor` and `squint` are consumed by *identical expressions* in CyclopsGaze's modified
  `EyeController` and upstream's, so the 305-line divergence is not implicated.
- It is authored differently: `backColor` 35138 where `nordicBlue` uses 0, so a coloured rim is
  painted where the eyeball ends but the eyelid aperture continues (`EyeController.h:435`); iris
  radius 60 against 69; pupil reaching 0.70 of that smaller iris against 0.47; and half the
  squint. Recorded per-eye for all ten in the `config.h` header comment and the README table, so
  the next person picking an eye can predict this instead of re-diagnosing it.

### Demo-video captions (media)

- New **[docs/media/VIDEO_CAPTIONS.md](docs/media/VIDEO_CAPTIONS.md)**: three caption drafts for
  the 39-second clip (build-log cold open, retention cut, documentary), why one shipped, the
  YouTube title/description, short-form caption, tags and Resolve notes (captions belong in the
  **upper** third here, the rig sits low in the orbit).
- Shipped `CyclopsGaze_captions_v2.srt` beside the Resolve project. The existing
  `CyclopsGaze_captions_final.srt` was left alone, already burned into a render.
- Clip measured with `ffprobe`: 38.702 s, 1920x1080, ~30 fps, AAC present.
- Captions written against sampled frames, not the firmware docs, which mattered: the footage
  shows `nordicBlue` only, so nothing claims eye rotation, and the operator never leaves frame,
  so lose-face-then-wander is worded as a capability rather than an on-screen event.
- Hero photo committed and the README hero image restored, fixing the broken-image defect
  CG-S14 recorded. It points at the real filename now and renders at a constrained width because
  the photo is a 1074x2114 portrait.
- Caught while sourcing a price: README's intro `$47` is [BOM.md](docs/BOM.md)'s **unbuilt**
  ESP32-S3 row. The rig on screen is the T4.1 build at **$63.90**.

- New **[docs/media/VIDEO_CAPTIONS.md](docs/media/VIDEO_CAPTIONS.md)**: three caption drafts
  (build-log cold open, retention cut, documentary), the reasoning for which one shipped, the
  YouTube title/description, short-form caption, tags, and Resolve timeline notes (captions
  belong in the **upper** third here, the rig sits low in the orbit).
- Shipped `CyclopsGaze_captions_v2.srt` to `C:\Users\SuperMaster\Videos\CyclopsGaze\`, beside
  the Resolve project. The pre-existing `CyclopsGaze_captions_final.srt` was left untouched,
  since it is already burned into `..._captioned_1080p.mp4`.
- Clip measured with `ffprobe`, not assumed: 38.702 s, 1920x1080, ~30 fps, AAC track present.
- Captions were written against sampled frames rather than the repo's description of the
  firmware, which mattered twice. The footage shows **`nordicBlue` only**, so no caption claims
  the CG-S15 two-set rotation; and the operator never leaves frame, so the lose-face-then-wander
  behavior is worded as a capability rather than as something the viewer is watching happen.
- Caught while sourcing a price for the captions: **README quotes ~$47 for "a single tracking
  eye" in its intro, which is the unbuilt ESP32-S3 figure.** docs/BOM.md puts the T4.1 rig in
  the video at **$63.90**, and README § Path A already says ~$64. The intro number is not wrong
  in itself but reads as the price of the thing being demonstrated. Left as-is, flagged.
### CG-S15 was flashed. Status corrected on the operator's own report.

The state discrepancy this session opened is closed by the operator, not by inference:

- **CG-S15 is DEPLOYED.** The operator flashed the firmware carrying the serial eye-set change
  to the board. That build also carries CG-S12's raw-score gate and per-axis gain/bias and
  CG-S13's PS_CFG parser, so all of it has now run on hardware.
- Gaze tracking was observed: the operator shot the 2026-07-24 demo clip with it, and the eye
  visibly holds on them through a full orbit. So the CG-S12 gate at 60 on the raw 0-100 scale
  does pass real faces, and the per-axis gain/bias give usable travel in the correct direction.
- **The eye-set rotation was flashed, seen, and rejected on taste.** The operator reverted
  `eyeDefinitions` to a single set by hand because cycling looks wrong in a face-tracking demo.
  That revert is the shipped default now, deliberately.
- **Still not confirmed, so still owed:** the `PS_CFG:` parser and readback, the facing gate,
  `LOST_MS`/autoMove resume, NATIVE_H, the dual-eye path (BENCH_PROTOCOL step 10), and the frame
  rate (step 11). A video cannot show any of them. Re-running docs/BENCH_PROTOCOL.md stays the
  #1 priority, but it is now a shorter list than CG-S15 implied.
- Also recorded, on the operator's instruction: **ChatGPT/Codex sessions on this repo are
  read-only.** The single-eye revert was the operator's own manual edit, not an agent's.

### Flashed and bench-observed, same session: CG-S16 is DEPLOYED

The operator flashed the T4.1 and the serial link was read directly on COM6. Observed, verbatim:

```
[CG] CyclopsGaze CG-S16
[CG] SEN0626 found at 9600 (attempt 1)
Init GC9A01A display #0: rotate=0, mirror=1
_t3n::begin mosi:11 SCLK:13 CS:10 DC:2 SPI clocks:20000000 2000000
0: useFrameBuffer() OK
Success
[CG] EYE sets=1 start=nordicBlue rotate=on every 20000ms
```

Then, by command:

```
VERSION          -> [CG] CyclopsGaze CG-S16
PS_CFG?          -> [CG] PS_CFG_DUMP CONF=60 FACING=0 LOST_MS=3000 X_GAIN=1.70 Y_GAIN=1.70 X_BIAS=0.00 Y_BIAS=1.26 LED=0
EYE?             -> [CG] EYE_DUMP current=0 name=nordicBlue auto=1 ms=20000 count=1 sets: nordicBlue
PS_CFG:BOGUS=1   -> [DBG] PS_CFG UNKNOWN key BOGUS
EYE:next         -> [CG] EYE set=0 name=nordicBlue (next)
```

**Now VERIFIED, after being owed since CG-S13:**
- The `PS_CFG?` readback parser works, and **every live value matches its `config.h` seed
  exactly** (CONF 60, FACING 0, LOST_MS 3000, gains 1.70/1.70, biases 0.00/1.26). The runtime
  variables are being initialised from the defaults as designed.
- The unknown-key guard rejects a typo with `UNKNOWN key BOGUS` **and does not ack it**, which is
  the S212c false-ack behaviour ported at CG-S13.
- `EYE?` reports `count=1`, and `EYE:next` at one set wraps to itself: a clean no-op rather than
  an out-of-range index. CG-S16's `EYE_SET_COUNT` and the single-set default both behave.
- CG-S16 boot line and `sets=1 start=nordicBlue` confirm the shipped default on hardware.

**Still not observed:** no `faces=` line appeared in the read window, so the gaze chain was not
exercised this session. Tracking still rests on the CG-S15 video. Also untouched: the facing
gate, `LOST_MS`/autoMove resume, NATIVE_H, dual-eye (step 10) and the frame rate (step 11).

- Status: **CG-S16 is DEPLOYED** (flashed, and the version string observed on COM6, per the
  repo's own rule). CG-S15 is superseded on the board. Both builds clean on Teensy 4.1: single-eye
  FLASH code 83,836 / data 362,056 / RAM1 free 413,824, and `-DDUAL_EYE` code 84,028 / data
  362,056. Enabling two extra eyes was compile-tested (`bigBlue` + `skull`, data 909,776) to
  prove the newly copied tables link, then reverted. **No second eye has been rendered on
  hardware**, since the shipped build carries one.

⚠ **New bench observation, open:** the operator reports **jitter/flicker on the display**, with
all wiring on dupont jumpers on a breadboard. Not diagnosed. The display initialises cleanly
(`useFrameBuffer() OK`, `Success`) so it is not a bring-up failure. Leading suspect is signal
integrity: `SPI_SPEED` is 20 MHz over unshielded jumpers with no paired ground return. The cheap
discriminator is dropping `SPI_SPEED` to 10 MHz and re-observing; if the flicker is unchanged,
the next suspects are the 3.3 V rail measured at the display's own VCC pin under load (the CG-S5
failure mode) and then the refresh rate itself, which **has never been measured on any board**
(`SHOW_FPS`, `src/displays/Display.h:5`, BENCH_PROTOCOL step 11, never executed).

## CG-S17 (2026-07-25) - flicker A/B: SPI 20 -> 10 MHz, and the first frame-rate reading

Two one-line changes, flashed together, to attack the CG-S16 flicker report and to finally
execute BENCH_PROTOCOL step 11. `FIRMWARE_VERSION` CG-S16 → **CG-S17**.

- **`SPI_SPEED` 20 MHz → 10 MHz** (both the single-eye and DUAL_EYE branches of `config.h`).
  This is a discriminator, not a fix. Dupont jumpers on a breadboard are a poor transmission line
  at 20 MHz: unshielded, no ground return paired with SCK/MOSI, breadboard stray capacitance. If
  the flicker clears, the cause is signal integrity and the real fix is physical, after which the
  clock can go back up. Cost of halving it: CG-S14 measured a full 115,200-byte frame as a ~21 FPS
  bus-time ceiling at 20 MHz, so this roughly halves that headroom.
  `updateChangedAreasOnly(true)` is what keeps the normal case well under a full-frame push.
- **`SHOW_FPS` enabled** (`src/displays/Display.h:5`), commented out since the engine was bundled
  at CG-S1. **This project has never had a frame-rate number on any board**, which is the gap
  CG-S14 flagged as blocking any ESP32-S3 judgement. The counter renders **on the eye itself**,
  not to serial, so the reading has to be read off the display and is slightly pessimistic
  because drawing it costs time.

- Build clean: FLASH code 85,500 / data 362,056 / RAM1 free 413,824. Code grew 1,664 bytes over
  CG-S16 for the counter; data is unchanged.
- **Status: DEPLOYED.** Flashed and read back on COM6 the same minute:
  `[CG] CyclopsGaze CG-S17`, `VERSION` agreeing, and crucially
  `_t3n::begin mosi:11 SCLK:13 CS:10 DC:2 SPI clocks:10000000 2000000`, so the new clock is
  confirmed live on the wire rather than merely compiled. Sensor found at 9600 on attempt 1,
  `EYE_DUMP` still `count=1 nordicBlue`.
- ⚠ **Not VERIFIED, and it cannot be from here:** both of this session's questions need human
  eyes on the display. Whether the flicker changed, and what number the FPS counter shows, are
  invisible over serial. Until the operator reports them, the flicker cause is still undiagnosed
  and step 11 is still unexecuted in substance. Record the FPS figure here when it is read; it
  becomes the baseline that
  [docs/ESP32_S3_MIGRATION.md](docs/ESP32_S3_MIGRATION.md) has been waiting on.
- Reminder for the next session: **comment `SHOW_FPS` back out** once the number is recorded, or
  the counter sits on the artwork permanently.

## CG-S17b/c (2026-07-25) - the flicker was the frame rate, and the frame rate doubled

CG-S17's A/B came back negative and that negative was the useful part. Operator reported the
flicker **persisted at 10 MHz**, worst on the white sclera, with **3.12 V measured at the display**
and the on-screen FPS digits rendering inverted relative to the eye. Chasing those three produced
this project's first frame-rate numbers and a 2x improvement. Both sub-versions were flashed and
read back on COM6.

### CG-S17b: FPS to serial instead of onto the eye

`GC9A01A_Display::update()` drew the counter with `drawNumber(fps, 110, 110)`, which has three
problems here. It sits on the artwork; it renders in the panel's coordinate space while the eye
receives an **additional software flip** for the `eyeIndex==0` pair convention (CG-S6), so the
digits necessarily disagree with the eye's orientation; and drawing it every frame costs time
that biases the measurement. Now printed once a second as `[CG] FPS display=N nn`. Deliberate
divergence from bundled TeensyEyes, recorded in docs/ATTRIBUTION.md.

### The measurement, and what it proved

**First frame-rate figures ever recorded for this project on any board.**

| Configuration | Measured FPS | Full-frame bus ceiling |
|---|---|---|
| 10 MHz SPI, synchronous (CG-S17) | **10-19**, mostly 10-12 | ~10.8 |
| 30 MHz SPI, async DMA (CG-S17c) | **20-22** | ~32 |

The 10 MHz row sits exactly on its bus ceiling. That is the diagnosis: **the panel is pushing
near-full frames every update**, because the eye aperture is 43,312 of 57,600 pixels (CG-S14) so
`updateChangedAreasOnly(true)` has almost nothing to skip. A 10 to 15 Hz refresh on a large white
field is itself the flicker, which is why the white sclera showed it worst, and why CG-S17's
clock reduction made the symptom worse rather than better. **Signal integrity is ruled out.**

### CG-S17c: adopt upstream's two performance values

Both were CyclopsGaze divergences from upstream that nobody had recorded a reason for, and both
cost frame rate:

- **`SPI_SPEED` 10 MHz → 30 MHz.** Upstream TeensyEyes ships 30 MHz; this repo had sat at 20 MHz
  since CG-S1.
- **`asyncUpdates` false → true** on every display entry. Upstream ships ASYNC on for both of its
  displays. Async DMA lets the next frame render while the current one is still transferring;
  `EyeController::renderFrame()` already supported it, bailing early on `!isAvailable()`.

Result: **20-22 FPS, roughly double**, confirmed live with `SPI clocks:30000000` on the boot line.
This also unblocks docs/ESP32_S3_MIGRATION.md, whose entire stage 0 was waiting on this number,
and sharpens its conclusion: the display is bus-bound before it is compute-bound, so an S3
backend without a DMA path loses half the frame rate regardless of cache behaviour.

### Gaze tracking observed, and the gate found chattering

The 30 MHz run caught the gaze chain live, which no previous session in this repo had observed
since CG-S12 changed it:

```
[CG] faces=1 rawScore=62 conf=62 gate=PASS | rawX=492 rawY=82
[CG]   -> raw=0.54,-0.66  target=0.91,0.13  (gain 1.70/1.70 bias 0.00/1.26)
```

**CG-S12's raw-score gate and per-axis gain/bias are now bench-observed**, not merely compiled.

But the scores oscillate 60 to 79 and land on `gate=REJECT` at exactly 60 repeatedly, so at the
operator's working distance and lighting **`PS_CONF_GATE_DEFAULT = 60` sits right on the noise
floor** and the eye keeps acquiring and dropping the same face. DFRobot's documented validity
floor is a vendor recommendation, not a measurement of this unit at this range. Live-tuned down
with `PS_CFG:CONF=50`, which also **VERIFIED the last unproven PS_CFG path, the ack**:
`[DBG] PS_CFG CONF=50` in IRIS's exact ack shape, confirmed by a readback showing `CONF=50`.
The value is RAM-only and was not written into `config.h`, because whether 50 is right needs more
than one sitting.

### Status

**CG-S17c is DEPLOYED**, boot line and `SPI clocks:30000000 2000000` both observed. Newly
VERIFIED this session: the gaze chain end to end, the `PS_CFG:` ack, and the frame rate.

**Confirmed by the operator, same session: "looks better, mostly fixed, observable only with
close inspection."** So the diagnosis holds and the flicker is closed as a fault. The 3.12 V
reading is left on the record but is **not** the cause; it never needed chasing. Async tearing
did not materialise either.

⚠ **Residual, and where the remaining headroom is.** A faint artefact survives at 20-22 FPS,
which is unsurprising at a ~21 Hz refresh. The arithmetic bounds what is left: a full frame at
30 MHz is 115,200 bytes x 8 / 30e6 = **30.7 ms, a 32.5 FPS ceiling**, while 20-22 FPS means
**45-50 ms per frame**. Two readings fit that, and they have opposite fixes:

- **Render-bound.** If async fully overlaps, frame time is `max(render, transfer)`, so the render
  is ~45-50 ms and the bus is idle a third of the time. More SPI clock buys nothing, and the only
  lever is render cost: 43,312 pixels at 4.95 table reads each, 214,492 reads per frame (CG-S14).
- **Partially overlapped.** If the transfer is not fully hidden, frame time is closer to
  `render + transfer` and a faster clock still helps.

**One flash distinguishes them:** set `SPI_SPEED` to 40 MHz and re-read the counter. Flat means
render-bound and 30 MHz is the right resting place; a rise means bus time is still exposed. Not
done, because 30 MHz is upstream's proven value and going beyond it on dupont wiring reintroduces
the signal-integrity risk this session just eliminated.

Housekeeping before any demo build: **turn off `SHOW_FPS` and `CG_CALIB_RAW`.** Note the latter is
not an FPS problem, roughly 14 serial lines per second against a ~21 Hz loop, so it is about
clean output rather than speed.
