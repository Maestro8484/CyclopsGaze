# CyclopsGaze — AI Peer Review Handoff
## Critical Review: Gaze Tracking Accuracy

Generated: 2026-06-24 (CG-S1)
Reviewer target: verify face target coordinates, gaze direction chain, and all sub-90%-confidence implementations.

---

## 1. Architecture in One Paragraph

SEN0626 (Modbus RTU over UART1 at 9600 baud, device addr 0x72) returns face CENTER coordinates (X: 0-640, Y: 0-480 assumed). The driver shim in `SEN0626Sensor.cpp` remaps center to 0-255 and constructs a **fake bounding box** (±32 around center) to match the `person_sensor_face_t` interface. `main.cpp` extracts the center from the fake box, runs the tracking formula, and calls `EyeController::setTargetPosition(targetX, targetY)`. `EyeController::renderFrame()` applies a software X-mirror to eye 0 before rendering. The tracking formula includes a negation to compensate for this mirror.

---

## 2. File Map

```
src/sensors/SEN0626Sensor.h    — shim interface + person_sensor_face_t struct
src/sensors/SEN0626Sensor.cpp  — Modbus RTU polling driver
src/main.cpp                   — setup/loop, tracking formula
src/config.h                   — single-eye config, initEyes()
src/eyes/EyeController.h       — renderFrame(), setTargetPosition() [copied from IRIS verbatim]
```

---

## 3. Tracking Direction Chain — Full Trace

This is the most non-obvious part of the system. Verify every step.

### Variables (nordicBlue, EyeController<1>)
- `mapRadius` = 240 (from PolarParams default in eyes.h; nordicBlue uses disp_240_125 but mapRadius is set by PolarParams.mapRadius default)
- `middle` = 240.0f
- `r` = (240*2 - 240*π/2) * 0.75 = (480 - 376.99) * 0.75 ≈ **77.3**
- `screenWidth` = 240

### Case: Face on RIGHT side of camera (faceX=400/640)

**Step 1 — Sensor read (SEN0626Sensor.cpp:118)**
```
cx = (uint8_t)(400 * 255 / 640) = (uint8_t)(159.375) = 159
```

**Step 2 — Fake box construction (SEN0626Sensor.cpp:122-125)**
```
box_left   = 159 - 32 = 127
box_right  = 159 + 32 = 191
```
(No clamping needed here; face is interior.)

**Step 3 — Center extraction (main.cpp:34)**
```
cx = 127 + (191 - 127) / 2.0 = 127 + 32 = 159   ✓ matches original center
```

**Step 4 — Tracking formula (main.cpp:36)**
```
targetX = -((159 / 127.5) - 1.0) = -(1.247 - 1.0) = -0.247
```
Negative when face is RIGHT.

**Step 5 — setTargetPosition (EyeController.h:530)**
```
state.eyeNewX = middle - xTarget * r = 240 - (-0.247 * 77.3) = 240 + 19.1 = 259.1
```
eyeNewX is **right of center** (> 240).

**Step 6 — renderFrame mirror for eyeIndex=0 (EyeController.h:590-592)**
```
eye.x = mapRadius * 2 - eyeNewX = 480 - 259.1 = 220.9
```
eye.x is **left of center** (< 240).

**Step 7 — renderEye xPositionOverMap (EyeController.h:283)**
```
xPositionOverMap = eye.x - screenWidth/2 = 220.9 - 120 = 100.9
```

**Step 8 — Screen-to-map sampling (renderEye inner loop)**
For screen pixel at screenX=120 (display center):
```
xx = xPositionOverMap + screenX = 100.9 + 120 = 220.9
```
xx=220.9 is **left of mapCenter (240)**.

**Step 9 — Polar map direction interpretation**
The polar map is a 2D projection of the eye sphere. A viewport shifted LEFT (xx < mapRadius) samples the globe from the LEFT quadrant, which projects the iris CENTER to the **RIGHT** of the display.

**CONCLUSION: Face RIGHT → eye looks RIGHT. Direction is CORRECT.**

### Case: Face at LEFT edge (faceX=40/640)

**Step 1:** cx = (uint8_t)(40 * 255 / 640) = **15**

**Step 2 — EDGE CLAMPING BUG:**
```
box_left  = max(0, 15-32) = 0      ← clamped, loses 17 pixels
box_right = min(255, 15+32) = 47
```

**Step 3 — Center extraction GIVES WRONG ANSWER:**
```
cx = 0 + (47 - 0) / 2.0 = 23.5    ← WRONG, actual center is 15
```
**Error: +8.5 pixels rightward shift.** The computed center is to the right of the actual face center. The eye will track toward the face but not far enough left — it will consistently under-track when the face is within 32 pixels (in 0-255 space) of either edge.

