# CyclopsGaze — Session Notes

## Purpose & Deployment Framing (operator, 2026-07-04)

CyclopsGaze is a **future-proofing / public-launch hardware change**, NOT a fix
for a current problem. IRIS's live Useful Sensors Person Sensor (SEN-21231) is
**working fine for the operator right now** — nothing about IRIS is broken and
nothing here changes live IRIS. This project exists because the Person Sensor is
**discontinued** [[person_sensor_irreplaceable]], so a drop-in replacement is
needed (a) as insurance if the live sensor ever dies, and (b) as the publicly-
**replicable** gaze path for the IRIS launch (buyers can't source a discontinued
part).

Deployment posture: **bench-only, on a spare NON-INSTALLED Teensy 4.1** — do NOT
pull the live Person Sensor or touch installed IRIS hardware to test this. Flash
+ verify happens on the standalone bench board at a later date, when the T4.1 is
connected. No timeline pressure; the live Person Sensor stays the source of
truth until CyclopsGaze is proven under load. Status stays REPO-ONLY until then.

## SEN0626 Protocol (from DFRobot GitHub, 2026-06-24)

Source: https://github.com/DFRobot/DFRobot_GestureFaceDetection

Protocol: Modbus RTU over UART (NOT raw binary packets as originally anticipated in handoff)
Device address: 0x72 (from DFRobot example detectGesture.ino)
Default baud rate: 9600 (factory default; 115200 also supported via configUart)
PID register: input reg 0x00 = 0x0272 (used for presence detection in begin())

Registers (input, FC04):
  0x00 = PID         (expect 0x0272)
  0x01 = VID
  0x04 = face_number (0-3, number of faces detected)
  0x05 = face_x      (center X, 0-640)
  0x06 = face_y      (center Y, 0-480 assumed VGA — unconfirmed, verify on bench)
  0x07 = face_score  (0-100)

No bounding box: SEN0626 returns face CENTER (X, Y), not a box.
Shim stores the SAME center in BOTH edges (box_left==box_right==cx,
box_top==box_bottom==cy) so IRIS-side consumers recover the exact target
center even at frame edges. (Superseded the earlier cx±32 fake-box scheme —
the b84033d "center box" fix commit; a fake width biased the recovered
center near the edges.)

No is_facing field: always set is_facing=1 in shim.
  KNOWN GAP: the real Person Sensor is_facing (is_looking_at) drives IRIS
  facing-gate logic. SEN0626 has no direct equivalent here; hardcoded true
  means "facing" is always asserted. If IRIS integration (HANDOFF C) needs a
  real facing flag, derive it from SEN0626 head-pose/gesture registers.

Coordinate remap (matches SEN0626Sensor.cpp read()):
  NATIVE_W = 640
  NATIVE_H = 480 (ASSUMED — confirm on bench)
  cx = faceX * 255 / 640   (clamped faceX<=640)
  cy = faceY * 255 / 480   (clamped faceY<=480)
  box_left = box_right = cx
  box_top  = box_bottom = cy
  box_confidence = score * 255 / 100   (score clamped 0-100)

Modbus multi-register read: registers 0x04-0x07 read in one FC04 transaction
(4 registers contiguous), ~22ms per read at 9600 baud. SAMPLE_TIME_MS = 150.

Deviations from handoff:
1. Protocol is Modbus RTU, not raw streaming packets — "check serial.available()"
   in begin() would not work; replaced with Modbus PID register read.
2. No bounding box — fake box constructed from center coords.
3. No is_facing field — hardcoded true.
4. Native Y resolution unconfirmed (480 assumed). Verify on bench.
5. SAMPLE_TIME_MS raised to 150ms (from 70ms) because 4 Modbus reads +
   inter-frame delay takes ~42ms; 150ms gives clean margin.

## Build

Target: Teensy 4.1 (migrated from T4.0; T4.1 arrived 2026-07-03).
  platformio.ini board = teensy41 (CG-S2). Wiring/pins IDENTICAL to T4.0 per
  05_WIRING.md — the board line was the only change needed.
First clean build (CG-S2, T4.1, 2026-07-04): SUCCESS, 8.33s.
  FLASH: code 74012, data 361032 -> free for files 7.68 MB
  RAM1: variables 11072, code 69544 -> free for locals 414912
  RAM2: variables 12416 -> free for malloc 511872
Warnings: none observed in final link.

Drop-in contract check (2026-07-04): SEN0626Sensor's person_sensor_face_t
matches IRIS src/sensors/PersonSensor.h byte-for-byte, and the public method
surface matches (isPresent/begin/read/enableID/setMode/enableLED/
numFacesFound/faceDetails/timeSinceFaceDetectedMs). Confirmed true drop-in.

## Flash & Verify

Firmware version in repo: CG-S2  (REPO-ONLY — NOT flashed this session)
Boot message confirmed: [pending — no Teensy enumerated on SuperMaster this session]
SEN0626 present on boot: [pending bench]
Face detected in serial output: [pending bench]
Eye tracks face on display: [pending bench]
AutoMove resumes on face lost: [pending bench]

## Issues Found

- No Teensy 4.1 enumerated on SuperMaster during CG-S2 (only COM1 legacy +
  COM4/COM5 Bluetooth). Could not flash or bench-verify. Connect the T4.1 via
  USB before the next flash session.

## Next Session (bench — needs T4.1 connected + operator at bench)

- [ ] Connect T4.1 via USB, confirm it enumerates (pio device list)
- [ ] Flash CG-S2, confirm "[CG] CyclopsGaze CG-S2" on serial @115200
- [ ] Confirm "[CG] SEN0626 found at 115200" or "found at 9600" (record which)
- [ ] Confirm native Y resolution (480 vs 640) — hold a face, watch y saturate
- [ ] Confirm face detect line: "[CG] faces=1 conf=NNN x=N.NN y=N.NN"
- [ ] Confirm eye tracks L/R/U/D; AutoMove resumes ~3s after face leaves frame
- [ ] If tracking direction inverted, flip sign in main.cpp targetX/targetY
