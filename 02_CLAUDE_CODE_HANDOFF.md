# CyclopsGaze — Claude Code Session Handoff

Model: claude-sonnet-4-6
Effort: medium

---

## SESSION START — READ FIRST

1. Read `CYCLOPSGAZE_RULES.md` in repo root — mandatory, governs this session
2. `git status` — confirm on main, clean tree
3. Confirm IRIS repo at `C:\Users\SuperMaster\Documents\PlatformIO\IRIS-Robot-Face` is accessible (read-only)

---

## TASK

Build CyclopsGaze — a standalone PlatformIO project on SuperMaster targeting a spare
Teensy 4.0 with one GC9A01A 240x240 round display and a DFRobot SEN0626 AI camera
sensor over UART. The SEN0626 replaces the Useful Sensors Person Sensor (SEN-21231)
via a shim driver presenting the identical PersonSensor.h public interface. Goal: single
animated eye renders on the GC9A01A and tracks the largest detected face in the
SEN0626 field of view. No Pi4, no mouth TFT, no sleep renderer, no serial bridge —
pure standalone testbed.

---

## HARDWARE

| Component | Detail |
|---|---|
| MCU | Teensy 4.0 (spare, all pins free) |
| Display | GC9A01A 240x240 round LCD (one display only) |
| Sensor | DFRobot SEN0626 AI camera, UART |
| Host | SuperMaster (Windows, USB serial) |

Wiring is complete per 05_WIRING.md before this session starts.

---

## DISCOVERY PHASE (read in order, minimum required)

0. Read `CYCLOPSGAZE_RULES.md` — always first
1. Read IRIS `src/sensors/PersonSensor.h` — extract full public interface contract
2. Read IRIS `src/sensors/PersonSensor.cpp` — extract polling pattern
3. Read IRIS `src/config.h` — extract GC9A01A config struct and initEyes() pattern
4. Read IRIS `src/displays/Display.h` and `GC9A01A_Display.h` — copy targets
5. Read IRIS `src/eyes/eyes.h` — copy target
6. Fetch SEN0626 UART packet format from live DFRobot wiki:
   https://wiki.dfrobot.com/SKU_SEN0626
   Must confirm before writing SEN0626Sensor.cpp:
   - Packet header/footer bytes
   - Face count field position
   - Bounding box field positions (x1, y1, x2, y2 or equivalent)
   - Confidence field position and scale (0-100? 0-255?)
   - Native resolution (W x H) for coord remap to 0-255
   - Whether is_facing equivalent exists
   NEVER guess the packet format. Read the wiki.

SKIP DISCOVERY IF: user says SKIP or GO

---

## REPO STRUCTURE

```
CyclopsGaze/
  platformio.ini
  CYCLOPSGAZE_RULES.md
  README.md
  NOTES.md
  src/
    CyclopsGaze.ino        <- dummy .ino required by Teensyduino
    main.cpp
    config.h
    displays/
      Display.h            <- copy verbatim from IRIS
      GC9A01A_Display.h    <- copy verbatim from IRIS
      GC9A01A_Display.cpp  <- copy verbatim from IRIS
    eyes/
      EyeController.h      <- copy verbatim from IRIS
      eyes.h               <- copy verbatim from IRIS
      240x240/
        nordicBlue.h       <- copy verbatim from IRIS
    sensors/
      SEN0626Sensor.h      <- new shim interface
      SEN0626Sensor.cpp    <- new UART polling driver
```

---

## IMPLEMENTATION ORDER

### 1. platformio.ini
```ini
[env:cyclopsgaze]
platform = teensy
board = teensy40
framework = arduino
lib_deps =
    https://github.com/KurtE/GC9A01A_t3n
build_flags = -DUSE_GC9A01A
monitor_speed = 115200
```

### 2. CyclopsGaze.ino
```cpp
// Dummy .ino — code is in main.cpp
```

### 3. Copy verbatim from IRIS (no edits)
- `src/displays/Display.h`
- `src/displays/GC9A01A_Display.h`
- `src/displays/GC9A01A_Display.cpp`
- `src/eyes/EyeController.h`
- `src/eyes/eyes.h`
- `src/eyes/240x240/nordicBlue.h`

### 4. SEN0626Sensor.h
Present identical public interface to IRIS PersonSensor.h:
- Same `person_sensor_face_t` struct (copy exactly from PersonSensor.h)
- Same public methods:
    bool isPresent();
    bool begin();            <- new: baud auto-detect, call from setup()
    bool read();
    void enableID(bool);     <- stub, no-op
    void setMode(Mode mode); <- stub, no-op
    void enableLED(bool);    <- stub, no-op
    int numFacesFound() const;
    person_sensor_face_t faceDetails(int faceNumber);
    unsigned long timeSinceFaceDetectedMs();
