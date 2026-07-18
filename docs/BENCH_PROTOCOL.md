# First-Flash Bench Protocol

A top-to-bottom checklist to bring up CyclopsGaze on real hardware, written so someone new to
PlatformIO can follow it. Each step: **action → expected serial → pass/fail.** Serial monitor at
**115200** baud.

> ⚠ **Re-running this is the #1 priority on the next flash.** The CG-S12 code sync (raw-score
> gate + per-axis gain/bias) compiles clean but has **not been re-observed on the bench** since
> the change. Steps 5–7 below are the ones that re-confirm it. Firmware in repo: **CG-S12** (bump
> `FIRMWARE_VERSION` in `src/config.h` before flashing if you change code).

## 0. Hardware first-checks (before anything else)

- SEN0626 **DIP switch on UART** (not I²C). This firmware has no I²C path — wrong mode reads as
  `NOT FOUND` with otherwise-correct wiring. This is the single most likely cause of a first-flash
  failure. Check it before re-wiring.
- Sensor's **own VCC pin ≈ 3.2–3.3 V under load** (measure at the sensor, not the Teensy pin).
- Faces will sit **≥ ~20 in (0.5 m)** from the lens (the sensor's documented range floor).

## 1. USB enumeration

```
pio device list
```
**PASS:** a Teensy port appears (e.g. `COM# … USB Serial (Teensy)`).
**FAIL:** none → check cable/board.

## 2. Flash + boot message

```
pio run -e cyclopsgaze -t upload
pio device monitor -b 115200
```
**Expect:** `[CG] CyclopsGaze CG-S12`
**PASS:** version line matches `config.h` `FIRMWARE_VERSION`.

## 3. SEN0626 detect (baud + PID)

**Expect within ~4 s of boot:** `[CG] SEN0626 found at 9600 (attempt 1)` (or `115200`). Record
which baud.
**FAIL:** `[CG] SEN0626 NOT FOUND at 115200 or 9600 -- check wiring` → **check the DIP switch
first** (step 0), then TX→pin0 / RX→pin1 cross, 3.3 V, GND.

## 4. Face detect (serial format)

Sit ~1 m in front of the sensor, facing it. With `CG_CALIB_RAW` on (default), expect:
```
[CG] faces=1 rawScore=NN conf=NN gate=PASS | rawX=NNN rawY=NNN
[CG]   -> tracking x=+0.NN y=+0.NN
```
`rawScore`/`conf` = DFRobot score 0–100 (CG-S12); `x`,`y` = gaze target in [−1,+1] after
gain/bias; `rawX/rawY` = sensor center 0–640 / 0–480(?).
**PASS:** `faces=1` with plausible values when a face is present; logging stops when you leave.

## 5. Direction verification (re-confirm after CG-S12)

Move slowly and check the eye **and** the serial signs:

| Stimulus (your position vs sensor) | Expected eye |
|---|---|
| Face to camera's **RIGHT** | eye looks **RIGHT** |
| Face to camera's **LEFT** | eye looks **LEFT** |
| Face toward **TOP** of frame | eye looks **UP** |
| Face toward **BOTTOM** of frame | eye looks **DOWN** |
| Face **centered** | eye straight ahead (x≈0, y≈0) |

**PASS:** eye follows you in all four directions.
- **If LEFT/RIGHT is mirrored:** flip the **sign** of `GAZE_X_GAIN` in `config.h` (+1.7 ↔ −1.7).
- **If UP/DOWN is inverted:** flip the sign of `GAZE_Y_GAIN`.
- **If the eye is biased off-center at neutral:** adjust `GAZE_X_BIAS` / `GAZE_Y_BIAS`
  (`Y_BIAS` compensates for the camera mounting below the eye).

## 6. `NATIVE_H` calibration (480 vs 640)

Keep a face in frame; move to the very top then very bottom, watching `rawY`. Note the **maximum**
`rawY`. ≈480 → leave `NATIVE_H=480`; ≈640 → set `NATIVE_H=640` in `SEN0626Sensor.h` and reflash.
Also sanity-check `rawX` maxes near 640. (Only matters if Y precision matters.)

## 7. Confidence calibration

At ~1 m frontal, read `conf`/`rawScore`. A clear frontal face should comfortably clear
`conf > 60` (`PS_CONF_GATE`, DFRobot's validity floor). If a clear face won't track, lower
`PS_CONF_GATE` incrementally; if empty-frame noise produces `faces=1`, raise it.

## 8. AutoMove resume (lost timeout)

With the eye tracking you, leave the frame. **Expect:** ~3 s (`FACE_LOST_MS`) after your last
detection the eye stops holding and begins idle wander; return → it re-locks.
**FAIL:** eye freezes forever → regression in the loop's no-face `else if`.

## 9. Edge tracking / flaky comms

(a) Move to the field corners and hold — eye should reach its travel limit smoothly (no freeze /
snap-back / drift). (b) Briefly interrupt the sensor TX wire — no crash; the eye holds, then
wanders after ~3 s, and re-locks when comms return.

## 10. Dual-eye (only if built with `#define DUAL_EYE`)

Wire the second display (WIRING.md dual-eye table: CS9/DC8/RST6, shared SCK13/MOSI11), uncomment
`#define DUAL_EYE`, reflash. **Expect:** both displays show an eye and track the same face
together. Per-eye refresh is ~half single-eye (shared bus) — expected, not a fault.

---

**At close:** state the real status (DEPLOYED vs VERIFIED) and which steps are confirmed vs open.
Set `CG_CALIB_RAW = 0` for quiet serial once done bench-logging.
