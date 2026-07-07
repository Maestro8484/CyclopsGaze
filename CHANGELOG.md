<!-- Completed history only. Append new entries at the bottom. Forward-looking work + current bench status live in NOTES.md. -->

# CyclopsGaze Changelog

---

## CG-S1 (2026-07-03) — initial testbed

Standalone single-eye face-tracking testbed created. SEN0626 Modbus RTU (FC04 input registers, device 0x72) driver + single GC9A01A 240×240 eye tracking on Teensy 4.0. `SEN0626Sensor` shim designed to match the Useful Sensors Person Sensor (SEN-21231) interface used by IRIS. Baud auto-detect (115200 → 9600). Files: `src/` (CyclopsGaze.ino, config.h, sensors/, displays/, eyes/), `platformio.ini`, doc scaffolding (`00`–`07`, README, NOTES, WIRING).

Follow-up fix (b84033d): gaze tracking corrected — store face **center** in both box edges (was a fake cx±32 box that biased the recovered center near frame edges), remove the `/3.0f` bias, add auto-move timeout.

## CG-S2 (2026-07-04) — Teensy 4.1 target + drop-in contract validated

- **Migrated to Teensy 4.1** (T4.1 arrived 2026-07-03). Only `board = teensy41` in `platformio.ini` changed; wiring/pins identical to T4.0 (05_WIRING).
- Clean build: SUCCESS 8.33 s (FLASH code 74012 / data 361032; RAM1 free 414912; RAM2 free 511872). No link warnings.
- **Drop-in contract validated:** `SEN0626Sensor`'s `person_sensor_face_t` matches IRIS `src/sensors/PersonSensor.h` byte-for-byte, and the public method surface matches (`isPresent/begin/read/enableID/setMode/enableLED/numFacesFound/faceDetails/timeSinceFaceDetectedMs`). Confirmed true drop-in.
- **Status: REPO-ONLY** — not flashed/bench-verified (no T4.1 enumerated on SuperMaster this session).
- Framed as future-proofing / public-launch replicability (not a fix for a current fault; live IRIS Person Sensor works fine). Bench-only on the spare NON-installed T4.1; live PS stays source of truth until VERIFIED.

## CG-S3 (2026-07-04) — durable gaze-node repo + IRIS plan, then firmware audit + dual-eye

CG-S3 spans two work items on the same day: first the docs/architecture pass
(IRIS HANDOFF C, below), then the firmware audit + dual-eye pass (Opus handoff,
further below). `FIRMWARE_VERSION` is bumped CG-S2 → **CG-S3** for the code work.

### Part A — durable gaze-node repo; IRIS integration plan (docs only)

