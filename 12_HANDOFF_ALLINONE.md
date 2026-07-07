# CyclopsGaze — All-In-One Handoff (paste this as your first message)

You are picking up a live embedded-firmware bench session, mid-debug, on a
physically-connected Teensy 4.1. This document is self-contained — you should
be able to act correctly from this alone, but the repo is the source of truth
if anything here goes stale. Read `01_CYCLOPSGAZE_RULES.md` in full before
your first edit; the summary below is not a substitute for it.

---

## 0. Hard rules (from 01_CYCLOPSGAZE_RULES.md — do not skip)

- **Repo:** `C:\Users\SuperMaster\Documents\PlatformIO\CyclopsGaze` — live files only, never from memory. Read a file before editing it.
- **Status terms:** REPO-ONLY (written, not flashed) / DEPLOYED (flashed) / VERIFIED (flashed + behavior confirmed on bench). Nothing is "done" until VERIFIED. Never claim VERIFIED without observing it yourself on serial/the display.
- **Bump `FIRMWARE_VERSION` in `src/config.h` before every flash.** Confirm the version string on the serial boot line after flashing.
- **One logical change per commit.** Update NOTES.md with actual findings before closing the session — never defer it.
- **Scope:** standalone testbed only. Touch only this repo — `IRIS-Robot-Face` (`C:\Users\SuperMaster\Documents\PlatformIO\IRIS-Robot-Face`) is a **read-only** reference, never edit it from here.
- Don't add features beyond the task at hand.

---

## 1. Hardware + current firmware state

