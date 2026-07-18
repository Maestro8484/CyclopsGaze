# CyclopsGaze → Handoff: Lateral (X-axis) Tracking Investigation

**Written:** 2026-07-06, end of the CG-S4/CG-S5 bench session.
**For:** whoever picks this up next (drafted for Claude Fable).
**Read 01_CYCLOPSGAZE_RULES.md first** — it governs this repo (live-repo-only,
read-before-edit, REPO-ONLY/DEPLOYED/VERIFIED discipline, bump
`FIRMWARE_VERSION` before every flash).

---

## 1. Where things stand

Firmware is **CG-S5**, flashed to the bench Teensy 4.1 on COM6. Status is
**DEPLOYED, not VERIFIED** — some things are bench-confirmed this session,
some are still open. Do not claim VERIFIED without observing it yourself.

**Fixed and bench-confirmed this session:**
- SEN0626 detection itself (root cause of an earlier "NOT FOUND" was the
  sensor's onboard I2C/UART DIP switch left in I2C mode — this firmware is
  UART-only, no fallback. Flipped to UART, fixed, no code change.)
- A power fault: SEN0626 VCC measured 2.6V under load vs. 3.25V at the
  Teensy's own 3.3V pin. Traced to a bad connector in the VCC run (not the
  Teensy's regulator, which tested healthy) — reseating/direct-wiring fixed it.
- `PS_CONF_GATE` was re-derived from DFRobot's own documented validity floor
  (raw score ≥60) instead of a borrowed IRIS constant. Now 152 (was 45).

**Still open — this is the actual task:**
The operator reports **lateral (X-axis, left-right) tracking is unreliable**,
while **vertical (Y-axis, up-down) tracking is fine**. It got noticeably
better after the power fix, but wasn't fully resolved. The operator also
reported the problem is worse at close range (**closer than ~15 inches**
throws it off) — but 15 inches is *below* the SEN0626's own documented
detection floor of ~19.7 inches (0.5m), so some or all of the original
complaint may simply have been out-of-spec operation. **This has not been
re-tested since the power fix, gate fix, and at a proper in-spec distance.**

## 2. What to do first — re-test before touching any code

Don't start changing firmware. Get fresh data first:

1. Confirm the sensor's VCC still reads ~3.2-3.3V (the power fix should be
   holding, but verify — a connector can work loose again).
2. Open `pio device monitor -p COM6 -b 115200`, reset, confirm boot shows
   `[CG] CyclopsGaze CG-S5` and the sensor is found.
3. Stand at **24-36 inches** (safely inside the documented 0.5-3m range) and:
   a. Hold still, centered, for a few seconds. Watch `rawX` in the serial log
      (format: `[CG] faces=1 rawScore=NN conf=NN gate=PASS|REJECT | rawX=NNN rawY=NNN`).
      **Is it steady, or jittering while you're not moving at all?**
   b. Move slowly left-to-right at that same distance, watching the eye and
      `rawX` together. Does the eye track smoothly, or does it lag/overshoot/jitter?
   c. Repeat closer (12-18 inches, i.e. still somewhat below spec but not
      extreme) and note whether it's meaningfully worse than at 24-36 inches.

Paste back a few representative log lines from each condition (steady-center,
moving, close-vs-far) before deciding anything.

## 3. How to interpret what you see

This determines which of three different problems you're actually looking at
— they have different fixes, don't jump to a fix before you know which one:

| Symptom | Likely cause | Fix direction |
|---|---|---|
| `rawX` steady while genuinely still, moves cleanly when you move, at ≥20in | Original complaint was mostly the now-fixed power fault + out-of-spec distance (<20in). Nothing more to do — document as resolved. | None — update NOTES.md and close this out. |
| `rawX` jitters/bounces even while standing still and centered at ≥20in | Real sensor-side noise in the face-centroid estimate, independent of distance/power. | Add a low-pass filter on `targetX` before `setTargetPosition()` — see §4. |
| `rawX` pins at an extreme (near 0 or 640) when you're roughly centered, especially up close | Face is exceeding the sensor's usable frame/model input at that distance — a clipping/saturation issue, not noise. | Document as a hardware/distance limitation (05_WIRING.md), not a firmware fix. |

