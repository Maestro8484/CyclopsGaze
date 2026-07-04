# CyclopsGaze — Gesture Integration Proposal
## Replacing IRIS Base-Mount Teensy with SEN0626 Gesture Detection

Date: 2026-06-24  
Status: PROPOSAL — not implemented

---

## 1. What the SEN0626 Already Has

The SEN0626 detects 5 gestures with 0-100 confidence scores, available via two
additional Modbus registers immediately after the face registers that are already
being read:

| Register | Name               | Current | Proposed |
|----------|--------------------|---------|----------|
| 0x04     | face_number        | ✓ read  | ✓ read   |
| 0x05     | face_x             | ✓ read  | ✓ read   |
| 0x06     | face_y             | ✓ read  | ✓ read   |
| 0x07     | face_score         | ✓ read  | ✓ read   |
| **0x08** | **gesture_type**   | —       | +2 regs  |
| **0x09** | **gesture_score**  | —       | +2 regs  |

Gesture types:
```
1 = LIKE  (👍)
2 = OK    (👌)
3 = STOP  (🤚)
4 = YES   (✌️)
5 = SIX   (🤙)
0 = none
```

---

## 2. Core Efficiency Insight

All 6 registers (0x04-0x09) are contiguous. The current code reads 4 registers
in one Modbus FC04 transaction. Reading 6 registers costs 4 more payload bytes:

```
Current:  [0x72][0x04][0x00][0x04][0x00][0x04][CRC][CRC]  → 13-byte response
Proposed: [0x72][0x04][0x00][0x04][0x00][0x06][CRC][CRC]  → 17-byte response
```

At 9600 baud: 17 bytes ≈ 17.7ms vs 13.5ms. **Gesture data costs ~4ms extra.**
No second Modbus round-trip, no second Modbus CRC, no second 20ms guard interval.
This is the only correct approach — a separate gesture read transaction would add
42ms per cycle.

---

## 3. Required Code Changes (CyclopsGaze)

### 3.1 SEN0626Sensor.h — add gesture output

Add to the existing interface (no breaking changes to face tracking):

```cpp
// Add to SEN0626Sensor.h public section:
uint16_t gestureType()  const { return lastGestureType; }
uint16_t gestureScore() const { return lastGestureScore; }

// Add to private section:
uint16_t lastGestureType{0};
uint16_t lastGestureScore{0};
```

### 3.2 SEN0626Sensor.cpp — extend readFaceData() to 6 registers

Change one constant and add two assignments:

```cpp
// OLD:
uint16_t SEN0626Sensor::readFaceData(uint16_t *faceX, uint16_t *faceY, uint16_t *score) {
  uint8_t buf[8];
  if (!modbusReadInputRegs(serial, DEVICE_ADDR, 0x04, 4, buf)) return 0;
  uint16_t count = (buf[0] << 8) | buf[1];
  *faceX  = (buf[2] << 8) | buf[3];
  *faceY  = (buf[4] << 8) | buf[5];
  *score  = (buf[6] << 8) | buf[7];
  return count;
}

// NEW:
uint16_t SEN0626Sensor::readFaceData(uint16_t *faceX, uint16_t *faceY, uint16_t *score) {
  uint8_t buf[12];
  if (!modbusReadInputRegs(serial, DEVICE_ADDR, 0x04, 6, buf)) return 0;  // 4 → 6
  uint16_t count     = (buf[0]  << 8) | buf[1];
  *faceX             = (buf[2]  << 8) | buf[3];
  *faceY             = (buf[4]  << 8) | buf[5];
  *score             = (buf[6]  << 8) | buf[7];
  lastGestureType    = (buf[8]  << 8) | buf[9];   // new
  lastGestureScore   = (buf[10] << 8) | buf[11];  // new
  return count;
}
```

Buffer size: 8 → 12 bytes. `modbusReadInputRegs` already uses a 32-byte stack
buffer (`uint8_t resp[32]`), so no resize needed there.

### 3.3 New file: src/GestureDispatcher.h

Self-contained, no dependencies beyond Arduino.h. Does NOT belong in sensors/
because it is policy (what to do with gestures), not protocol (how to read them).