Frequency of this error: any face where cx255 < 32 or cx255 > 223, which corresponds to:
- X < 80/640 = 12.5% from left edge
- X > 560/640 = 87.5% from right edge
Same math applies to Y with cy255 boundaries at 32/255 and 223/255.

---

## 4. Issues by Confidence Rating

### BUG (HIGH CONFIDENCE — will produce wrong behavior)

**Issue 4.1 — Edge-clamped box shifts computed center**
- File: `SEN0626Sensor.cpp:122-125` (box construction), `main.cpp:34` (center extraction)
- Root cause: fake box clamped to [0,255] at edges, but main.cpp re-derives center from box edges. For interior faces, cx_computed = cx_actual ✓. For edge faces, cx_computed ≠ cx_actual.
- Severity: the eye under-tracks when a face is near the camera frame edge (innermost ~12% of field).
- Fix A (minimal): Skip the fake box entirely. Store center directly:
  ```cpp
  // In SEN0626Sensor.cpp read():
  face.box_left   = cx;   // actual center
  face.box_right  = cx;
  face.box_top    = cy;
  face.box_bottom = cy;
  ```
  And in main.cpp:
  ```cpp
  float cx = f.box_left;   // center already stored
  float cy = f.box_top;    // center already stored
  ```
  Note: area = 0 for "find largest" loop, but this doesn't matter since SEN0626 shim always returns 0 or 1 faces.

- Fix B (preserves interface semantics): clamp before subtracting, not after:
  ```cpp
  uint8_t half = BOX_HALF;
  face.box_left   = (cx >= half) ? cx - half : 0;
  face.box_right  = (cx + half <= 255) ? cx + half : 255;
  // Then in main.cpp, use stored cx directly, not (box_left + box_right)/2
  ```
  But this requires passing cx through a separate field (id_confidence or id, which are zeroed). Ugly.

- Fix C (recommended): Store cx in BOTH box_left and box_right, cy in both box_top and box_bottom. The /2 in main.cpp then gives exactly cx. No edge error. The 0-area "find largest" sort is irrelevant because n is always ≤ 1.

**Issue 4.2 — `/3.0f` Y formula expects real face bounding box, not fake centered box**
- File: `main.cpp:35`
- Code: `float cy = f.box_top + (f.box_bottom - f.box_top) / 3.0f;`
- The `/3.0f` was designed for a real bounding box where the forehead is at the top and the eyes are at roughly 1/3 of the face height. Using 1/3 rather than 1/2 intentionally targets the eye region instead of the midface.
- For a fake symmetric box (height=64, top=cy-32): `cy_computed = (cy-32) + 64/3 = cy - 32 + 21.3 = cy - 10.7`.
  - The computed target is ~10.7 pixels **above** the actual face center.
  - This persistent upward bias means the eye consistently looks slightly above where the face center is.
- Severity: small but consistent systematic error (~10px in 0-255 space = ~4% of frame height).
- Fix: Change `/3.0f` to `/2.0f` since the box center IS the face center. OR change to use `f.box_top` directly if you implement Fix C above.

---

### UNVERIFIED (< 99% confidence — needs bench confirmation)

**Issue 4.3 — Native Y resolution assumed 480, not confirmed**
- File: `SEN0626Sensor.h:51`
- Code: `static constexpr uint16_t NATIVE_H = 480;`
- DFRobot wiki says "Facial Position Coordinate Range: 0-640" (X only). Y range not documented.
- Possibilities: 640x480 (standard VGA), 640x640 (square), or something else.
- Impact: if Y is actually 0-640, the current remap `cy = faceY * 255 / 480` overcounts by 640/480 = 1.33x — Y values >480 would be clamped, and the eye will over-respond vertically.
- If Y is actually 0-640, change `NATIVE_H = 640`.
- **Bench test:** print raw faceY values at top/bottom of frame. If max observed ≈ 480, correct. If ≈ 640, fix.

**Issue 4.4 — Modbus register 0x04 may not be face COUNT when no face present**
- File: `SEN0626Sensor.cpp:93`
- The DFRobot header defines `REG_GFD_FACE_NUMBER = 0x04`. The code treats any nonzero value as face detected. But Modbus registers may return 0xFFFF or other sentinel on comms error (modbusReadInputRegs returns false for CRC/timeout failures). The `readFaceData()` returns 0 on Modbus failure, which maps correctly to "no face". ✓
- But: if the register returns a value like 0x0000 when no face is present and a nonzero value (1-3) when faces are found, the logic is correct. No confirmed risk here, but worth logging raw count values on bench.

