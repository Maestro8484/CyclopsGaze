# CyclopsGaze → IRIS Integration Plan (PS-bus drop-in)

Written for a **later** IRIS deploy session. Nothing here is deployed or flashed by the session that wrote this doc (IRIS HANDOFF C, 2026-07-04). This is the architecture + step plan the operator approved; execute it only when the bench-verify prerequisites below are green and the operator says DEPLOY.

---

## 1. Decision on record (IRIS HANDOFF C, 2026-07-04)

Two choices were made with the operator before any code was written:

1. **Repo identity — CyclopsGaze is the durable gaze-node repo.** The old ESP32-S3 + OV2640 vision node (`../OGLE`) is tombstoned/archived (see its README + CHANGELOG banners). Research concluded Teensy 4.1 + DFRobot SEN0626 is substantially more reliable for face-tracking gaze than the ESP32 camera path, which never reached reliable end-to-end tracking at IRIS's real bench distance/lighting.
2. **Integration path — native Person-Sensor-bus drop-in.** CyclopsGaze's `SEN0626Sensor` shim is read *directly by IRIS's own Teensy 4.1*, standing in the slot the dead Useful Sensors Person Sensor (SEN-21231, RD-033) used to occupy. We are **NOT** using OGLE's old separate-node USB-CDC → Pi4 `ogle_bridge.py` → UDP `GAZE:` bridge. That bridge and its `OGLE,...` contract are retired, and the IRIS-side consumer code was already removed (IRIS S184 / S140).

Why the drop-in over the bridge: it is the simplest thing to **replicate publicly** (one sensor wired to the Teensy — no separate board, no Pi daemon, no udev symlink, no systemd unit), and it keeps gaze on the Teensy's own fast local loop instead of a multi-hop network path. CyclopsGaze was purpose-built for exactly this: `SEN0626Sensor` already emulates `person_sensor_face_t` and the Person Sensor method surface byte-for-byte.

---

## 2. The contract being emulated (already satisfied)

`src/sensors/SEN0626Sensor.h` presents the **identical** interface IRIS's `src/sensors/PersonSensor.h` exposes. Confirmed byte-for-byte drop-in (CG-S2, NOTES.md):

- Struct `person_sensor_face_t` (packed): `box_confidence, box_left, box_top, box_right, box_bottom, id_confidence, id, is_facing` — identical layout.
- Method surface: `begin() / isPresent() / read() / enableID() / setMode() / enableLED() / numFacesFound() / faceDetails(i) / timeSinceFaceDetectedMs()` — identical signatures. The no-op methods (`enableID`, `setMode`, `enableLED`) keep IRIS call sites compiling and behaving.

So at the **software** layer, IRIS's existing `reportFaceState()` loop (`src/main.cpp`, the `personSensor.read()` → iterate `faceDetails(i)` → `setTargetPosition()` block) works against the shim unchanged. The real integration work is the **physical bus** difference plus a few IRIS-side edits, below.

---

## 3. The one real architectural change: I2C → UART

This is the crux and must not be glossed. The two sensors sit on **different physical buses**:

| | IRIS Person Sensor (today) | CyclopsGaze SEN0626 (drop-in) |
|---|---|---|
| Bus | I2C (`Wire`) | UART Modbus RTU (`Serial1`) |
| T4.1 pins | SDA 18 / SCL 19 | RX 0 / TX 1 (cross: sensor TX→pin 0, sensor RX→pin 1) |
| Construction in IRIS | `PersonSensor personSensor(Wire)` (`main.cpp:116`) | `SEN0626Sensor personSensor(Serial1)` |
| Address / protocol | I2C 0x62 | Modbus device 0x72, FC04 input regs, 9600 baud (confirm on bench) |
| Recovery path | `psI2cBusRecover()` — clock-out SDA + STOP + `Wire` re-init (`main.cpp` ~438–496) | N/A — UART has no SDA-latch failure mode |

The "drop-in" is at the **interface/struct** level, not the pin level. Wiring changes, and the sensor object's transport changes from `Wire` to `Serial1`. Everything downstream of `faceDetails()` is untouched.

---

## 4. IRIS-side changes required (for the later deploy session)

