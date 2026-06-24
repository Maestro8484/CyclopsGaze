# CyclopsGaze — Session Notes

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
Shim constructs fake box: cx±32 (0-255 remapped), cy±32. This gives correct
tracking center regardless of fake box size.

No is_facing field: always set is_facing=1 in shim.

Coordinate remap:
  native_width  = 640
  native_height = 480 (ASSUMED — confirm on bench)
  cx255 = faceX * 255 / 640
  cy255 = faceY * 255 / 480
  box_left = cx255 - 32,  box_right = cx255 + 32
  box_top  = cy255 - 32,  box_bottom = cy255 + 32
  box_confidence = score * 255 / 100

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

First clean build: [fill in after bench]
Warnings: [fill in after bench]

## Flash & Verify

Firmware version flashed: CG-S1
Boot message confirmed: [fill in]
SEN0626 present on boot: [fill in]
Face detected in serial output: [fill in]
Eye tracks face on display: [fill in]
AutoMove resumes on face lost: [fill in]

## Issues Found

[fill in after bench]

## Next Session

- [ ] Confirm native Y resolution (480 vs 640) on bench
- [ ] Confirm baud rate on bench
- [ ] Flash to Teensy, verify serial output and eye tracking
- [ ] If tracking direction inverted, flip sign in main.cpp targetX/targetY
