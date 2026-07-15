# CyclopsGaze → IRIS Integration Plan (PS-bus drop-in)

Written for a **later** IRIS deploy session. Nothing here is deployed or flashed by the session that wrote this doc (IRIS HANDOFF C, 2026-07-04). This is the architecture + step plan the operator approved; execute it only when the bench-verify prerequisites below are green and the operator says DEPLOY.

> **CG-S8 update (2026-07-07) — code-review pass against live IRIS, both consumers.**
> CyclopsGaze tracking is now **VERIFIED** on the bench, and the integration was
> code-reviewed against IRIS's actual source. Key results folded into the sections
> below:
> - **There are TWO consumers, not one.** T4.1 eyes (`PersonSensor` class, X+Y) AND
>   the T4.0 servo head-pan node (`setupPersonSensor()`/`pollPersonSensor()` →
>   `PersonResult`, X-only). The class shim only covers the eyes; the servo needs
>   its own adapter — now provided in `integration/servo_teensy40_base_mount/`.
> - **Eyes method surface confirmed sufficient:** IRIS calls only
>   `isPresent/read/setMode/enableID/enableLED/faceDetails/numFacesFound/timeSinceFaceDetectedMs`
>   — all present. `isPresent()` now lazily runs `begin()`, so IRIS's probe loop
>   needs no added `begin()` call.
> - **Do NOT propagate CyclopsGaze's CG-S6 targetX change or CG-S8 `Y_CENTER` to
>   IRIS.** They are single-eye / bench-mount specific. IRIS keeps its own targetX
>   negation and its runtime `psYBias` (§4, §5).
> - **Confidence scale is the one real behavior decision (§5).** IRIS's `psConfGate`
>   is constrained 0–100 but compared to `box_confidence`; the shim emits 0–255.
> - Ready-to-copy files + per-consumer steps: `integration/README.md`.

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

**Method-surface audit (CG-S8, verified against live IRIS `src/main.cpp`).** IRIS
calls exactly these on `personSensor`, and all are present in the shim:
`isPresent` (×2), `read`, `setMode` (×2), `enableID` (×2), `enableLED` (×2),
`faceDetails` (×2), `numFacesFound` (×2), `timeSinceFaceDetectedMs` (×2). It does
**not** call `singleShot/labelNextID/persistIDs/eraseIDs`, so the shim not having
those is harmless. The struct `person_sensor_face_t` is byte-identical (same 8
fields incl. `is_facing`). **One gap closed:** the real `PersonSensor` has no
`begin()` — IRIS brings the sensor up by calling `isPresent()` in a probe loop
(`main.cpp` ~498/539). The shim's `isPresent()` now lazily runs `begin()` (UART
bring-up + baud auto-detect) until the sensor answers, so that loop works with
**no added `begin()` call**. Once present it short-circuits.

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

**Line numbers verified CG-S8:** `main.cpp:14` (`#include "sensors/PersonSensor.h"`) and `main.cpp:116` (`PersonSensor personSensor(Wire);`) are still exact. The target math is `main.cpp:594-596`.

> **⚠ Do NOT copy CyclopsGaze's CG-S6 / CG-S8 tracking edits into IRIS.**
> IRIS's eyes math (`main.cpp:594-596`) is:
> ```cpp
> float targetX = -((box_left + (box_right-box_left)/2.0f)/127.5f - 1.0f);   // negated
> float targetY =  ((box_top  + (box_bottom-box_top)/3.0f)/127.5f - 1.0f) + psYBias;
> ```
> - CG-S6 **removed** the `targetX` negation in *CyclopsGaze* because its single
>   eye is always `eyeIndex==0` and `EyeController::renderFrame()` unconditionally
>   flips `eye.x` for eye 0 — a double flip. IRIS's eye config keeps the negation
>   as its own field-proven convention. **Leave IRIS's negation in place** and
>   bench-verify L/R direction on the actual IRIS eyes; flip in IRIS `main.cpp`
>   only if that bench check shows it inverted.
> - CG-S8 added `Y_CENTER=33` in CyclopsGaze because its sensor is mounted below
>   the eye. IRIS already has the equivalent knob: **`psYBias`** (runtime, via
>   `PS_CFG:YBIAS`). Since SEN0626 returns a center-only box, IRIS's
>   `(box_bottom-box_top)/3.0f` term is 0, so `targetY = (cy/127.5 - 1) + psYBias`.
>   Trim `psYBias` on the bench for the sensor's mount height — no code change.

---

## 4b. Second consumer: the T4.0 servo head-pan node