Touch only what's listed. All paths are in the IRIS repo (`C:\Users\SuperMaster\Documents\PlatformIO\IRIS-Robot-Face`).

1. **Add the shim to IRIS sources.** Copy CyclopsGaze `src/sensors/SEN0626Sensor.{h,cpp}` into IRIS `src/sensors/`. Keep `PersonSensor.{h,cpp}` in-tree (do not delete — see §7 rollback).
2. **`src/main.cpp:14`** — `#include "sensors/SEN0626Sensor.h"` (in addition to, or replacing, the PersonSensor include).
3. **`src/main.cpp:116`** — swap the object: `SEN0626Sensor personSensor(Serial1);` in place of `PersonSensor personSensor(Wire);`. The variable name `personSensor` stays so every downstream call site is untouched.
4. **`begin()` / re-probe blocks (`main.cpp` ~490–544)** — the I2C-specific `Wire.begin(); Wire.setClock(100000);` calls and the `psI2cBusRecover()` invocation become inert for a UART sensor. Guard or remove them on the SEN0626 path (the shim's `begin()` owns `Serial1` bring-up + baud auto-detect). **Do not** delete `psI2cBusRecover()` itself while `PersonSensor` remains in-tree for rollback — just don't call it on the SEN0626 path.
5. **`src/config.h`** — `USE_PERSON_SENSOR` stays the master enable; bump `FIRMWARE_VERSION` to the deploy session tag before flashing (per IRIS CLAUDE.md).
6. **Nothing on the Pi4.** No bridge, no udev, no service, no WebUI route. This is the whole point of the drop-in. (The IRIS S184 cleanup already removed the old OGLE Pi4 surface.)

---

## 5. Behavioral gaps + how each is handled

| Gap | Detail | Resolution |
|-----|--------|-----------|
| **`is_facing` has no SEN0626 equivalent** | Shim hardcodes `is_facing = 1` (NOTES.md §"No is_facing field"). | **Non-blocking for live IRIS:** `psFacingRequired` defaults **false** since IRIS S153c (`main.cpp:146` — the real PS `is_facing` bit flickered and dropped locks, so facing-gating was turned off as the durable fix). With FACING off, IRIS never reads `is_facing`, so hardcoded-true is harmless. **If** the operator ever re-enables `PS_CFG:FACING=1`, derive a real facing flag from SEN0626 head-pose/gesture registers first — otherwise every detection counts as "facing." |
| **Confidence scale** | Shim emits `box_confidence = score × 255 / 100` (0..255). IRIS gates with `face.box_confidence > psConfGate`, and **CONF=60 is the operator's long-standing intentional setting** (memory `project_ps_conf_operator_setting`; S157b was wrong to change it). | **Bench-verify before trusting the gate.** Confirm the real Person Sensor's `box_confidence` was on the same 0..255 scale the shim emits. If the real PS reported 0..100, then CONF=60 was calibrated against 0..100 and the shim's ×255/100 rescale would make the gate far too permissive. Fix by matching the shim's output scale to whatever CONF=60 was calibrated against — **do not** silently change CONF; confirm with the operator (per `project_ps_conf_operator_setting`). |
| **Coordinate mapping / flips** | Shim maps SEN0626 640×480 → 0..255 box space (`cx = faceX·255/640`, `cy = faceY·255/480`) storing center in both box edges. IRIS `main.cpp` computes `targetX/targetY` from box coords with its own sign/mirror. | Bench-verify direction: a face at image-left must pull the eyes the correct way (L/R **and** U/D). If inverted, flip the sign in IRIS `main.cpp` target math (same knob the old PS used), not in the shim. Also confirm SEN0626 native Y resolution (480 assumed, unverified — NOTES.md). |
| **LED liveness** | Shim `enableLED()` is a no-op; SEN0626 has no PS-style status LED. | Cosmetic only. IRIS PSLED behavior (`main.cpp:398/507`) becomes a no-op; no functional impact. |
| **`timeSinceFaceDetectedMs` / lost-timeout** | Shim tracks this and IRIS uses it for the auto-move-resume timeout (`main.cpp:597`). | Works as-is; confirm the ~3 s resume feel on the bench and tune `psLostMs` if needed. |

---

## 6. Prerequisites before any IRIS deploy (gate)

CyclopsGaze is still **REPO-ONLY** — CG-S3 builds clean for T4.1 (single-eye and
`-DDUAL_EYE`) but has **not** been flashed or bench-verified (no T4.1 was
connected during CG-S1/S2/S3). The gate stays **RED** until the bench pass runs.

**What CG-S3 already unblocked (code-side, no bench needed):**
- The tracking-chain **audit is done** and fixes landed (NOTES.md §"CG-S3 Audit").
  The Y-direction question is resolved *in firmware*: CyclopsGaze's sign
  convention is byte-for-byte IRIS's production `main.cpp` on the identical
  EyeController, so the mapping/render logic is confirmed correct — the only
  remaining direction question is the SEN0626's physical Y-axis orientation, now
  a single documented bench flip rather than a debugging expedition.
- The **confidence-scale check is now turnkey.** The shim exposes raw score and
  the firmware logs it next to the rescaled `box_confidence` under
  `CG_CALIB_RAW`; CyclopsGaze also now gates with `PS_CONF_GATE` (default 45,
  matching IRIS S153c), so the bench directly measures the exact quantity the
  IRIS gate will consume. (Note: IRIS's own gate default is 45 since S153c — the
  §5 table's "CONF=60" reference predates that; verify against the live setting.)
- The **NATIVE_H (480 vs 640)** and **direction** checks have an explicit,
  copy-pasteable procedure — NOTES.md "Flash & Verify — Bench Protocol" steps
  5-7. This replaces the old placeholder.
- Modbus timeout, baud-boot robustness, and the autoMove-resume path are
  hardened/verified in code (audit 3.2/3.6/3.9), removing three fragility risks
  before the sensor is ever trusted on the IRIS bus.

**Still RED — must be VERIFIED on the bench before touching IRIS** (NOTES.md bench protocol):

- [ ] Flash CG-S3 to the spare T4.1; confirm `[CG] CyclopsGaze CG-S3` on serial (step 2).
- [ ] Confirm SEN0626 enumerated + baud (`found at 9600` vs `115200` — record which) (step 3).
- [ ] Confirm native Y resolution (480 vs 640) via `rawY` max (step 6).
- [ ] Confirm `[CG] faces=1 conf=.. x=.. y=..` on a real face; eye tracks L/R/U/D
      (flip targetY sign only if vertical inverted); AutoMove resumes ~3 s after
      the face leaves frame (steps 4/5/8).
- [ ] **Confidence-scale check (§5):** capture raw SEN0626 `score` vs the shim's
      emitted `box_confidence`; confirm/tune `PS_CONF_GATE` against the live IRIS
      CONF setting before trusting the gate (step 7).

Deploying an unproven sensor design into the live IRIS T4.1 is exactly the "refactor of an unproven design = wasted motion" the handoff warns against. **Live IRIS keeps the working Person Sensor as source of truth until CyclopsGaze is VERIFIED on the bench** (NOTES.md posture; memory `person_sensor_irreplaceable`).

---

## 7. Staged rollout + rollback

**Staging:** do the IRIS-side edits on a branch, flash the **spare** T4.1 (not the installed one), bench the whole IRIS eyes loop against SEN0626 off-robot, and only then schedule the in-enclosure sensor swap during a maintenance window with the operator present.

**Rollback (fast):** because `PersonSensor.{h,cpp}` and `psI2cBusRecover()` stay in-tree, reverting is a one-object change back to `PersonSensor personSensor(Wire)` + reflash + re-land the I2C sensor. Keep the removed Person Sensor labeled and boxed — it is a discontinued, load-bearing, recover-at-all-costs part (`person_sensor_irreplaceable`); the SEN0626 is insurance/replicability, not a reason to scrap a working PS.

---

## 8. Public-replicability note (why this path serves the launch)

For the IRIS public launch, a buyer cannot source the discontinued SEN-21231. The drop-in makes the replacement recipe: "wire one DFRobot SEN0626 to the Teensy (4 wires, §05_WIRING), flash the IRIS firmware built with the SEN0626 sensor object." No ESP32 node, no Pi bridge, no udev/systemd. That is the single simplest publishable gaze path — and the reason the OGLE bridge architecture was retired rather than folded forward.
