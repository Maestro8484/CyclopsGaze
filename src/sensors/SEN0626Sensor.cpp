#include "SEN0626Sensor.h"

// ── Modbus RTU helpers ────────────────────────────────────────────────────────

static uint16_t modbusCRC(const uint8_t *data, size_t len) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (int j = 0; j < 8; j++) {
      if (crc & 0x0001) crc = (crc >> 1) ^ 0xA001;
      else crc >>= 1;
    }
  }
  return crc;
}

// FC04: Read Input Registers — addr=device address, reg=first register, count=number of registers.
// Returns count*2 payload bytes into buf. Returns true on success.
static bool modbusReadInputRegs(HardwareSerial &ser, uint8_t addr,
                                 uint16_t reg, uint8_t count, uint8_t *buf) {
  uint8_t req[8];
  req[0] = addr;
  req[1] = 0x04;
  req[2] = reg >> 8;
  req[3] = reg & 0xFF;
  req[4] = 0x00;
  req[5] = count;
  uint16_t crc = modbusCRC(req, 6);
  req[6] = crc & 0xFF;
  req[7] = crc >> 8;

  while (ser.available()) ser.read();
  ser.write(req, 8);
  ser.flush();

  // Response: addr(1) + FC(1) + byte_count(1) + data(count*2) + CRC(2)
  const uint8_t respLen = 5 + count * 2;
  uint32_t deadline = millis() + 300;
  while (ser.available() < respLen && millis() < deadline) {}
  if (ser.available() < respLen) return false;

  uint8_t resp[32];
  for (uint8_t i = 0; i < respLen; i++) resp[i] = ser.read();

  // Validate CRC
  uint16_t rcrc = modbusCRC(resp, respLen - 2);
  if (resp[respLen - 2] != (rcrc & 0xFF) || resp[respLen - 1] != (rcrc >> 8)) return false;

  // Validate header
  if (resp[0] != addr || resp[1] != 0x04 || resp[2] != count * 2) return false;

  memcpy(buf, &resp[3], count * 2);
  return true;
}

// ── SEN0626Sensor ─────────────────────────────────────────────────────────────

SEN0626Sensor::SEN0626Sensor(HardwareSerial &serial) : serial(serial) {}

bool SEN0626Sensor::tryBaud(long baud) {
  serial.begin(baud);
  delay(500);
  // Read PID register (input register 0x00), expect 0x0272
  uint8_t buf[2];
  if (modbusReadInputRegs(serial, DEVICE_ADDR, 0x00, 1, buf)) {
    uint16_t pid = (buf[0] << 8) | buf[1];
    if (pid == 0x0272) return true;
  }
  serial.end();
  return false;
}

bool SEN0626Sensor::begin() {
  // Try 115200 first per spec, then 9600 (factory default)
  for (long baud : {115200L, 9600L}) {
    if (tryBaud(baud)) {
      Serial.printf("[CG] SEN0626 found at %ld\n", baud);
      present = true;
      return true;
    }
  }
  Serial.println("[CG] SEN0626 NOT FOUND at 115200 or 9600 -- check wiring");
  present = false;
  return false;
}

// Reads face count, X, Y, score in a single Modbus transaction.
// Registers 0x04..0x07 are contiguous: face_number, face_x, face_y, face_score.
// Returns face count (0 if read fails). Sets faceX, faceY, score on success.
uint16_t SEN0626Sensor::readFaceData(uint16_t *faceX, uint16_t *faceY, uint16_t *score) {
  uint8_t buf[8];
  if (!modbusReadInputRegs(serial, DEVICE_ADDR, 0x04, 4, buf)) return 0;
  uint16_t count = (buf[0] << 8) | buf[1];
  *faceX  = (buf[2] << 8) | buf[3];
  *faceY  = (buf[4] << 8) | buf[5];
  *score  = (buf[6] << 8) | buf[7];
  return count;
}

bool SEN0626Sensor::read() {
  if (!present) return false;
  if (timeSinceSampledMs < (uint32_t)SAMPLE_TIME_MS) return false;
  timeSinceSampledMs = 0;

  uint16_t faceX, faceY, score;
  uint16_t count = readFaceData(&faceX, &faceY, &score);

  if (count == 0) {
    num_faces = 0;
    return true;
  }

  // Clamp and remap center coords to 0-255
  faceX = min(faceX, (uint16_t)NATIVE_W);
  faceY = min(faceY, (uint16_t)NATIVE_H);
  score = min(score, (uint16_t)100);

  uint8_t cx = (uint8_t)((uint32_t)faceX * 255 / NATIVE_W);
  uint8_t cy = (uint8_t)((uint32_t)faceY * 255 / NATIVE_H);

  face.box_confidence = (uint8_t)(score * 255 / 100);
  face.box_left   = (cx > BOX_HALF) ? cx - BOX_HALF : 0;
  face.box_right  = (cx + BOX_HALF < 255) ? cx + BOX_HALF : 255;
  face.box_top    = (cy > BOX_HALF) ? cy - BOX_HALF : 0;
  face.box_bottom = (cy + BOX_HALF < 255) ? cy + BOX_HALF : 255;
  face.id_confidence = 0;
  face.id = 0;
  face.is_facing = 1;

  num_faces = 1;
  lastDetectionTimeMs = 0;
  return true;
}

person_sensor_face_t SEN0626Sensor::faceDetails(int faceNumber) {
  if (faceNumber >= num_faces) return person_sensor_face_t{};
  return face;
}