The eyes are not the only thing the dead Person Sensor drove. The servo base-mount
Teensy 4.0 (`servo_teensy40/teensy40_base_mount/`) reads its **own** Person Sensor
to pan the head toward a face. It does **not** use the `PersonSensor` class — it
has a separate C-style driver `person_sensor.{h,cpp}` exposing:

```cpp
struct PersonResult { bool ok; bool faceVisible; float faceCenterX; uint8_t confidence; bool isFacing; };
void setupPersonSensor();
PersonResult pollPersonSensor();
```

and the `.ino` consumes it (verified `teensy40_base_mount.ino:105-110`):

```cpp
PersonResult ps = pollPersonSensor();
if (!ps.ok) return;                       // short read / throttle -> hold pan
if (ps.faceVisible) updatePanFromFace(ps.faceCenterX);   // faceCenterX in 0-255
```

So the class shim does **not** cover this node. A matching SEN0626 adapter is
provided: **`integration/servo_teensy40_base_mount/person_sensor.{h,cpp}`**. It
reimplements `setupPersonSensor()`/`pollPersonSensor()`/`setPersonSensorLed()`
on top of `SEN0626Sensor` (Serial1), preserving the `PersonResult` contract:

- `ok` = a fresh sample was taken (SEN0626's internal 150 ms throttle → `ok=false`
  between samples → the `.ino` holds pan, exactly the old short-read behavior).
- `faceCenterX = (box_left+box_right)/2` — identical formula, same 0-255 space; the
  center-only box makes it the exact center.
- Gate: `box_confidence > 152` (raw score ≥60, DFRobot floor). The old driver
  gated `boxConfidence < 60` on the Useful Sensors 0-255 scale (~24%); the new
  gate is stricter and vendor-derived. Facing is always satisfied (no SEN0626
  facing bit; matches IRIS `psFacingRequired=false`).

**Servo-side edits (deploy session):**
1. Copy `src/sensors/SEN0626Sensor.{h,cpp}` into `servo_teensy40/teensy40_base_mount/`.
2. Replace that folder's `person_sensor.{h,cpp}` with the two `integration/` files.
   The `.ino` is unchanged.
3. Wire SEN0626 to the T4.0's `Serial1`. Leave the `.ino`'s `Wire.begin()` — the
   paj7620 gesture sensor still uses I2C; only the Person Sensor moves to UART.
4. Bench-verify pan direction (a face at image-left must pan the head the correct
   way). Direction lives downstream in `updatePanFromFace()` and is **not** flipped
   in the adapter — fix it there if inverted, same as the old sensor.

**Hardware confirmed (operator, 2026-07-07):** IRIS has **two** physical Person
Sensors — one on the T4.1 eyes node, one on the T4.0 servo/head node — both mounted
on the platform (eyes + TFT mouth) facing outward-front. Each node reads its own
sensor independently, exactly as the code shows. So the swap is **two independent
1:1 replacements**: each Teensy gets its own SEN0626 on its own `Serial1`. This is
precisely what the kit assumes — no shared-sensor arbitration needed. (Both sensors
being co-located and front-facing, their coordinate/direction bench-verifies can be
done together, but each node still tunes its own sign/bias/pan independently.)

## 5. Behavioral gaps + how each is handled

