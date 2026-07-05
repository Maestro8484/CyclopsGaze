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

  // Gate on confidence exactly like IRIS (face.box_confidence > psConfGate,
  // default 45). The shim emits box_confidence = score*255/100, so with strict
  // '>' the minimum passing SEN0626 score is 19/100 (18 -> 45, not > 45; 19 ->
  // 48). Tune PS_CONF_GATE on the bench if clear faces score lower (audit 3.7).
  person_sensor_face_t f = sensor.faceDetails(0);
  bool haveFace = sampled && sensor.numFacesFound() > 0 && f.box_confidence > PS_CONF_GATE;

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
    // One-time bench calibration: raw register values expose the true max Y
    // (confirm NATIVE_H 480 vs 640) and raw score vs rescaled conf. Set
    // CG_CALIB_RAW to 0 in config.h to compile this out for normal operation.
    Serial.printf("[CG] faces=%d conf=%d x=%.2f y=%.2f | rawX=%u rawY=%u rawScore=%u\n",
                  sensor.numFacesFound(), f.box_confidence, targetX, targetY,
                  sensor.rawFaceX(), sensor.rawFaceY(), sensor.rawScore());
#else
    Serial.printf("[CG] faces=%d conf=%d x=%.2f y=%.2f\n",
                  sensor.numFacesFound(), f.box_confidence, targetX, targetY);
#endif

  } else if (sensor.timeSinceFaceDetectedMs() > FACE_LOST_MS) {
    // Runs whether read() returned false (throttle / comms failure) or returned
    // true with no qualifying face, so autoMove always resumes and never sticks
    // OFF when the sensor stops responding (audit 3.2).
    eyes->setAutoMove(true);
  }

  eyes->renderFrame();
}