```cpp
#pragma once
#include <Arduino.h>

class GestureDispatcher {
public:
    enum class Gesture : uint8_t {
        None = 0, Like = 1, OK = 2, Stop = 3, Yes = 4, Six = 5
    };

    static constexpr uint16_t SCORE_THRESHOLD = 60;   // 0-100; matches SEN0626 factory default
    static constexpr uint32_t COOLDOWN_MS     = 1500; // min ms between gesture fires

    // Call once per sensor.read() cycle with raw values from sensor.
    // Returns true exactly once per gesture event (leading edge, score-gated,
    // cooldown-enforced). Read gesture() immediately after true return.
    bool update(uint16_t rawType, uint16_t rawScore) {
        const uint32_t now = millis();

        if (rawType == 0 || rawScore < SCORE_THRESHOLD) {
            _armed = true;   // type cleared → re-arm for next gesture
            return false;
        }

        if (!_armed) return false;           // still holding previous gesture
        if ((now - _lastFireMs) < COOLDOWN_MS) return false;  // cooldown active

        _armed       = false;
        _lastFireMs  = now;
        _last        = (Gesture)rawType;
        return true;
    }

    Gesture gesture() const { return _last; }

    static const char* name(Gesture g) {
        switch (g) {
            case Gesture::Like: return "LIKE";
            case Gesture::OK:   return "OK";
            case Gesture::Stop: return "STOP";
            case Gesture::Yes:  return "YES";
            case Gesture::Six:  return "SIX";
            default:            return "NONE";
        }
    }

private:
    bool     _armed{true};
    uint32_t _lastFireMs{0};
    Gesture  _last{Gesture::None};
};
```

**Debounce logic explained:**

```
Sensor output (type field over time):
   0 0 0 3 3 3 3 3 3 0 0 0 0 0 0 1 1 1 1 0 0 3 3 3 3
           ^fire       ^re-arm             ^fire  ^(blocked: cooldown)
```

- `_armed` starts true.
- On first nonzero type with score ≥ 60: fire, set `_armed = false`.
- While same gesture held: `_armed = false`, never re-fires.
- When type returns to 0: `_armed = true` again.
- Even if armed, the cooldown gate blocks accidental rapid double-fires from
  electrical glitches (the sensor can briefly report 0 mid-hold).

The cooldown and the arm/disarm are COMPLEMENTARY, not redundant:
- Armed state prevents re-fire on hold (type stays nonzero continuously).
- Cooldown prevents re-fire on a momentary glitch that drops to 0 and back.

### 3.4 main.cpp additions (~12 lines)

```cpp
// At top of file, after other includes:
#include "GestureDispatcher.h"

// In global scope, after sensor declaration:
GestureDispatcher gestures;

// In loop(), after sensor.read() block:
if (sensor.gestureType() > 0) {
    if (gestures.update(sensor.gestureType(), sensor.gestureScore())) {
        auto g = gestures.gesture();
        Serial.printf("GESTURE:%s\n", GestureDispatcher::name(g));
        eyes->blink();  // non-blocking attention signal
    }
}
```

Total new code: 4 includes/declarations + 8 loop lines = **12 lines in main.cpp**.

---

## 4. Complete Gesture-to-Command Mapping (Proposed)

Assign based on natural gesture semantics and ergonomic reach from a seated position.
These map directly to what the base-mount Teensy currently sends to the RPi4.

| Gesture | Type | Serial command    | IRIS action          | Rationale                          |
|---------|------|-------------------|----------------------|------------------------------------|
| LIKE 👍  | 1    | `GESTURE:LIKE`    | Volume up            | Thumbs up = increase               |
| OK 👌    | 2    | `GESTURE:OK`      | Play / confirm       | OK = go / affirmative              |
| STOP 🤚  | 3    | `GESTURE:STOP`    | Stop / pause         | Palm-out = halt                    |
| YES ✌️   | 4    | `GESTURE:YES`     | Volume down          | Victory/down sign                  |
| SIX 🤙   | 5    | `GESTURE:SIX`     | Skip / next track    | Call-me = "next"                   |

The RPi4 assistant.py maps these string commands to system actions. The Teensy
emits the strings; the RPi4 owns the semantics. Remapping requires no firmware change.

---

## 5. IRIS Integration Architecture

### Current state (before integration):
```
[IRIS Teensy 4.1] ←── USB ──→ [RPi4 assistant.py]
  - PersonSensor (SEN-21231 I2C)         ↑ FACE:1/FACE:0
  - Eyes, Mouth, Sleep                   ↑ [DBG]/*
  - Sends: FACE:1, FACE:0                ↓ EMOTION:*, MOUTH:n, MOUTHGEST

[Base-mount Teensy 4.0] ─── USB ──→ [RPi4]
  - Gesture sensor (unknown)
  - Sends: volume/stop/etc commands
```