**Issue 4.5 — 300ms Modbus response timeout may be insufficient or excessive**
- File: `SEN0626Sensor.cpp:38`
- At 9600 baud, 13-byte response = ~13.5ms. 300ms is 22x generous — fine for normal operation.
- Risk: if sensor sends a partial response then stalls, the wait blocks for 300ms before failing. This freezes eye animation for 300ms per failed read. At SAMPLE_TIME_MS=150, a stuck sensor could cause periodic 300ms freezes.
- No fix needed unless bench shows sensor sends partial responses. If so, reduce timeout to ~50ms.

**Issue 4.6 — autoMove sticks OFF if sensor reads continuously fail**
- File: `main.cpp:21-45`
- `sensor.timeSinceFaceDetectedMs() > 3000` check is inside `if (sensor.read())`.
- If `sensor.read()` returns false every call (Modbus not responding), the `> 3000` check never runs.
- Result: if setAutoMove(false) was already called, the eye freezes on the last tracked position indefinitely.
- Fix: move the `timeSinceFaceDetectedMs()` check outside the `if (sensor.read())` block:
  ```cpp
  sensor.read();  // attempt; may return false
  if (sensor.numFacesFound() > 0) {
      ...
      eyes->setAutoMove(false);
  } else if (sensor.timeSinceFaceDetectedMs() > 3000) {
      eyes->setAutoMove(true);
  }
  ```

**Issue 4.7 — SEN0626 FC04 register 0x00 returns PID=0x0272, but this is not documented in DFRobot wiki**
- File: `SEN0626Sensor.cpp:66`
- The PID value `0x0272` and register `0x00` were taken from the DFRobot C++ library source. The wiki page does not document this. If a firmware update changes PID or register layout, begin() will fail to detect the sensor. Low risk for new hardware but documented for posterity.

**Issue 4.8 — mapRadius for nordicBlue — assumed 240**
- File: `EyeController.h`, `config.h`
- The `r` calculation in setTargetPosition uses `eye.definition->polar.mapRadius`:
  ```cpp
  auto r = (middle * 2.0f - screenWidth * M_PI_2) * 0.75f;
  ```
- The tracking trace above assumed mapRadius=240 (PolarParams default). If nordicBlue.h's EyeDefinition overrides polar.mapRadius to a different value, `r` changes and the eye will have a different range of motion.
- To verify: read nordicBlue.h's EyeDefinition struct definition and check if `polar.mapRadius` is overridden from 240.
- Impact: if mapRadius differs, the gaze range of motion changes but direction remains correct.

---

## 5. Modbus RTU Implementation Spot-Check

**CRC-16 (SEN0626Sensor.cpp:5-15)**
Algorithm: standard Modbus CRC-16, polynomial 0xA001 (reflected 0x8005), init 0xFFFF.
This matches the Modbus standard precisely. ✓

**Request frame (lines 21-31)**
FC04 Read Input Registers:
```
[addr][0x04][reg_hi][reg_lo][count_hi][count_lo][CRC_lo][CRC_hi]
```
Note: CRC bytes are appended little-endian (lo byte first), which is correct for Modbus RTU. ✓

**Response validation (lines 46-50)**
Checks CRC, then addr/FC/byte_count. Correct order — CRC checked before trusting payload length. ✓

**Flush before write (line 32-34)**
```cpp
while (ser.available()) ser.read();
ser.write(req, 8);
ser.flush();
```
Discards stale RX bytes before sending. `flush()` on Teensy HardwareSerial blocks until TX drain. Correct. ✓

**Multi-register read (line 92)**
```cpp
modbusReadInputRegs(serial, DEVICE_ADDR, 0x04, 4, buf)
```
Reads input registers 0x04-0x07 in one transaction. These are contiguous in DFRobot's register map. ✓