| Gap | Detail | Resolution |
|-----|--------|-----------|
| **`is_facing` has no SEN0626 equivalent** | Shim hardcodes `is_facing = 1` (NOTES.md §"No is_facing field"). | **Non-blocking for live IRIS:** `psFacingRequired` defaults **false** since IRIS S153c (`main.cpp:146` — the real PS `is_facing` bit flickered and dropped locks, so facing-gating was turned off as the durable fix). With FACING off, IRIS never reads `is_facing`, so hardcoded-true is harmless. **If** the operator ever re-enables `PS_CFG:FACING=1`, derive a real facing flag from SEN0626 head-pose/gesture registers first — otherwise every detection counts as "facing." |
| **Confidence scale (THE decision — verify first)** | Shim emits `box_confidence = score × 255 / 100` (0..255). CG-S8 read the live IRIS gate: `main.cpp:562` gates `face.box_confidence > psConfGate`, and `psConfGate` is **`constrain((int)val, 0, 100)`** (`main.cpp:393`) — capped at 100 — default 45 (`main.cpp:145`). So IRIS's gate can never exceed 100, while the shim's value ranges 0..255. A real SEN0626 detection (raw score 60–90 → box_confidence 153–229) sails past any 0..100 gate: **the operator's CONF knob becomes inert on the eyes.** | **This is the one behavior choice to make before deploy — recommendation: emit raw score (0..100) from the shim** so IRIS's 0..100 `psConfGate` works as intended and `CONF=60` means exactly DFRobot's "score ≥60" floor. That change also requires setting CyclopsGaze's own `PS_CONF_GATE` from 152→60 and a re-verify (it would de-VERIFY the current 0..255 build). The servo adapter side-steps this (it gates on the 0..255 value directly at 152). **Do not flip the scale silently** — it changes CyclopsGaze's verified build; confirm with the operator, then re-bench (per `project_ps_conf_operator_setting`). Until then the eyes effectively track any sensor-reported face, which is acceptable because SEN0626 only emits faces above its own internal floor. |
| **Coordinate mapping / flips** | Shim maps SEN0626 640×480 → 0..255 box space (`cx = faceX·255/640`, `cy = faceY·255/480`) storing center in both box edges. IRIS `main.cpp:594-596` computes `targetX` (negated) / `targetY` (+`psYBias`) with its own sign/mirror. | **Keep IRIS's existing negation + `psYBias`; do not import CyclopsGaze's CG-S6/S8 values** (§4 warning). Bench-verify direction on IRIS: a face at image-left must pull the eyes the correct way (L/R **and** U/D). If inverted, flip in IRIS target math, not the shim. Trim `psYBias` for mount height. CyclopsGaze itself bench-proved the shim's coords are sane (CG-S6/S7/S8 VERIFIED), so any residual is IRIS-side sign/bias only. Native Y res (480 assumed) still unconfirmed — NOTES.md; only matters for Y precision. |
| **LED liveness** | Shim `enableLED()` is a no-op; SEN0626 has no PS-style status LED. | Cosmetic only. IRIS PSLED behavior (`main.cpp:398/507`) becomes a no-op; no functional impact. |
| **`timeSinceFaceDetectedMs` / lost-timeout** | Shim tracks this and IRIS uses it for the auto-move-resume timeout (`main.cpp:597`). | Works as-is; confirm the ~3 s resume feel on the bench and tune `psLostMs` if needed. |

---

## 6. Prerequisites before any IRIS deploy (gate)

**CG-S8 (2026-07-07): CyclopsGaze tracking is VERIFIED on the bench.** The standalone
sensor+eye path is proven — lateral direction, gaze gain, and Y-center all
bench-confirmed on the T4.1 at COM6 (CHANGELOG CG-S6/S7/S8, NOTES.md). The remaining
gate items below are now **IRIS-side integration checks**, not CyclopsGaze unknowns.
The two drop-in adapters are code-reviewed against live IRIS source but are
**REPO-ONLY for IRIS** until a deploy session flashes and benches them.

**What CG-S3 already unblocked (code-side, no bench needed):**
- The tracking-chain **audit is done** and fixes landed (NOTES.md §"CG-S3 Audit").
  The Y-direction question is resolved *in firmware*: CyclopsGaze's sign
  convention is byte-for-byte IRIS's production `main.cpp` on the identical
  EyeController, so the mapping/render logic is confirmed correct — the only
  remaining direction question is the SEN0626's physical Y-axis orientation, now
  a single documented bench flip rather than a debugging expedition.
- The **confidence-scale check is now turnkey, and re-derived from the vendor's
  own spec.** CG-S5: `PS_CONF_GATE` is no longer IRIS's borrowed constant — it's
  DFRobot's own documented validity floor for this sensor (raw score >=60,
  wiki.dfrobot.com/sen0626/docs/23024) translated to box_confidence (152). The
  shim exposes raw score and the firmware logs it next to both the rescaled
  `box_confidence` and a PASS/REJECT gate verdict under `CG_CALIB_RAW`, so the
  bench directly measures the exact quantity IRIS's gate would consume.
- The **NATIVE_H (480 vs 640)** and **direction** checks have an explicit,
  copy-pasteable procedure — NOTES.md "Flash & Verify — Bench Protocol" steps
  5-7. This replaces the old placeholder.
- Modbus timeout, baud-boot robustness, and the autoMove-resume path are
  hardened/verified in code (audit 3.2/3.6/3.9), removing three fragility risks
  before the sensor is ever trusted on the IRIS bus.

**Progress this session (2026-07-06), T4.1 on bench at COM6:**
- [x] Flashed and enumerated; SEN0626 found (root-caused an onboard I2C/UART
  DIP switch left in I2C mode — fixed).
- [x] Found + fixed a power fault (2.6V at sensor VCC vs 3.25V at the Teensy
  pin — bad connector, not the regulator).
