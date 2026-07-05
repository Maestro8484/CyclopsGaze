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
