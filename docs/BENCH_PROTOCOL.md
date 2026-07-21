# First-Flash Bench Protocol

A top-to-bottom checklist to bring up CyclopsGaze on real hardware, written for someone new
to PlatformIO. Each step: action → expected serial → pass/fail. Serial monitor at **115200**
baud.

Re-running this is the #1 priority on the next flash. The CG-S12 code sync (raw-score gate +
per-axis gain/bias) compiles clean but hasn't been re-observed on the bench since the change.
Steps 5–7 below re-confirm it. Firmware in repo: **CG-S13** (bump `FIRMWARE_VERSION` in
`src/config.h` before flashing if you change code).

CG-S13 note: tune live, don't reflash. Every gate/gain/bias/timeout below is now settable
over the same USB serial link you're watching, via the `PS_CFG:` protocol ported from IRIS.
See [§ Live tuning](#live-tuning-ps_cfg) before you start. Type a value, watch the eye change.
Only write the keeper back into `config.h` at the end.

## 0. Hardware first-checks (before anything else)

- SEN0626 DIP switch on UART (not I²C). This firmware has no I²C path. Wrong mode reads as
  `NOT FOUND` with otherwise-correct wiring. This is the single most likely cause of a
  first-flash failure. Check it before re-wiring.
- Sensor's own VCC pin ≈ 3.2–3.3V under load (measure at the sensor, not the Teensy pin).
- Faces will sit ≥ ~20in (0.5m) from the lens (the sensor's documented range floor).

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
**Expect:** `[CG] CyclopsGaze CG-S13`
**PASS:** version line matches `config.h` `FIRMWARE_VERSION`. (You can also ask any time by
typing `VERSION` into the monitor.)

## Live tuning (`PS_CFG:`)

Type these into the serial monitor (line-terminated). They take effect immediately, no
reflash. Ported verbatim from IRIS (S141 + S212c), so the same commands drive an IRIS board.

| Command | Effect | Default |
|---|---|---|
| `PS_CFG:CONF=n` | confidence gate, raw DFRobot score 0–100 | `60` |
| `PS_CFG:X_GAIN=f` | X gain: sign = direction, magnitude = range | `1.7` |
| `PS_CFG:Y_GAIN=f` | Y gain: sign = up/down direction, magnitude = range | `1.7` |
| `PS_CFG:X_BIAS=f` | X centering offset | `0.0` |
| `PS_CFG:Y_BIAS=f` | Y centering offset (compensates sensor mounted below the eye) | `1.26` |
| `PS_CFG:LOST_MS=n` | ms with no face before idle wander resumes | `3000` |
| `PS_CFG:FACING=0/1` | require the `is_facing` bit (inert, SEN0626 has no facing register) | `0` |
| `PS_CFG:LED=0/1` | accepted for API parity; no-op (no LED register exists) | `0` |
| `PS_CFG?` | print all live values on one line | n/a |

Each accepted key echoes `[DBG] PS_CFG KEY=value`. An unimplemented key answers
`[DBG] PS_CFG UNKNOWN key …`. If you see that, you typo'd, and nothing changed.

Values are RAM-only and reset with the board. There's no `ps_config.json` here the way IRIS
has on its Pi4. When a value proves out, write it into `src/config.h` (the `*_DEFAULT`
constants) and reflash, or it's lost on the next power cycle.

## 3. SEN0626 detect (baud + PID)

**Expect within ~4s of boot:** `[CG] SEN0626 found at 9600 (attempt 1)` (or `115200`). Record
which baud.
**FAIL:** `[CG] SEN0626 NOT FOUND at 115200 or 9600 -- check wiring` → check the DIP switch
first (step 0), then TX→pin0 / RX→pin1 cross, 3.3V, GND.

## 4. Face detect (serial format)

Sit ~1m in front of the sensor, facing it. With `CG_CALIB_RAW` on (default), expect:
```
[CG] faces=1 rawScore=NN conf=NN gate=PASS | rawX=NNN rawY=NNN
[CG]   -> raw=+0.NN,+0.NN  target=+0.NN,+0.NN  (gain 1.70/1.70 bias 0.00/1.26)
```
`rawScore`/`conf` = DFRobot score 0–100 (CG-S12); `raw` = sensor-space target in [−1,+1]
before shaping; `target` = what the eye was actually driven with; the trailing gain/bias are
the live values, so a `PS_CFG:` change is visible in the very next line. `rawX/rawY` = sensor
center 0–640 / 0–480(?).
**PASS:** `faces=1` with plausible values when a face is present; logging stops when you leave.

## 5. Direction verification (re-confirm after CG-S12)

Move slowly and check the eye and the serial signs:

| Stimulus (your position vs sensor) | Expected eye |
|---|---|
| Face to camera's RIGHT | eye looks RIGHT |
| Face to camera's LEFT | eye looks LEFT |
| Face toward TOP of frame | eye looks UP |
| Face toward BOTTOM of frame | eye looks DOWN |
| Face centered | eye straight ahead (x≈0, y≈0) |

**PASS:** eye follows you in all four directions. Fix any failure live, no reflash:
- LEFT/RIGHT mirrored: flip the sign, `PS_CFG:X_GAIN=-1.7`.
- UP/DOWN inverted: `PS_CFG:Y_GAIN=-1.7`.
- Travel too small: raise the magnitude, `PS_CFG:X_GAIN=2.5`. (Not a bug on its own: at 20in
  from an 85° FOV a ±6in head move only crosses ~40% of frame.)
- Eye biased off-center at neutral: `PS_CFG:Y_BIAS=…` / `PS_CFG:X_BIAS=…` (`Y_BIAS`
  compensates for the camera mounting below the eye).

Write whatever proves out back into `src/config.h` (`GAZE_*_DEFAULT`) before you power down.

## 6. `NATIVE_H` calibration (480 vs 640)

Keep a face in frame; move to the very top then very bottom, watching `rawY`. Note the
maximum `rawY`. ≈480 → leave `NATIVE_H=480`; ≈640 → set `NATIVE_H=640` in `SEN0626Sensor.h`
and reflash. Also sanity-check `rawX` maxes near 640. (Only matters if Y precision matters.)

## 7. Confidence calibration

At ~1m frontal, read `conf`/`rawScore`. A clear frontal face should comfortably clear
`conf > 60` (the gate default, DFRobot's own validity floor). If a clear face won't track,
lower it live (`PS_CFG:CONF=55`, …); if empty-frame noise produces `gate=PASS`, raise it.
Record the value that stabilises tracking and put it in `PS_CONF_GATE_DEFAULT`.

For reference, live IRIS runs `CONF=25`. Don't copy that. It's a leftover from the Person
Sensor's 0–255 scale (~10%) that predates the SEN0626 swap, not a tuned value. See
[ENGINEERING_LOG.md](ENGINEERING_LOG.md) CG-S13.

## 8. AutoMove resume (lost timeout)

With the eye tracking you, leave the frame. **Expect:** ~3s (the `LOST_MS` default) after
your last detection the eye stops holding and begins idle wander; return → it re-locks.
Shorten it while bench-testing with `PS_CFG:LOST_MS=1500` so you're not waiting on every pass.
**FAIL:** eye freezes forever → regression in the loop's no-face `else if`.

## 9. Edge tracking / flaky comms

(a) Move to the field corners and hold. Eye should reach its travel limit smoothly (no
freeze / snap-back / drift). (b) Briefly interrupt the sensor TX wire. No crash; the eye
holds, then wanders after ~3s, and re-locks when comms return.

## 10. Dual-eye (only if built with `#define DUAL_EYE`)

Wire the second display (WIRING.md dual-eye table: CS9/DC8/RST6, shared SCK13/MOSI11),
uncomment `#define DUAL_EYE`, reflash. **Expect:** both displays show an eye and track the
same face together. Per-eye refresh is ~half single-eye (shared bus), expected, not a fault.

---

**At close:** state the real status (DEPLOYED vs VERIFIED) and which steps are confirmed vs
open. Set `CG_CALIB_RAW = 0` for quiet serial once done bench-logging.