## 4. If it's noise: the smoothing-filter fix

`IRIS-Robot-Face/docs/servo_teensy40_wiring.md` §"Servo Control Notes"
documents that IRIS's own pan-servo control already hit this exact class of
problem and fixed it with a **low-pass filter**: `filteredPan` at
`PAN_FILTER_ALPHA = 0.15`, smoothing direction reversals over ~130ms. That's
the proven pattern to reach for here, not a novel invention.

Sketch (in `src/main.cpp`, only if §3 confirms real noise):
```cpp
static float filteredTargetX = 0.0f;
constexpr float X_FILTER_ALPHA = 0.15f;  // start here, tune on bench
// ...
float rawTargetX = -((cx / 127.5f) - 1.0f) * GAZE_GAIN;
filteredTargetX = filteredTargetX * (1.0f - X_FILTER_ALPHA) + rawTargetX * X_FILTER_ALPHA;
eyes->setTargetPosition(filteredTargetX, targetY);
```
Don't filter Y unless it turns out to need it too — the operator reports Y is
already clean, adding unneeded smoothing there just adds lag for no benefit.
Make the alpha a named `config.h` constant (matching the existing
`PS_CONF_GATE`/`GAZE_GAIN`/`FACE_LOST_MS` pattern) so it's bench-tunable
without a rebuild-from-scratch each time.

**Before committing to this fix, also consider:** is X noisier than Y because
of something structural rather than random noise — e.g. does the operator
naturally turn their head (yaw) more than they tilt it (pitch) when following
something laterally, and would a frontal-trained face detector's centroid
estimate be more sensitive to yaw than pitch? This is untested reasoning (see
NOTES.md "External research," 3.7 section) — not confirmed by DFRobot's docs
or by any third-party source found this session. If you can distinguish
"jitter while genuinely still and facing forward" from "jitter that only
appears when the head is turned to track something lateral," that's more
diagnostic than either of us can get from vendor docs alone.

## 5. What's already sourced — don't re-research this

Already pulled from DFRobot's own docs and forum this session (full citations
in `NOTES.md` "External research — SEN0626 real-world specs" and 3.7):
- Detection range: 0.5-3m (~19.7in-9.8ft), same number for gesture and face.
- FOV: 85° diagonal only — DFRobot does not publish separate H/V numbers.
  There is no vendor number to compute exact degrees-per-pixel for either axis.
- DFRobot's own recommended validity threshold: raw score ≥60/100.
- Forum-documented failure modes are dominated by power/wiring, matching what
  was found and fixed this session.
- No public FOV-in-degrees or minimum-working-distance spec exists for the
  Person Sensor (SEN-21231) to make an exact numeric comparison — the
  "datasheet" PDF found is a one-page product blurb, not a technical spec
  sheet. Don't present a numeric FOV/range comparison as sourced fact; the
  honest comparison is qualitative (09_IRIS_INTEGRATION_PLAN.md §6).

Don't re-fetch these unless you have a specific reason to doubt them.

## 6. Session-close checklist (01_CYCLOPSGAZE_RULES.md)

- [ ] Clean `pio run -e cyclopsgaze` build confirmed after any code change.
- [ ] `FIRMWARE_VERSION` bumped (currently CG-S5) before any reflash.
- [ ] NOTES.md updated with **actual bench findings**, not placeholders —
      specifically resolve or update the "Open" items this doc lists.
- [ ] One logical commit per change.
- [ ] State the real hardware status at close: DEPLOYED vs VERIFIED, and which
      specific NOTES.md checklist items are now confirmed vs still open.
- [ ] If lateral tracking is resolved, dismiss/close this handoff doc's open
      items in NOTES.md rather than leaving them stale.

Do not touch the IRIS-Robot-Face repo (read-only reference). Do not expand
scope beyond lateral-tracking diagnosis/fix — no new features.
