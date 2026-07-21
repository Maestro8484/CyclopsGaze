# Lasercut: panel/enclosure designs

Laser-cuttable 2D designs (stand-off panels, brackets, a boxed enclosure, etc.) go here as `.svg`
(and/or `.dxf` for cutters that prefer it), with kerf and material thickness noted per file. A
laser-cut fit is far less forgiving of an unstated kerf than a 3D print is of an unstated tolerance.

**Empty today.** See [`../README.md`](../README.md) for what's needed before a design can start
(measured dimensions, none are recorded in this repo yet, and the full-board-vs-detached-camera
decision for the SEN0626 mount).

## Naming convention (once files exist)

`<part>_<material>t<thickness_mm>_v<n>.svg`, e.g. `base_panel_acrylic3mm_v1.svg`. Keep cut-line and
engrave-line layers separate and labeled in the SVG (most laser software keys off layer/color).

## Cut notes to record per file

- Material + thickness the design was cut for, and the **kerf** compensation baked into the paths
  (or explicitly note if kerf compensation was left to the operator's software).
- Which laser cutter parameters (or a cutter class) it was tested on, if any. Settings vary
  enough between machines that "it worked on mine" doesn't generalize without saying which "mine."
- Whether the design has actually been cut and test-fit, or is geometry-only / unverified.
