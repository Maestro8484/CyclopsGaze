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
  if (sensor.read()) {
    int n = sensor.numFacesFound();
    if (n > 0) {
      // Find largest face by bounding box area
      int bestIdx = 0;
      int bestArea = 0;
      for (int i = 0; i < n; i++) {
        person_sensor_face_t f = sensor.faceDetails(i);
        int area = (f.box_right - f.box_left) * (f.box_bottom - f.box_top);
        if (area > bestArea) { bestArea = area; bestIdx = i; }
      }

      person_sensor_face_t f = sensor.faceDetails(bestIdx);
      float cx = f.box_left + (f.box_right  - f.box_left) / 2.0f;
      float cy = f.box_top  + (f.box_bottom - f.box_top)  / 3.0f;
      float targetX = -((cx / 127.5f) - 1.0f);
      float targetY =  ((cy / 127.5f) - 1.0f);

      eyes->setAutoMove(false);
      eyes->setTargetPosition(targetX, targetY);
      Serial.printf("[CG] faces=%d conf=%d x=%.2f y=%.2f\n", n, f.box_confidence, targetX, targetY);

    } else if (sensor.timeSinceFaceDetectedMs() > 3000) {
      eyes->setAutoMove(true);
    }
  }

  eyes->renderFrame();
}
