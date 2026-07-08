#include "person_sensor.h"
#include "SEN0626Sensor.h"   // copied in alongside this file (see person_sensor.h porting note)

// SEN0626 on the T4.0's Serial1 (sensor TX->pin 0, sensor RX->pin 1). The old
// driver read the Person Sensor over Wire (I2C 0x62); this reads Modbus over
// UART. The .ino's Wire.begin() (for the paj7620 gesture sensor) is untouched;
// only the Person Sensor transport moves here.
static SEN0626Sensor sensor(Serial1);

// LED state kept for API parity; the SEN0626 has no PS-style LED register, so
// writing it is a no-op (the write path simply does nothing).
bool personSensorLedEnabled = (PERSON_SENSOR_LED_ENABLED != 0);

void setPersonSensorLed(bool on) {
  personSensorLedEnabled = on;   // remembered so PSLED reads back consistently
  // no-op on SEN0626 -- no LED control register exists (confirmed against
  // DFRobot's Modbus map + DFRobot_GestureFaceDetection library).
}

void setupPersonSensor() {
  // Owns Serial1 bring-up + baud auto-detect (115200 -> 9600) + boot settle.
  sensor.begin();
}

PersonResult pollPersonSensor() {
  PersonResult result = {false, false, 0.0f, 0, false};

  // read() is true ONLY when a fresh sample was taken (it self-throttles to
  // SAMPLE_TIME_MS and returns false while throttled or if the sensor is
  // absent). A false here maps to the original short-read case: ok=false, and
  // the caller holds pan. So between samples pan simply holds -- correct.
  if (!sensor.read()) {
    delay(PERSON_SENSOR_DELAY);
    return result;   // ok=false
  }

  result.ok = true;

  if (sensor.numFacesFound() > 0) {
    person_sensor_face_t f = sensor.faceDetails(0);
    // Confidence + facing gate. SEN0626 has no facing bit (is_facing is
    // hardcoded 1 by the shim), so the facing half is always satisfied -- this
    // matches IRIS's own psFacingRequired=false decision (S153c: the real PS
    // facing bit flickered and dropped locks). Gate is confidence-only, at the
    // DFRobot-derived floor.
    if (f.box_confidence > PS_SERVO_CONF_GATE) {
      result.faceVisible = true;
      // SEN0626 stores the face CENTER in both box edges, so (left+right)/2 is
      // the exact center -- identical formula to the original driver, in the
      // same 0-255 space updatePanFromFace() already expects. Pan DIRECTION
      // (whether center-left pans the head the correct way) is unchanged from
      // the old sensor and stays a downstream bench check in updatePanFromFace,
      // NOT something this adapter flips.
      result.faceCenterX = (f.box_left + f.box_right) / 2.0f;
      result.confidence  = f.box_confidence;
      result.isFacing    = f.is_facing;
    }
  }

  delay(PERSON_SENSOR_DELAY);
  return result;
}
