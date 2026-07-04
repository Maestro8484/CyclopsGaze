# CyclopsGaze — Session Notes

## SEN0626 Bench Verification

Date: 
Baud rate confirmed: 
Method: USB-UART adapter → SuperMaster serial terminal

Raw packet sample (hex):


## SEN0626 Packet Format

Source: [URL of datasheet/wiki used]

Header bytes:
Face count field: byte offset X, length Y
Bounding box fields:
  x1: offset X, length Y, scale 0-N
  y1: offset X, length Y, scale 0-N
  x2: offset X, length Y, scale 0-N
  y2: offset X, length Y, scale 0-N
Confidence field: offset X, length Y, scale 0-N
Native resolution: W x H pixels
is_facing equivalent: yes/no — field details:

## Coord Remap

native_width  = 
native_height = 
Remap formula verified: yes/no

## Build

First clean build: yes/no
Warnings: 

## Flash & Verify

Firmware version flashed: CG-S1
Boot message confirmed: yes/no
SEN0626 present on boot: yes/no
Face detected in serial output: yes/no
Eye tracks face on display: yes/no
AutoMove resumes on face lost: yes/no

## Deviations from Handoff

None / [list any]

## Issues Found

None / [list any]

## Next Session

- [ ] [carry-forward items here]
