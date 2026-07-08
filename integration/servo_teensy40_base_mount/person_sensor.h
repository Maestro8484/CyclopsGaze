#pragma once
#include <Arduino.h>

// ─────────────────────────────────────────────────────────────────────────────
// SEN0626 drop-in replacement for the IRIS T4.0 servo Person Sensor driver.
//
// This file replaces IRIS `servo_teensy40/teensy40_base_mount/person_sensor.h`
// (the Useful Sensors SEN-21231 I2C driver) with a DFRobot SEN0626 UART/Modbus
// backend, while keeping the SAME public surface the servo .ino consumes:
//   setupPersonSensor(), pollPersonSensor() -> PersonResult,
//   setPersonSensorLed(bool), personSensorLedEnabled.
//
// The servo .ino (teensy40_base_mount.ino) is UNCHANGED: it still calls
//   PersonResult ps = pollPersonSensor();
//   if (!ps.ok) return;                 // short read / throttle -> hold pan
//   if (ps.faceVisible) updatePanFromFace(ps.faceCenterX);   // faceCenterX 0-255
//
// PORTING (see integration/README.md):
//   1. Copy CyclopsGaze src/sensors/SEN0626Sensor.{h,cpp} into this project.
//   2. Replace the project's person_sensor.{h,cpp} with these two files.
//   3. Wire SEN0626 to the T4.0's Serial1 (sensor TX->pin 0, sensor RX->pin 1),
//      not I2C. Remove/ignore the Wire.begin() line that fed the old PS bus
//      (the paj7620 gesture sensor keeps its own Wire usage; only the PS moves
//      off I2C).
//   4. The old I2C Person Sensor LED register does not exist on SEN0626, so the
//      LED calls become no-ops (kept as symbols so PSLED serial commands and the
//      .ino still compile/link). See 09_IRIS_INTEGRATION_PLAN.md.
// ─────────────────────────────────────────────────────────────────────────────

// Kept for source compatibility with any code that referenced these. The
// SEN0626 is a Modbus device (0x72), not I2C 0x62 -- these no longer describe a
// live bus address but are left defined so nothing that #include'd them breaks.
#define PERSON_SENSOR_I2C_ADDRESS 0x62
#define PERSON_SENSOR_FACE_MAX 4

// Pause between sensor polls (ms). SEN0626Sensor also self-throttles internally
// (SAMPLE_TIME_MS=150), so this only bounds the poll loop's own cadence.
#define PERSON_SENSOR_DELAY 50

// Compile-time DEFAULT for the (now absent) Person Sensor LED. Retained so the
// PSLED=0/1 serial command path still compiles; writing it is a no-op.
#define PERSON_SENSOR_LED_ENABLED 0

// Confidence gate on the SEN0626 0-255 box_confidence (= raw score*255/100).
// 152 == raw score >= 60, DFRobot's own documented validity floor for this
// sensor (wiki.dfrobot.com/sen0626/docs/23024), matching CyclopsGaze's verified
// PS_CONF_GATE. NOTE: the original servo driver gated `boxConfidence < 60` on
// the Useful Sensors 0-255 scale (~24%); this is stricter and vendor-derived.
// See 09_IRIS_INTEGRATION_PLAN.md §confidence-scale before changing.
#ifndef PS_SERVO_CONF_GATE
#define PS_SERVO_CONF_GATE 152
#endif

// Live LED state + setter, kept for API parity. On SEN0626 the setter is a
// no-op (no LED register) but the symbol stays so the .ino / serial PSLED
// command link unchanged.
extern bool personSensorLedEnabled;
void setPersonSensorLed(bool on);

// Result of one poll cycle -- byte-for-byte the original contract.
//   ok          = a fresh sample was taken this cycle. false => caller holds pan
//                 (matches the original short-read early-return). SEN0626's
//                 internal throttle means ok is false between samples, which is
//                 the desired "hold pan, no new data" behavior.
//   faceVisible = an accepted (gated) face is present this sample.
//   faceCenterX = face center X in 0-255 sensor space (= the old
//                 (boxLeft+boxRight)/2; SEN0626 stores a center-only box so this
//                 is the exact center).
struct PersonResult {
  bool    ok;
  bool    faceVisible;
  float   faceCenterX;
  uint8_t confidence;
  bool    isFacing;
};

void setupPersonSensor();
PersonResult pollPersonSensor();