**Possible off-by-one: register address 0x04 vs 0-indexed**
In Modbus, input register addresses in FC04 requests start at 0x0000 (register #1 in 1-indexed Modbus convention, but DFRobot's library uses 0-indexed addresses directly matching the register defines). DFRobot_RTU::readInputRegister passes the register address directly as given, no offset added (for UART). So `0x04` in our request = `REG_GFD_FACE_NUMBER = 0x04`. ✓

---

## 6. Questions the Reviewer Must Answer

1. **nordicBlue mapRadius**: Is `polar.mapRadius` overridden in the nordicBlue EyeDefinition? If so, what value? This affects the range of eye motion.

2. **Edge-clamped center bug**: Confirm Issue 4.1 is a real bug by tracing: if `cx=10`, does `main.cpp` receive `box_left=0, box_right=42`, and does it then compute center as 21? Should the shim store `cx` directly in both box_left and box_right?

3. **autoMove freeze bug**: Confirm Issue 4.6 — does the `timeSinceFaceDetectedMs()` check execute when `sensor.read()` returns false? If not, the fix in 4.6 is required.

4. **Y tracking sign**: The Y tracking formula in main.cpp:37 is:
   ```cpp
   float targetY = ((cy / 127.5f) - 1.0f);
   ```
   (positive sign, no negation). Is this correct? The EyeController's setTargetPosition maps yTarget to `state.eyeNewY = middle - yTarget * r`. For face at top (cy=0): targetY = -1.0, eyeNewY = middle + r (below center). renderFrame does NOT mirror Y for eye 0 (only X is mirrored). So: face at top → targetY=-1 → eyeNewY > middle → eye.y > middle → yPositionOverMap > 0 → eye looks DOWN.
   **Face at top → eye looks DOWN. This is likely INVERTED.**
   Expected: face at top → eye looks UP.
   Proposed fix: negate targetY to match targetX:
   ```cpp
   float targetY = -((cy / 127.5f) - 1.0f);
   ```

5. **box_right clamping boundary**: `(cx + BOX_HALF < 255)` should this be `<= 255`? If cx=224, cx+32=256 which doesn't satisfy `< 255`, so box_right = 255. But cx=223, cx+32=255 which satisfies `< 255`, so box_right = 255. Both cases produce 255 which is correct. The condition as written has an edge case at cx=223: box_right=255 correctly, but the condition `223+32=255 < 255` is FALSE, so box_right = 255. Actually `255 < 255` is false, so it takes the `:255` branch. ✓ for cx=223. For cx=224: `256 < 255` is false → box_right=255 ✓. Condition is correct, though it could read more clearly as `<= 254` or the assignment as `min(cx+BOX_HALF, 255)`.

---

## 7. Suggested Fixes Priority Order

**P0 — Fix before first flash (will cause wrong behavior):**
1. **Y direction (Issue Q4 above)** — change `main.cpp:37` to negate targetY if the analysis in Q4 is confirmed. Easy one-liner.
2. **Edge center bug (Issue 4.1)** — store `cx`/`cy` directly in both box_left/box_right and box_top/box_bottom. Eliminates all edge tracking error.
3. **Y formula (Issue 4.2)** — change `/3.0f` to `/2.0f` in `main.cpp:35` since box center = face center.

**P1 — Fix before calling tracking "working":**
4. **autoMove freeze (Issue 4.6)** — restructure main.cpp loop so `timeSinceFaceDetectedMs()` check runs even when `sensor.read()` returns false.

**P2 — Verify on bench, then fix if needed:**
5. **NATIVE_H (Issue 4.3)** — log raw faceY at extremes, confirm 480 vs 640.

---

## 8. Unreviewed Code (out of scope for this review)

- `EyeController.h` — copied verbatim from IRIS (production-tested)
- `GC9A01A_Display.cpp/h` — copied verbatim from IRIS (production-tested)
- `eyes.h`, `nordicBlue.h`, `polarAngle_240`, `polarDist_240_125_69_0`, `disp_240_125` — copied verbatim from IRIS
- These files have accumulated bugs noted in IRIS git history (see `GC9A01A_Display::~GC9A01A_Display` comment about GC9A01A_t3n destructor) but are not CyclopsGaze-specific

---

## 9. Test Cases for Bench Verification

After flashing, verify in order:

| Test | Stimulus | Expected Serial | Expected Display |
|------|----------|-----------------|------------------|
| Boot | Power on | `[CG] CyclopsGaze CG-S1` then `[CG] SEN0626 found at 9600` | Eye appears, idle wander |
| Center | Face centered in frame | `x≈0.00 y≈0.00` | Eye looks straight |
| Move right | Face moves from center to right | x goes more negative | Eye tracks RIGHT |
| Move left | Face moves from center to left | x goes more positive | Eye tracks LEFT |
| Move up | Face moves up in frame | y decreases toward -1 | Eye tracks UP (if fix applied) |
| Move down | Face moves down in frame | y increases toward +1 | Eye tracks DOWN |
| Far edge | Face at extreme left edge | x approaches +1 | Eye tracks far left without freezing |
| Hide face | Remove face, wait >3s | (no face log) | Eye resumes wander |
| Flaky comms | Partially block sensor TX | No crash | Eye holds last position |

---

## 10. Known Acceptable Limitations

- SEN0626 reports only 1 face max (best detected); multi-face selection loop in main.cpp is dead code but harmless.
- SAMPLE_TIME_MS=150ms caps tracking update rate to ~6.7Hz. Eye animation continues at full rate between updates.
- No `is_facing` equivalent — eye tracks any detected face regardless of whether it's looking at camera.
- SEN0626 only returns face position of the SINGLE best-detected face when multiple are present; cannot select by size as the interface implies.
