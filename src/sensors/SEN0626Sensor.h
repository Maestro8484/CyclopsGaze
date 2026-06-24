#pragma once

#include <Arduino.h>

#define PERSON_SENSOR_MAX_FACES_COUNT (4)
#define PERSON_SENSOR_MAX_IDS_COUNT (7)

typedef struct __attribute__ ((__packed__)) {
  uint8_t reserved[2];
  uint16_t data_size;
} person_sensor_results_header_t;

typedef struct __attribute__ ((__packed__)) {
  uint8_t box_confidence;
  uint8_t box_left;
  uint8_t box_top;
  uint8_t box_right;
  uint8_t box_bottom;
  int8_t id_confidence;
  int8_t id;
  uint8_t is_facing;
} person_sensor_face_t;

typedef struct __attribute__ ((__packed__)) {
  person_sensor_results_header_t header;
  int8_t num_faces;
  person_sensor_face_t faces[PERSON_SENSOR_MAX_FACES_COUNT];
  uint16_t checksum;
} person_sensor_results_t;

class SEN0626Sensor {
public:
  static constexpr uint8_t DEVICE_ADDR = 0x72;

  enum class Mode { Standby = 0x00, Continuous = 0x01 };

  explicit SEN0626Sensor(HardwareSerial &serial);

  bool begin();
  bool isPresent() { return present; }
  bool read();
  void enableID(bool) {}
  void setMode(Mode) {}
  void enableLED(bool) {}
  int numFacesFound() const { return num_faces; }
  person_sensor_face_t faceDetails(int faceNumber);
  unsigned long timeSinceFaceDetectedMs() { return (unsigned long)lastDetectionTimeMs; }

private:
  static constexpr long SAMPLE_TIME_MS = 150;
  static constexpr uint16_t NATIVE_W = 640;
  static constexpr uint16_t NATIVE_H = 480;
  static constexpr uint8_t BOX_HALF = 32;

  HardwareSerial &serial;
  bool present{false};
  int8_t num_faces{0};
  person_sensor_face_t face{};

  elapsedMillis timeSinceSampledMs{(uint32_t)SAMPLE_TIME_MS};
  elapsedMillis lastDetectionTimeMs{};

  bool tryBaud(long baud);
  uint16_t readFaceData(uint16_t *faceX, uint16_t *faceY, uint16_t *score);
};