### After integration:
```
[IRIS Teensy 4.1] ←── USB ──→ [RPi4 assistant.py]
  (unchanged)                      ↑ FACE:1/FACE:0
                                   ↑ [DBG]/*
                                   ↓ EMOTION:*, MOUTH:n, MOUTHGEST

[CyclopsGaze Teensy 4.0] ─── USB ──→ [RPi4]
  - SEN0626 (UART1, Modbus RTU)         ↑ GESTURE:LIKE / STOP / etc.
  - Face tracking (eye display)         ↑ FACE:1, FACE:0 (optional)
  - Gesture detection (zero extra cost) ↓ (no inbound needed, unless eye cmds wanted)
```

The base-mount Teensy 4.0 is physically removed. CyclopsGaze T4.0 takes its USB slot.
RPi4 assistant.py adds a `GESTURE:*` handler to its existing serial parser.

### RPi4 handler addition (Python sketch, ~8 lines):

```python
# In assistant.py serial handler, alongside FACE:1 parsing:
elif line.startswith("GESTURE:"):
    g = line.split(":")[1].strip()
    if   g == "LIKE":  audio.volume_up()
    elif g == "OK":    audio.play()
    elif g == "STOP":  audio.stop()
    elif g == "YES":   audio.volume_down()
    elif g == "SIX":   audio.next_track()
    iris_teensy.write(b"MOUTHGEST\n")   # show SILLY acknowledgment on mouth TFT
```

The `MOUTHGEST` write to IRIS Teensy preserves the existing visual acknowledgment
behavior without any changes to the IRIS Teensy firmware.

---

## 6. Serial Port Management on RPi4

The RPi4 will now have two relevant USB serial ports:
- `/dev/ttyIRIS_EYES` (Teensy 4.1) — existing, unchanged
- `/dev/ttyIRIS_GAZE` (CyclopsGaze T4.0) — new

Assign stable symlinks via udev rule (same pattern as existing IRIS_EYES rule):
```
SUBSYSTEM=="tty", ATTRS{idVendor}=="16c0", ATTRS{idProduct}=="0483",
  ATTRS{serial}=="<CyclopsGaze T4.0 serial>", SYMLINK+="ttyIRIS_GAZE"
```

Get serial number: `udevadm info /dev/ttyACMx | grep SERIAL`

---

## 7. Implementation Order for a Future Session

### Phase A — CyclopsGaze testbed (validate everything before touching IRIS):
1. Apply fixes from 07_PEER_REVIEW_HANDOFF.md (P0 bugs first)
2. Extend readFaceData() to 6 registers (Section 3.2)
3. Add gestureType()/gestureScore() accessors to SEN0626Sensor.h (Section 3.1)
4. Create GestureDispatcher.h (Section 3.3)
5. Add 12 lines to main.cpp (Section 3.4)
6. Flash, bench test:
   - Show each of the 5 gestures 5× each; verify exactly 1 fire per gesture hold
   - Hold a gesture for 5s; verify it does NOT repeat
   - Show 2 different gestures rapidly; verify cooldown blocks the second
   - Verify eye blink fires on each gesture

### Phase B — IRIS integration (separate session, after Phase A verified):
1. Copy confirmed SEN0626Sensor.h/.cpp to IRIS src/sensors/
2. Wire CyclopsGaze T4.0 to IRIS chassis (replace base-mount T4.0)
3. Add udev rule to RPi4 for stable /dev/ttyIRIS_GAZE
4. Extend RPi4 assistant.py serial parser with GESTURE:* handler (~8 lines)
5. Test all 5 gestures end-to-end: gesture → serial → RPi4 → action + MOUTHGEST
6. Decommission base-mount Teensy 4.0

---

## 8. Risks and Mitigations

| Risk | Likelihood | Mitigation |
|------|-----------|------------|
| SEN0626 gesture zone conflicts with face zone | Medium | SEN0626 reports gesture and face independently; typically gesture wins when hand obscures face. Verify empirically. |
| Gesture fires during face tracking (accidental) | Low | Score threshold 60 + cooldown 1500ms is conservative. Tune on bench. |
| SEN0626 gesture range shorter than base-mount sensor | Unknown | DFRobot specs: gesture detection up to 1m. Verify CyclopsGaze position covers operator reach zone. |
| Two USB serials confusing RPi4 parser | Low | Stable udev symlinks give deterministic port names. Existing IRIS_EYES rule is the template. |
| MOUTHGEST + audio action race on RPi4 | Very Low | Python is synchronous serial read; MOUTHGEST write takes <1ms after gesture handler runs. |

---

## 9. What NOT to Change

- `SAMPLE_TIME_MS`: no change. Gesture data arrives every 150ms cycle — plenty
  fast enough. Human gesture holds are ≥500ms.
- Modbus device address or baud rate: no change.
- IRIS Teensy 4.1 firmware: no change required.
- The 5-gesture mapping above: not hardcoded anywhere. Defined in RPi4 Python,
  changeable without a firmware flash.