- Internal: HardwareSerial &serial, packet buffer, elapsedMillis
- static constexpr long SAMPLE_TIME_MS = 70

### 5. SEN0626Sensor.cpp

begin() — baud auto-detect:
```cpp
bool SEN0626Sensor::begin() {
    // Try 115200 first
    serial.begin(115200);
    delay(500);
    if (serial.available()) {
        Serial.println("[CG] SEN0626 found at 115200");
        return true;
    }
    // Fallback to 9600
    serial.end();
    serial.begin(9600);
    delay(500);
    if (serial.available()) {
        Serial.println("[CG] SEN0626 found at 9600");
        return true;
    }
    Serial.println("[CG] SEN0626 NOT FOUND at 115200 or 9600 -- check wiring");
    return false;
}
```

isPresent() — returns true if begin() succeeded (store result in bool member).

read() — Option A polling:
- Gate on SAMPLE_TIME_MS
- Check serial.available(), read bytes into buffer
- Parse complete packet per confirmed packet format from DFRobot wiki
- Remap bounding box coords to 0-255 range
- Populate: num_faces, box_confidence, box_left, box_top, box_right, box_bottom
- is_facing: default true if SEN0626 has no equivalent field
- Reset lastDetectionTimeMs on successful face read

### 6. config.h (stripped CyclopsGaze version)
- Single GC9A01A: CS=10, DC=2, MOSI=11, SCK=13, RST=3
  rotation=0, mirror=true, useFrameBuffer=true, asyncUpdates=false
- EyeController<1, GC9A01A_Display> — one eye only
- nordicBlue only
- FIRMWARE_VERSION = "CG-S1"
- No mouth TFT, no sleep renderer, no Entropy
- initEyes() adapted for single display

### 7. main.cpp — minimal tracking loop

Setup:
```
Serial.begin(115200)
wait up to 2s for Serial
print "[CG] CyclopsGaze CG-S1"
sensor.begin()          <- auto-detects baud, prints result
randomSeed(analogRead(A0))
initEyes(true, true, true)
eyes->setTargetPupil(0.40f, 300)
```

Loop:
```
if sensor.read():
    find largest face by (box_right-box_left)*(box_bottom-box_top)
    if face found:
        eyes->setAutoMove(false)
        targetX = -((box_left + (box_right-box_left)/2.0f) / 127.5f - 1.0f)
        targetY =  ((box_top  + (box_bottom-box_top)/3.0f) / 127.5f - 1.0f)
        eyes->setTargetPosition(targetX, targetY)
        Serial.printf("[CG] faces=%d conf=%d x=%.2f y=%.2f\n", ...)
    else if timeSinceFaceDetectedMs() > 3000:
        eyes->setAutoMove(true)
eyes->renderFrame()
```

---

## COORD REMAP

SEN0626 native coords → person_sensor_face_t (0-255):
```
box_left   = (x1 / native_width)  * 255
box_right  = (x2 / native_width)  * 255
box_top    = (y1 / native_height) * 255
box_bottom = (y2 / native_height) * 255
```
native_width and native_height from datasheet — fill in after wiki read.

---

## VERIFY STEPS (in order after flash)

1. `pio run -e cyclopsgaze` — clean build, zero errors
2. Flash to T4.0 (PlatformIO auto-detect COM port — never hardcode COMx)
3. Open serial monitor at 115200
4. Confirm: `[CG] CyclopsGaze CG-S1`
5. Confirm: `[CG] SEN0626 found at 115200` or `found at 9600`
   If NOT FOUND: check TX/RX wiring (most common cause — swapped)
6. Hold face in front of SEN0626
7. Confirm serial: `[CG] faces=1 conf=NNN x=N.NN y=N.NN`
8. Confirm eye on display tracks face movement left/right/up/down
9. Move out of frame — eye resumes autoMove wander after ~3s

---

## SESSION CLOSE

Before ending:
- NOTES.md updated: confirmed baud rate, packet format, native resolution,
  coord remap values, any deviations from this handoff, issues found
- README.md wiring table verified accurate
- `pio run -e cyclopsgaze` clean build confirmed
- Commit:
  `git add -A && git commit -m "CG-S1: initial CyclopsGaze -- SEN0626 UART driver + single GC9A01A eye tracking"`
- State: Teensy firmware is REPO-ONLY — user flashes via PlatformIO
- State: carry-forward items if any

---

## DO NOT

- Touch IRIS-Robot-Face repo
- Hardcode COM port — PlatformIO auto-detects
- Guess SEN0626 packet format — fetch wiki first
- Use Entropy library — use randomSeed(analogRead(A0))
- Add mouth TFT, sleep renderer, Pi4 bridge, or any IRIS pipeline component
- Mark anything DEPLOYED unless physically flashed and serial output verified
- Defer NOTES.md to next session
