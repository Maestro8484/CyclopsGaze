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

  // Gate on confidence (face.box_confidence > PS_CONF_GATE). CG-S12: box_confidence
  // is now the RAW DFRobot score (0-100) and PS_CONF_GATE=60 is DFRobot's own
  // documented validity floor (score >=60, wiki.dfrobot.com/sen0626/docs/23024)
  // -- same effective threshold as the old 152/255, just the vendor's native
  // scale. Tune further only against real bench scores (audit 3.7, docs).
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
    // these are the exact face center. CG-S12 (synced from IRIS S212c, ⚠ UNVERIFIED
    // since the change -- re-bench #1 priority): per-axis signed gain + bias,
    // targetN = rawN * GAIN + BIAS with rawN = (cN/127.5)-1 the sensor-space
    // target in [-1,+1]. The gain SIGN is direction, its MAGNITUDE is range, so
    // one knob covers both "mirrored" and "barely moves". This replaced the older
    // single GAZE_GAIN + Y_CENTER scheme; the config.h defaults are algebraically
    // identical to the bench-VERIFIED CG-S6 (no X negation -- our single eye is
    // eyeIndex 0, already X-flipped by EyeController), CG-S7 (1.7 range) and CG-S8
    // (Y_BIAS carries the below-eye-mount compensation Y_CENTER=33 used to apply).
    // setTargetPosition clamps to the unit circle, so |gain|>1 is safe (audit 3.8).
    float cx = f.box_left;
    float cy = f.box_top;
    float rawX = (cx / 127.5f) - 1.0f;
    float rawY = (cy / 127.5f) - 1.0f;
    float targetX = rawX * GAZE_X_GAIN + GAZE_X_BIAS;
    float targetY = rawY * GAZE_Y_GAIN + GAZE_Y_BIAS;

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