- **Board:** Teensy 4.1, connected via USB on **COM6**.
- **Sensor:** DFRobot SEN0626 (Gravity AI gesture/face camera), wired over `Serial1` (Modbus RTU, UART, device addr 0x72), TX→Teensy pin 0, RX→Teensy pin 1 (crossed).
- **Display:** one GC9A01A 240×240 round LCD, single-eye config (dual-eye exists behind `#define DUAL_EYE` in `config.h`, not in use).
- **Firmware:** `FIRMWARE_VERSION = "CG-S5"` (in `src/config.h`), **flashed to the board right now**. Status: **DEPLOYED, not VERIFIED.**
- Reflash command: `pio run -e cyclopsgaze -t upload --upload-port COM6` (from the repo root). Monitor: `pio device monitor -p COM6 -b 115200` (open this BEFORE resetting the board, or you'll miss the boot line — Teensy boots faster than the monitor usually attaches).

## 2. What was fixed this session (do not re-investigate these)

1. **SEN0626 "NOT FOUND" → root cause was the sensor's onboard I2C/UART DIP switch left in I2C mode.** This firmware is UART-only, no I2C fallback. Fixed by flipping the switch to UART. No code involved. Documented in `05_WIRING.md`.
2. **Power fault:** SEN0626 VCC measured **2.6V** under load vs. **3.25V** at the Teensy's own 3.3V pin. The Teensy's regulator was healthy — the drop was in a bad connector along the wire run. Fixed by reseating/direct-wiring. Undervolting is a documented SEN0626 forum failure mode (random resets, frozen output, degraded detection) — if anything looks flaky again, check this again before touching firmware.
3. **Calibration logging was gated (bug):** the per-detection serial log used to only print *inside* the confidence-gate-passed branch, so the instant a detection dropped below the gate, the log line vanished — exactly the data needed to tune the gate. Fixed: now logs every detected face, gate outcome and all (see the exact log format in §4 below).
4. **Confidence gate was wrong:** `PS_CONF_GATE` was borrowed from IRIS's own unrelated `psConfGate=45`, which mapped to accepting SEN0626 raw scores as low as ~19/100. DFRobot's own docs (`wiki.dfrobot.com/sen0626/docs/23024`) say **"a score >=60 is considered valid"** and their sample code sets exactly that threshold. Fixed: `PS_CONF_GATE` is now **152** (`floor(60*255/100)-1` — so raw score 60 passes, 59 doesn't).

## 3. The open problem — this is the actual task

**Operator report:** lateral (X-axis, left-right) tracking is unreliable — jittery/wrong. **Vertical (Y-axis, up-down) tracking is fine.** It's worse the closer the face is to the sensor; specifically, closer than ~15 inches throws it off. It improved noticeably after the power fix (#2 above) but was not confirmed fully resolved.

**Critical context:** DFRobot documents the SEN0626's detection range as **0.5–3 meters (~19.7in – ~9.8ft)** for both gesture and face recognition — this is the vendor's own spec, not a bug in this repo. **15 inches is below that documented floor.** So before assuming there's a firmware bug: some or all of the original complaint may simply have been (a) the now-fixed power fault, and/or (b) operating below the sensor's documented range. **This has not been re-tested since the power fix and gate fix, at a proper in-spec distance.** Do that first.

### Step 1 — re-test before touching any code

1. Confirm SEN0626 VCC still reads ~3.2–3.3V (a connector can work loose again).
2. `pio device monitor -p COM6 -b 115200`, reset the board, confirm boot shows `[CG] CyclopsGaze CG-S5` and the sensor is found (`[CG] SEN0626 found at ...`).
3. Stand at **24–36 inches** (safely inside the documented 0.5–3m range):
   - Hold still, centered, for a few seconds. Watch `rawX` in the log. **Steady, or jittering while genuinely motionless?**
   - Move slowly left-right at that same distance. Does the eye track smoothly, or lag/overshoot/jitter?
   - Repeat at 12–18 inches (still somewhat below spec) and compare — meaningfully worse than at 24–36in?

Paste back a few representative log lines from each condition before deciding anything.

### Step 2 — how to read the result

| Symptom (at ≥20in, power confirmed good) | Cause | Fix |
|---|---|---|
| `rawX` steady while still, moves cleanly when you move | Original complaint was the power fault + below-spec distance. Nothing left to fix. | Close this out — update NOTES.md, mark resolved. |
| `rawX` jitters/bounces even while standing still and centered | Real sensor-side X-axis noise, independent of distance/power. | Add a low-pass filter on `targetX` — see §5. |
| `rawX` pins near 0 or 640 when roughly centered, especially close-up | Face exceeds the sensor's usable frame/model input at that distance — clipping, not noise. | Document as a hardware/distance limitation in `05_WIRING.md`, not a firmware fix. |

### Step 3 (if noise) — the fix pattern already proven elsewhere in this project family

`IRIS-Robot-Face/docs/servo_teensy40_wiring.md` documents that IRIS's own pan-servo hit this exact class of problem (jittery tracking input) and fixed it with a low-pass filter: `filteredPan`, `PAN_FILTER_ALPHA = 0.15`, smoothing over ~130ms. Same pattern applies here if — and only if — Step 1/2 confirms real noise (don't add filtering to mask a distance/power problem; that just hides the real issue).

Sketch, in `src/main.cpp` (current file content is reproduced in §4 below so you can see exactly where this goes):
```cpp
static float filteredTargetX = 0.0f;
constexpr float X_FILTER_ALPHA = 0.15f;  // start here, tune on bench
// ...
float rawTargetX = -((cx / 127.5f) - 1.0f) * GAZE_GAIN;
filteredTargetX = filteredTargetX * (1.0f - X_FILTER_ALPHA) + rawTargetX * X_FILTER_ALPHA;
eyes->setTargetPosition(filteredTargetX, targetY);
```
Don't filter Y — operator reports it's already clean; smoothing it adds lag for no benefit. Make the alpha a named constant in `config.h` alongside `PS_CONF_GATE`/`GAZE_GAIN`/`FACE_LOST_MS` so it's bench-tunable without a full edit each time.

**Untested hypothesis worth probing, not vendor-confirmed:** people naturally turn their head in **yaw** (left/right) more than **pitch** (up/down) when following something laterally with their face, and a frontal-trained face detector's centroid estimate may be more sensitive to yaw than pitch — which would show up as exactly this X-specific noise, worse at close range where angular resolution per unit of head movement is coarsest. If you can distinguish "jitter while facing forward and still" from "jitter that only appears when the head is turned to track laterally," that's more diagnostic than anything in DFRobot's docs.

## 4. Exact current source (so you don't have to trust a description)

**`src/main.cpp`** (full file, current state, CG-S5):
```cpp
#include <Arduino.h>
#include "config.h"
#include "sensors/SEN0626Sensor.h"

SEN0626Sensor sensor(Serial1);

void setup() {
  Serial.begin(115200);
  uint32_t deadline = millis() + 2000;
  while (!Serial && millis() < deadline) {}
  Serial.printf("[CG] CyclopsGaze %s\n", FIRMWARE_VERSION);

  sensor.begin();

  randomSeed(analogRead(A0));
  initEyes(true, true, true);
  eyes->setTargetPupil(0.40f, 300);
}

void loop() {
  bool sampled = sensor.read();

  // Gate on confidence (face.box_confidence > PS_CONF_GATE). CG-S5: PS_CONF_GATE
  // is now DFRobot's own documented validity floor for this sensor (raw score
  // >=60, wiki.dfrobot.com/sen0626/docs/23024) translated to our 0-255 scale --
  // not an arbitrary/IRIS-borrowed number. Tune further only against real bench
  // scores (audit 3.7, NOTES.md "External research").
  bool anyFace = sampled && sensor.numFacesFound() > 0;
  person_sensor_face_t f = anyFace ? sensor.faceDetails(0) : person_sensor_face_t{};
  bool haveFace = anyFace && f.box_confidence > PS_CONF_GATE;

#if CG_CALIB_RAW
  // Bench range/angle tuning (S4): log EVERY raw detection the sensor reports,
  // gate-passed or not. Logging only inside the gated branch (CG-S3) meant the
  // line vanished the instant a detection dropped below PS_CONF_GATE -- exactly
  // the boundary data needed to tell "gate too strict" from "sensor's own
  // detection envelope ended" apart. gate=PASS/REJECT makes that call visible
  // without touching the eye (untracked detections never call setTargetPosition).
  if (anyFace) {
    Serial.printf("[CG] faces=%d rawScore=%u conf=%d gate=%s | rawX=%u rawY=%u\n",
                  sensor.numFacesFound(), sensor.rawScore(), f.box_confidence,
                  haveFace ? "PASS" : "REJECT", sensor.rawFaceX(), sensor.rawFaceY());
  }
#endif

  if (haveFace) {
    // box_left==box_right==cx and box_top==box_bottom==cy (center-only box), so
    // these are the exact face center. Sign convention is byte-for-byte IRIS's
    // production main.cpp (targetX negated for the eye-0 X-mirror; targetY NOT
    // negated) -- verified correct, not inverted (audit 3.1). GAZE_GAIN scales
    // the tracking range for bench tuning; setTargetPosition clamps to the unit
    // circle so overshoot past +/-1 is safe (audit 3.8).
    float cx = f.box_left;
    float cy = f.box_top;
    float targetX = -((cx / 127.5f) - 1.0f) * GAZE_GAIN;
    float targetY =  ((cy / 127.5f) - 1.0f) * GAZE_GAIN;

    eyes->setAutoMove(false);
    eyes->setTargetPosition(targetX, targetY);

#if CG_CALIB_RAW
    Serial.printf("[CG]   -> tracking x=%.2f y=%.2f\n", targetX, targetY);
#endif

  } else if (sensor.timeSinceFaceDetectedMs() > FACE_LOST_MS) {
    // Runs whether read() returned false (throttle / comms failure) or returned
    // true with no qualifying face, so autoMove always resumes and never sticks
    // OFF when the sensor stops responding (audit 3.2).
    eyes->setAutoMove(true);
  }

  eyes->renderFrame();
}
```

**`src/config.h`** tunables block (current state, CG-S5 — full file also exists at `src/config.h`, this is the relevant part):
```cpp
static constexpr uint8_t PS_CONF_GATE = 152;   // derived from DFRobot's score>=60 floor
static constexpr float GAZE_GAIN = 1.0f;       // eye travel range multiplier, not a detection-range knob
static constexpr unsigned long FACE_LOST_MS = 3000;
#define CG_CALIB_RAW 1                          // bench logging, leave ON until VERIFIED
```

Sensor internals (raw accessors used by the calibration log) live in `src/sensors/SEN0626Sensor.h` / `.cpp` — `rawFaceX()/rawFaceY()/rawScore()` return the pre-remap register values.

## 5. Sourced research — already done, don't re-fetch unless you doubt it

From DFRobot's own wiki/forum and a Person Sensor datasheet lookup, this session:
- **Detection range:** 0.5–3m (~19.7in–9.8ft), same number for gesture and face detection. Source: `wiki.dfrobot.com/sen0626/` and the setup guide.
- **FOV:** 85° **diagonal only** — DFRobot never publishes a horizontal/vertical split. There is no vendor number to compute exact degrees-per-pixel on either axis. (This is the same kind of gap we already knew about for `NATIVE_H` — DFRobot documents the face_x range as 0-640 but never states face_y's range; NOTES.md still flags `NATIVE_H=480` as assumed/unconfirmed.)
- **Confidence:** DFRobot's own recommended validity floor is raw score ≥60/100 (used to derive `PS_CONF_GATE=152` above).
- **Known real-world failure modes** (DFRobot forum, `dfrobot.com/forum/topic/401101`): dominated by power-supply instability and wiring/comms issues, not general sensor unreliability — matches this session's own power-fault finding exactly.
- **Person Sensor (SEN-21231) comparison — honest gap:** no public FOV-in-degrees or minimum-working-distance spec was found. The SparkFun/DigiKey "datasheet" PDF is a one-page product blurb, not a technical spec sheet. **Do not present a precise numeric comparison as fact.** Qualitatively: IRIS's own production history with the Person Sensor has never surfaced a "must stand back" complaint, while the SEN0626 has a hard documented floor sitting right at typical close conversational distance. If IRIS's real interaction distance is often under ~20 inches, that's a genuine, sourced downgrade for that specific use case — flagged in `09_IRIS_INTEGRATION_PLAN.md` §6 as a deploy-decision consideration, not just a tuning gap.

## 6. Files you'll touch or should check

- `src/main.cpp`, `src/config.h` — reproduced in full above; edit here if a filter is needed.
- `src/sensors/SEN0626Sensor.h` / `.cpp` — raw sensor driver, probably doesn't need changes for this task.
- `NOTES.md` — "External research" section and bench step "7b" have the full sourced detail; "Issues Found" / "Next Session" checklists track resolved-vs-open state — update these when you resolve something.
- `05_WIRING.md` — DIP-switch note, power-fault note, and the new "Mounting Distance" section.
- `09_IRIS_INTEGRATION_PLAN.md` §5/§6 — confidence gate + distance-floor considerations for the eventual IRIS deploy decision.
- `11_HANDOFF_FABLE_LATERAL_TRACKING.md` — an earlier, more procedural version of this same handoff (this document, `12_HANDOFF_ALLINONE.md`, supersedes it for onboarding purposes — both describe the same open problem).
- `CHANGELOG.md` — CG-S1 through CG-S5 history, append your session's entry at the bottom when done.

## 7. Session-close checklist (from 01_CYCLOPSGAZE_RULES.md)

- [ ] `pio run -e cyclopsgaze` clean build confirmed after any code change.
- [ ] `FIRMWARE_VERSION` bumped in `src/config.h` before any reflash (currently CG-S5 — your change would be CG-S6).
- [ ] Reflashed to COM6, boot version string confirmed on serial monitor.
- [ ] NOTES.md updated with **actual bench findings**, not placeholders — resolve or update the open items this doc lists.
- [ ] One logical commit per change (see recent git log for the established message style: `CG-S#: <what and why>`).
- [ ] State the real hardware status at close: DEPLOYED vs VERIFIED, and exactly which NOTES.md checklist items are now confirmed vs still open.
- [ ] Do not touch `IRIS-Robot-Face` (read-only reference). Do not expand scope beyond the lateral-tracking diagnosis/fix.
