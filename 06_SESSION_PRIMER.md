# CyclopsGaze — Claude Code Session Primer

Paste this as your FIRST message in Claude Code.

---

CYCLOPSGAZE SESSION START

Model: claude-sonnet-4-6
Repo: C:\Users\SuperMaster\Documents\PlatformIO\CyclopsGaze
IRIS source (read-only): C:\Users\SuperMaster\Documents\PlatformIO\IRIS-Robot-Face

Hardware is wired and ready per 05_WIRING.md.
Teensy 4.0 connected via USB.
SEN0626 wired to Teensy — baud auto-detected on first flash, no pre-check needed.
GC9A01A wired to Teensy.

MANDATORY FIRST ACTIONS (in order):
1. Read CYCLOPSGAZE_RULES.md from repo root
2. git status — confirm on main, clean tree
3. Read 02_CLAUDE_CODE_HANDOFF.md from repo root and execute top to bottom

DO NOT begin implementation until:
- PersonSensor.h interface contract read from live IRIS repo
- SEN0626 packet format fetched from live DFRobot wiki
- All IRIS source files read live before copying
