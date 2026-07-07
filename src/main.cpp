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