- **CyclopsGaze is now the durable gaze-node identity.** The old ESP32-S3 + OV2640 approach (`../OGLE`) was tombstoned/archived — it never reached reliable end-to-end face tracking at IRIS's real bench distance/lighting; research favors T4.1 + SEN0626. Decision made with the operator.
- **Integration path chosen: native Person-Sensor-bus drop-in** (SEN0626 read directly by IRIS's own T4.1, standing in the dead PS slot), **not** OGLE's separate-node USB-CDC → Pi4 `ogle_bridge.py` → UDP `GAZE:` bridge. Simplest to replicate publicly; keeps gaze on the Teensy's local loop.
- Added **`09_IRIS_INTEGRATION_PLAN.md`** — the full PS-bus drop-in plan for a later IRIS deploy session: the I2C→UART bus change, the enumerated IRIS-side edits (`main.cpp:14/116`, begin/re-probe guard, config), the behavioral-gap handling (`is_facing` non-blocking because IRIS `psFacingRequired` defaults false since S153c; confidence-scale bench-verify vs operator CONF=60; coordinate/flip verify; LED no-op), the bench-verify gate, and staged rollout + rollback.
- README refreshed to the T4.1 identity and to point at the integration plan.
- No IRIS deploy, no flash. CyclopsGaze remains REPO-ONLY pending the NOTES.md bench-verify checklist.

### Part B — firmware audit + dual-eye (Opus handoff)

Full audit of the tracking chain against live code and the IRIS reference; fixes
landed; optional dual-eye added. Clean build (single-eye and `-DDUAL_EYE`), still
**REPO-ONLY** (no T4.1 enumerated to flash). Full verdicts in NOTES.md §"CG-S3 Audit".

- **Y-direction: confirmed CORRECT, not inverted.** The sign convention (targetX
  negated, targetY not) is byte-for-byte IRIS's production `main.cpp:594-595` on
  the identical EyeController — field-proven. The peer review's "Y inverted"
  hypothesis is rejected on the firmware side; any residual is the SEN0626 Y-axis
  orientation, a one-line bench flip if needed (NOTES bench step 5).
- **autoMove freeze: verified fixed.** The resume check runs as an `else if` for
  every no-face path (throttle, comms failure, sub-gate), so it can't stick OFF.
- **Modbus timeout 300 → 100 ms** (`MODBUS_RESP_TIMEOUT_MS`): caps a stalled-read
  eye freeze at 100 ms while keeping ~4x margin over the ~25 ms normal round trip.
- **Baud detect hardened:** 2 s power-on settle floor before first probe (AI-model
  boot), 200 ms per-baud line settle, and 3 full-sweep retries (`BOOT_SETTLE_MS`,
  `BAUD_ATTEMPTS`).
- **Confidence gate added to the consumer loop** (`PS_CONF_GATE`, default 45 to
  match IRIS S153c) — previously CyclopsGaze tracked any detection ungated.
  Effective min SEN0626 score ≈19/100; bench-tunable.
- **NATIVE_H calibration aid:** shim now exposes rawFaceX/Y/Score; main logs them
  under `#define CG_CALIB_RAW 1` so the operator can confirm 480 vs 640 from serial.
- **GAZE_GAIN** knob (default 1.0) for bench range-tuning; range verified to match
  IRIS (mapRadius=240, r≈77.3). Center-recovery and 4:3-vs-1:1 mapping confirmed
  correct (no drift; per-axis normalisation is intentional).
- **Dual-eye (optional):** `#define DUAL_EYE` in config.h drives a second GC9A01A.
  Resolved a T4.1 pin conflict — IRIS's second bus (SPI1) collides with Serial1 on
  BOTH pin 0 (SPI1 CS) and pin 1 (SPI1 MISO), so both eyes share SPI0 with separate
  CS (eye1: CS9/DC8/RST6). 05_WIRING gains a full dual-eye section.
- Tunables (PS_CONF_GATE, GAZE_GAIN, FACE_LOST_MS, CG_CALIB_RAW) centralised in
  config.h. NOTES.md gains the full 10-step first-flash bench protocol.

## CG-S4 (2026-07-06) — first real bench pass: DIP-switch fix, ungated calibration logging

First session with the T4.1 actually connected and flashed (COM6). Status moved from REPO-ONLY to DEPLOYED (bench-verified in part; see NOTES.md for what's still open).

- **SEN0626 NOT FOUND, root-caused:** the sensor's onboard I2C/UART mode DIP switch was left on I2C. This firmware is UART-only with no fallback. Fixed by flipping the switch — no code change. Documented in 05_WIRING.md and the bench protocol so it's checked first next time, before re-wiring.
- **Calibration logging ungated:** CG-S3's raw/diagnostic serial line only printed inside the confidence-gated branch, so it vanished the instant a detection dropped below `PS_CONF_GATE` — exactly the boundary data needed for range/gate tuning. `main.cpp` now logs every raw detection (`rawScore`/`conf`/`gate=PASS|REJECT`/`rawX`/`rawY`) regardless of gate outcome; the eye still only moves on a `PASS`.
- FIRMWARE_VERSION CG-S3 → CG-S4.

## CG-S5 (2026-07-06) — power fault fixed, confidence gate re-derived from vendor spec, distance floor documented

Same bench session, continued. Bench testing surfaced a narrow detection envelope (1-2ft / ±10-20°) and unstable lateral tracking; before more firmware tuning, traced both to real, sourced causes.

- **Power fault found and fixed:** SEN0626 VCC measured 2.6V under load vs. 3.25V at the Teensy's own 3.3V pin — the drop was in a bad connector along the run, not the Teensy regulator. Undervolting an active-inference sensor like this is a documented SEN0626 forum failure mode (random resets, frozen output, degraded detection). Fixed by reseating/direct-wiring; documented in 05_WIRING.md as a first-check item.
- **PS_CONF_GATE re-derived from DFRobot's own spec, not IRIS's:** CG-S3's default (45) was borrowed from IRIS's unrelated `psConfGate` constant and mapped to an effective SEN0626 score floor of only ~19/100. DFRobot's own setup guide states "a score >=60 is considered valid" and its sample code sets exactly that threshold. New default: `PS_CONF_GATE = 152` (`floor(60*255/100)-1`), so raw score 60 passes and 59 doesn't.
- **Detection-range floor documented:** DFRobot specs 0.5-3m (~19.7in-9.8ft) for both gesture and face recognition. The operator's bench observation of instability below ~15in is *below this documented floor* — out-of-spec operation, not a bug. Added to 05_WIRING.md (mounting guidance) and NOTES.md bench step 7b.
- **Person Sensor comparison — honest gap flagged, not fabricated:** no public FOV-in-degrees or minimum-working-distance spec was found for the Person Sensor to do an exact numeric comparison. Qualitatively, IRIS's own production history with the Person Sensor has never surfaced a "must stand back" complaint, while the SEN0626's 0.5m floor sits at typical close conversational distance — flagged in 09_IRIS_INTEGRATION_PLAN.md §6 as a real consideration for the deploy decision, not just a tuning task.
- **Open, handed off:** lateral (X-axis) tracking instability at close range needs re-testing now that power and the gate are fixed, before concluding anything further is wrong. See `11_HANDOFF_FABLE_LATERAL_TRACKING.md`.
- FIRMWARE_VERSION CG-S4 → CG-S5.
- **Status: DEPLOYED, not yet VERIFIED** — sensor detection, power, and gate are bench-confirmed; direction (3.1), NATIVE_H (3.5/step 6), and lateral tracking are still open per NOTES.md.
