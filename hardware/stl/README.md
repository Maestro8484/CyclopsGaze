# STL: 3D-printable mounts

3D-printable parts (Teensy tray, eye bezel, sensor mount, enclosure, etc.) go here as `.stl`
(print-ready mesh) and, ideally, the editable source (`.f3d`, `.step`, `.scad`, or similar) beside
it so the design can be revised, not just reprinted.

**Empty today.** See [`../README.md`](../README.md) for what's needed before a design can start
(measured dimensions, none are recorded in this repo yet, and the full-board-vs-detached-camera
decision for the SEN0626 mount).

## Naming convention (once files exist)

`<part>_<eye-config>_v<n>.stl`, e.g. `sensor_mount_singleeye_v1.stl`,
`eye_bezel_dualeye_v1.stl`. Keep the editable source under the same base name.

## Print notes to record per part

- Material tested (PLA/PETG/etc.), infill %, and any supports required.
- Fit tolerance used for press-fit features (screw bosses, snap clips).
- Which physical unit it was test-fit against (bench T4.1 + GC9A01A + SEN0626, per
  [`../../docs/WIRING.md`](../../docs/WIRING.md)) and the date.