- [x] Confidence-scale check done, re-derived from DFRobot's own documented
  floor (`PS_CONF_GATE=152`, CG-S5) rather than IRIS's borrowed constant.

**New finding that bears directly on this gate decision:** DFRobot documents
the SEN0626's detection range as **0.5–3 m (~19.7in–~9.8ft)** for both gesture
and face recognition. No public FOV-in-degrees or minimum-working-distance spec
was found for the Person Sensor to compare numerically, but IRIS's own
production history with the Person Sensor has never surfaced a "must stand
back ~20 inches" complaint, while the SEN0626 has a hard documented floor
sitting right at typical close conversational distance. **If IRIS's real
interaction distance is often under ~20 inches, this is a genuine, sourced
downgrade for this specific use case** — not something tuning can fix — and
should be weighed before deploying, not discovered after. See NOTES.md
"External research" for the full sourcing.

**CyclopsGaze standalone — DONE (CG-S6/S7/S8, bench-verified 2026-07-07):**

- [x] `[CG] faces=1 ...` on a real face; eye tracks L/R (CG-S6 mirror fix) and
      U/D (CG-S8 Y-center); PS_CONF_GATE=152 confirmed passing on live scores 60–74.
- [x] Lateral (X) tracking resolved — was a mirror double-flip, not noise/distance.
- [ ] Native Y resolution (480 vs 640) still unconfirmed — non-blocking, only
      affects Y precision (NOTES.md).

**IRIS-side — must be bench-verified during the deploy session, on the SPARE Teensies:**

- [ ] **Confidence-scale decision (§5)** — pick raw-0..100 vs 0..255 emission and
      set the eyes `psConfGate` / shim accordingly; re-verify CyclopsGaze if the
      shim scale changes.
- [ ] **Eyes direction/bias** — keep IRIS's targetX negation; verify L/R + U/D on
      the real IRIS eyes; trim `psYBias` for mount height (do NOT import CG-S6/S8).
- [ ] **Servo pan** — copy the servo adapter; verify pan direction in
      `updatePanFromFace()`; confirm hold-pan on no-face.
- [ ] **AutoMove / lost-timeout** — eyes resume ~3 s after face leaves frame.
- [x] **Sensor count** — confirmed two sensors, one per Teensy (§4b); kit matches.

Deploying an unproven sensor design into the live IRIS T4.1 is exactly the "refactor of an unproven design = wasted motion" the handoff warns against. **Live IRIS keeps the working Person Sensor as source of truth until CyclopsGaze is VERIFIED on the bench** (NOTES.md posture; memory `person_sensor_irreplaceable`).

---

## 7. Staged rollout + rollback

**Staging:** do the IRIS-side edits on a branch, flash the **spare** T4.1 (not the installed one), bench the whole IRIS eyes loop against SEN0626 off-robot, and only then schedule the in-enclosure sensor swap during a maintenance window with the operator present.

**Rollback (fast):** because `PersonSensor.{h,cpp}` and `psI2cBusRecover()` stay in-tree, reverting is a one-object change back to `PersonSensor personSensor(Wire)` + reflash + re-land the I2C sensor. Keep the removed Person Sensor labeled and boxed — it is a discontinued, load-bearing, recover-at-all-costs part (`person_sensor_irreplaceable`); the SEN0626 is insurance/replicability, not a reason to scrap a working PS.

---

## 8. Public-replicability note (why this path serves the launch)

For the IRIS public launch, a buyer cannot source the discontinued SEN-21231. The drop-in makes the replacement recipe: "wire one DFRobot SEN0626 to the Teensy (4 wires, §05_WIRING), flash the IRIS firmware built with the SEN0626 sensor object." No ESP32 node, no Pi bridge, no udev/systemd. That is the single simplest publishable gaze path — and the reason the OGLE bridge architecture was retired rather than folded forward.

**CG-S10 update (2026-07-15) — the recipe can get physically smaller.** Bench-confirmed the SEN0626's camera lens/sensor is separable from the main Gravity board (ribbon cable + tape, not soldered); face detect/track worked with only the camera + ribbon connected, no head-shoulder LED / gesture RGB LED / IR-adjacent components required. That means the publishable recipe isn't stuck carrying the full Gravity board footprint — a buyer replicating IRIS's eyes can mount just the camera element, in a space closer to the original SEN-21231's tiny footprint, instead of the whole breakout board. **Not yet safe to publish as a confirmed recipe:** whether the detached camera still answers on the same I2C/UART register set / device address / baud once separated from the main board is unverified (see `NOTES.md` CG-S10) — do not add "detach the camera" to any public wiring instructions until that side-by-side register check is done.
