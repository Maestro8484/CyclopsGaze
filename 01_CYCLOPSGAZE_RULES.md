# CyclopsGaze — Session Rules

## Source of truth
Local Repo: C:\Users\SuperMaster\Documents\PlatformIO\CyclopsGaze
Live repo files only. Never write from memory. Read before every edit.

## Status terminology
- REPO-ONLY: written/committed locally, not flashed
- DEPLOYED: flashed to Teensy
- VERIFIED: flashed + behavior confirmed on bench
Nothing is "done" until VERIFIED.

## Firmware discipline
- Bump `FIRMWARE_VERSION` in config.h before every flash
- After flash, confirm version string appears in serial monitor
- Never claim DEPLOYED without observing the serial boot message

## File discipline
- Full file writes for new files
- Read live file before any edit to existing file
- Never guess file contents — read them

## Commit discipline
- One logical change per commit
- Update NOTES.md with actual findings before closing session
- Never defer NOTES.md to next session

## Scope
- Standalone testbed only — no Pi4, no IRIS pipeline
- Touch only CyclopsGaze repo — IRIS-Robot-Face is read-only source
- Do not add features beyond the task (no mouth, no sleep, no serial bridge)

## SEN0626 discipline
- Fetch datasheet/packet format before writing driver — never assume packet layout
- Confirm baud rate on bench via USB-UART adapter before wiring to Teensy
- Document confirmed packet format in NOTES.md

## Session close checklist
- [ ] Clean build confirmed
- [ ] FIRMWARE_VERSION updated
- [ ] NOTES.md reflects actual packet format, baud rate, and any wiring findings
- [ ] README.md wiring table accurate
- [ ] Commit created
- [ ] Status of Teensy firmware stated (always REPO-ONLY at close)
