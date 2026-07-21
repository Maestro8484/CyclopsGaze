# Hardware — mounts & enclosures

Physical mounting designs for the CyclopsGaze rig (Teensy + GC9A01A eye + SEN0626 camera):
3D-printable parts in [`stl/`](stl/), laser-cuttable panels in [`lasercut/`](lasercut/).

## Status: no verified designs yet

This directory is a scaffold, not a parts bin. No mount has been designed, measured against
the real hardware, printed, or cut — there are currently zero files in `stl/` or `lasercut/`
beyond their own README placeholders. Nothing here should be treated as fit-for-purpose until
a design goes through the same discipline as the firmware: REPO-ONLY → DEPLOYED → VERIFIED
(see [`../RULES.md`](../RULES.md)). For hardware that means modeled → printed/cut →
test-fitted on the real Teensy/display/sensor.

## What's needed before a design can start

No physical dimensions for any of these are recorded anywhere in this repo yet. Whoever
designs the first mount should measure (calipers, not a datasheet guess) and record here:

- **GC9A01A round display module** — outer PCB diameter, mounting-hole positions (if any),
  viewable circle diameter, connector-side clearance.
- **DFRobot SEN0626 Gravity board** — full board outline, mounting holes, connector/header
  clearance. See the open question below — this may not be the right footprint to design for.
- **Teensy 4.1** — standard footprint (well-documented by PJRC), but confirm which mounting
  holes are actually used once the enclosure layout is known.
- Cable/connector clearance for the SPI ribbon to the display and the UART leads to the sensor.

## Open question this affects: detached camera module

[`../docs/ENGINEERING_LOG.md`](../docs/ENGINEERING_LOG.md) (CG-S10) and
[`../docs/WIRING.md`](../docs/WIRING.md) §"Compact Mounting" found that the SEN0626's camera
lens/sensor is physically separable from the rest of the Gravity board (ribbon + tape, not
solder), which would allow a much smaller mount than the full board footprint — closer to the
original Person Sensor's tiny form factor. That finding is explicitly unverified: it hasn't
been confirmed that the detached camera still answers on the same register set once separated
from the main board's circuitry. Any mount design should pick one of two paths and say which:

1. **Full-board mount** — design for the complete Gravity board footprint. Safe today, bulkier.
2. **Detached-camera mount** — design for just the camera + ribbon. Smaller, but depends on
   the CG-S10 finding being confirmed first (raw register output compared attached vs. detached).

## Contributing a design

- State the source of your measurements (measured off the physical part vs. a vendor drawing)
  and the tool used (FreeCAD, Fusion 360, etc. — note it here even if the export is the only
  artifact checked in).
- Test-fit before calling it done: printed/cut and physically test-fit against the real parts,
  not just checked in a slicer/preview.
- Follow the same status language as the firmware — call it REPO-ONLY until it's been
  printed/cut and test-fit, VERIFIED only after.
- Add a short entry to [`../CHANGELOG.md`](../CHANGELOG.md) the same way firmware changes are
  logged, and note the design's target path (full-board vs. detached-camera) from the section
  above.
