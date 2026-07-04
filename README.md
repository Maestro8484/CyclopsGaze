# CyclopsGaze

Standalone single-eye animatronic face-tracking testbed **and the durable IRIS gaze-node repo**.

Teensy 4.1 + GC9A01A 240x240 round display + DFRobot SEN0626 AI camera.
SEN0626 driven via Modbus RTU over UART1 with a drop-in shim matching the Useful Sensors
Person Sensor (SEN-21231) interface used in the IRIS project.

## Purpose

Testbed for validating SEN0626 as a replacement for the unreliable/EOL
SEN-21231 before deploying to IRIS-Robot-Face. Runs on Teensy 4.1 as of CG-S2
(migrated from T4.0 when the T4.1 arrived 2026-07-03).

**This is the surviving gaze-node design.** As of 2026-07-04 (IRIS HANDOFF C) CyclopsGaze
is the durable gaze-node identity; the earlier ESP32-S3 + OV2640 camera node (`../OGLE`) is
archived/tombstoned (it never reached reliable end-to-end tracking). The IRIS integration is
a **native Person-Sensor-bus drop-in** — the SEN0626 shim is read directly by IRIS's own
Teensy 4.1, not via a Pi4 bridge. Full plan: [09_IRIS_INTEGRATION_PLAN.md](09_IRIS_INTEGRATION_PLAN.md).
History: [CHANGELOG.md](CHANGELOG.md). Current bench status: [NOTES.md](NOTES.md).

## Wiring

### GC9A01A Display

| Signal | Teensy 4.0 Pin |
|--------|---------------|
| CS     | 10            |
| DC     | 2             |
| MOSI   | 11            |
| SCK    | 13            |
| RST    | 3             |
| VCC    | 3.3V          |
| GND    | GND           |

### SEN0626 AI Camera

| SEN0626 | Teensy 4.0 Pin |
|---------|---------------|
| TX      | 0 (Serial1 RX)|
| RX      | 1 (Serial1 TX)|
| VCC     | 3.3V          |
| GND     | GND           |

## SEN0626 Config

- Protocol: Modbus RTU (FC04 input registers)
- Device address: 0x72
- Baud rate: 9600 (factory default)
- Coordinate range X: 0-640, Y: 0-480 (assumed VGA, confirm on bench)
- Face score: 0-100
- Interface: UART (Serial1, pins 0/1)

## Build & Flash

```powershell
cd C:\Users\SuperMaster\Documents\PlatformIO\CyclopsGaze
pio run -e cyclopsgaze            # build
pio run -e cyclopsgaze -t upload  # flash
pio device monitor                # serial monitor
```

## Serial Output

```
[CG] CyclopsGaze CG-S1
[CG] SEN0626 found at 9600
[CG] faces=1 conf=180 x=0.12 y=-0.05
```

## Status

| Component | Status |
|-----------|--------|
| SEN0626 Modbus RTU driver | REPO-ONLY |
| GC9A01A display | REPO-ONLY |
| Face tracking | REPO-ONLY |
| Firmware | REPO-ONLY |

## Relation to IRIS

Parent project: IRIS-Robot-Face (private)
Successful validation here → SEN0626Sensor.h/.cpp promoted to IRIS src/sensors/
