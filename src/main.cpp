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

  if (sampled && sensor.numFacesFound() > 0) {
    person_sensor_face_t f = sensor.faceDetails(0);
    float cx = f.box_left;
    float cy = f.box_top;
    float targetX = -((cx / 127.5f) - 1.0f);
    float targetY =  ((cy / 127.5f) - 1.0f);

    eyes->setAutoMove(false);
    eyes->setTargetPosition(targetX, targetY);
    Serial.printf("[CG] faces=%d conf=%d x=%.2f y=%.2f\n",
                  sensor.numFacesFound(), f.box_confidence, targetX, targetY);

  } else if (sensor.timeSinceFaceDetectedMs() > 3000) {
    eyes->setAutoMove(true);
  }

  eyes->renderFrame();
}
