#pragma once

static constexpr char FIRMWARE_VERSION[] = "CG-S17";

#include "eyes/eyes.h"

// ─────────────────────────────────────────────────────────────────────────────
// Eye artwork: pick one (CG-S16)
// ─────────────────────────────────────────────────────────────────────────────
//
// Ten eye designs ship with this repo. Enabling one is two uncomments that have
// to agree: the #include here, and the matching row in eyeDefinitions below.
// That pairing is upstream TeensyEyes' own mechanism (its src/config.h does
// exactly this with 23 eyes), adopted rather than reinvented.
//
// A disabled eye costs ZERO flash. Measured on this toolchain at CG-S16, not
// assumed: the Teensy builder passes -fdata-sections and -Wl,--gc-sections, so
// tables nothing references are dropped at link time. Controlled A/B, one
// variable, everything else identical: hazel.h included but its row commented
// out gave FLASH code 83,836 / data 362,056, and commenting the #include as well
// gave exactly the same two figures. Both equal the CG-S13 baseline from before
// any second eye existed, so the eight bundled-but-disabled headers and the ten
// bundled table .cpp files cost nothing at all.
//
// Consequence: a stale #include costs build time only, never flash. The failure
// mode that actually matters is the reverse, uncommenting a row without its
// #include, and that is a compile error naming the eye.
//
// All ten share polar.mapRadius 240, so any of them may also be swapped at
// runtime (see the rotation note in eyeDefinitions). What actually differs:
//
//   eye         eyeball  surround      iris    pupil       notes
//   nordicBlue  125 px   black         69 px   0.25-0.47   original artwork, default
//   bigBlue     125 px   black         85 px   0.30-0.70   realistic, largest human iris
//   hazel       125 px   colour 35138  60 px   0.30-0.70   realistic, small iris
//   doe         130 px   black         95 px   0.30-0.40   animal, has left/right
//   dragon      125 px   black         90 px   0.10-0.20   slit pupil, black sclera
//   cat         125 px   colour 65504  90 px   0.30-0.40   slit pupil, untextured
//   demon       125 px   colour 20480  110 px  0.10-0.25   slit pupil, spinning iris,
//                                                          tracking off, has left/right
//   doomRed     125 px   colour 42260  50 px   0.15-0.40   tiny iris
//   fish        120 px   black         115 px  0.40-0.50   no eyelids
//   skull       120 px   black         70 px   0.10-0.25   no eyelids
//
// "surround" is EyeDefinition::backColor, painted where the eyeball ends but the
// eyelid aperture continues (EyeController.h:435). Raw 565 values are given
// rather than colour names because none of the four has been observed on this
// display yet. nordicBlue's is black, so the coloured ones will not match it.
//
// That is also the answer to "hazel looks off" (operator, bench, CG-S16): hazel
// is byte-identical to upstream and this engine reads its parameters with the
// same expressions upstream does, so nothing is broken. It is authored
// differently: non-black surround, a 60 px iris where nordicBlue has 69, a pupil
// reaching 0.70 of that smaller iris where nordicBlue stops at 0.47, and half
// the squint. Different design, not a defect. docs/ENGINEERING_LOG.md CG-S16.
//
// Adding one of the 13 upstream eyes not bundled here: copy <name>.h from
// TeensyEyes/src/eyes/240x240/, plus every polarDist_240_* / disp_240_* /
// noeyelids_* table its header includes (.h and .cpp both), then add an #include
// and a row. Full upstream list and credits: docs/ATTRIBUTION.md.
#include "eyes/240x240/nordicBlue.h"
//#include "eyes/240x240/bigBlue.h"
//#include "eyes/240x240/cat.h"
//#include "eyes/240x240/demon.h"
//#include "eyes/240x240/doe.h"
//#include "eyes/240x240/doomRed.h"
//#include "eyes/240x240/dragon.h"
//#include "eyes/240x240/fish.h"
//#include "eyes/240x240/hazel.h"
//#include "eyes/240x240/skull.h"

#include "eyes/EyeController.h"

#define USE_GC9A01A
#include "displays/GC9A01A_Display.h"

// ─────────────────────────────────────────────────────────────────────────────
// Sensor transport (CG-S13)
// ─────────────────────────────────────────────────────────────────────────────
//
// Which hardware UART the SEN0626 hangs off. Parameterised rather than hardcoded
// in main.cpp because the live IRIS install had to move it: IRIS runs the sensor
// on Serial4 because pin 0 — Serial1's RX — is its LEFT EYE CS (IRIS src/config.h
// "Gaze sensor transport (S212)"). CyclopsGaze's single display is on CS 10, so
// pins 0/1 are free here and Serial1 stays the bench-VERIFIED default.
//   Serial1 = RX 0  / TX 1    <- CyclopsGaze default (docs/WIRING.md)
//   Serial4 = RX 16 / TX 17   <- what live IRIS uses
// Either way: sensor D/T -> the RX pin, sensor C/R -> the TX pin.
#define SEN0626_SERIAL Serial1

// ─────────────────────────────────────────────────────────────────────────────
// Tracking tunables — COMPILE-TIME DEFAULTS (see docs/BENCH_PROTOCOL.md)
// ─────────────────────────────────────────────────────────────────────────────
//
// CG-S13: these are SEEDS, not the live values. main.cpp copies each into a
// runtime `ps*` variable that the PS_CFG: serial protocol retunes live with no
// reflash (ported from IRIS S141/S212c — see main.cpp processSerial()). A board
// reset reverts to the defaults below; the standalone board has no persistence
// (IRIS persists via the Pi4's ps_config.json and re-pushes it on serial open),
// so once the bench proves a value, write it back here.
//
// The runtime variable names in main.cpp are IRIS's VERBATIM (psConfGate,
// psXGain, psYBias, …) so the two code bases stay directly diffable — keeping
// them from drifting is the entire point of this repo.

// Confidence gate. A face must report box_confidence > psConfGate to be tracked.
// PS_CFG:CONF=n retunes it live (clamped 0-100). CG-S12 (⚠ UNVERIFIED, re-bench
// #1 priority): moved 152 -> 60 because
// SEN0626Sensor now emits the RAW DFRobot score (0-100), not the old 0-255 remap
// (see SEN0626Sensor.cpp CG-S12 note). 60 IS DFRobot's own documented validity
// floor -- the SEN0626 wiki setup guide (wiki.dfrobot.com/sen0626/docs/23024)
// states "a score >=60 is considered valid" and its sample code calls
// gfd.setFaceDetectThres(60). With strict '>' a raw score of 60 passes and 59
// does not. This is the SAME effective threshold as the old bench-VERIFIED
// 152/255 (≈0.60) -- only the scale changed -- but it has NOT been re-observed on
// the bench since the change. Lower only if bench data shows the vendor floor is
// too strict for this specific unit (audit 3.7).
//
// NOTE the live IRIS install runs CONF=25 (observed on the wire 2026-07-21). That
// is NOT a tuned SEN0626 value: it is a leftover from the Person Sensor's 0-255
// scale (~10%) that predates the swap, and IRIS's own S212 source comment flags
// raising it to 60 as an unmade operator decision. Deliberately NOT adopted here.
// See docs/ENGINEERING_LOG.md CG-S13 "IRIS-vs-CyclopsGaze value diff (OPEN)".
static constexpr uint8_t PS_CONF_GATE_DEFAULT = 60;

// Require the face's is_facing bit (PS_CFG:FACING=0/1). Ported from IRIS for
// parity; CyclopsGaze had no facing gate at all before CG-S13. Default FALSE and
// effectively inert here: the SEN0626 has no facing register, so the shim
// hardcodes is_facing=1 (docs/SEN0626_PROTOCOL.md). IRIS also defaults it false —
// its S153c bench found the real Person Sensor's facing bit flickered during
// normal head movement and kept dropping the lock. The knob exists so that a
// future derived facing signal (head-pose/gesture registers) is a one-line change.
static constexpr bool PS_FACING_REQUIRED_DEFAULT = false;

// ── Per-axis signed gain + bias (CG-S12, synced from IRIS S212c) ─────────────
// Gaze shaping model: targetN = rawN * GAIN + BIAS, where rawN = (cN/127.5)-1 is
// the sensor-space target in [-1,+1] (cN = box center on the shim's 0-255 scale).
// The SIGN of the gain sets direction and the MAGNITUDE sets range, so one knob
// covers both "it's mirrored" and "it barely moves". setTargetPosition clamps the
// result to the unit circle, so |gain|>1 saturates gracefully (audit 3.8).
// PS_CFG:X_GAIN / Y_GAIN / X_BIAS / Y_BIAS retune all four live.
//
// ⚠ UNVERIFIED (re-bench #1 priority). This replaced the single GAZE_GAIN + the
// Y_CENTER offset with IRIS's proven per-axis model. The defaults below are
// algebraically equal to the old bench-VERIFIED CG-S6/S7/S8 behavior:
//   * X_GAIN = +1.7  positive = un-mirrored. CG-S6 removed the targetX negation
//     (CyclopsGaze's single eye is eyeIndex 0, which EyeController already
//     X-flips), and CG-S7 set the 1.7 magnitude from a measured rawX span.
//   * Y_GAIN = +1.7, Y_BIAS = +1.26  together reproduce the old CG-S8
//     Y_CENTER=33 below-eye-mount compensation exactly:
//       old:  ((cy-33)/127.5)*1.7
//       new:  ((cy/127.5)-1)*1.7 + 1.26   [1.26 = 1.7*(1 - 33/127.5)]
//     i.e. a face at true eye level (cy≈33) still maps to targetY≈0. Re-measure
//     Y_BIAS if the sensor's mount height relative to the eye changes.
//   * X_BIAS = 0  horizontal centering offset; 0 = symmetric.
//
// These are CyclopsGaze's OWN measured numbers and are deliberately kept over the
// live IRIS values (X_GAIN=Y_GAIN=1.0, both biases 0.0, observed 2026-07-21).
// Those IRIS values are the firmware's untuned compile-time defaults — the Pi4's
// ps_config.json predates the SEN0626 swap and carries no gain/bias keys at all —
// so adopting them would trade measured data for unmeasured. Gain and bias are
// mount-geometry-specific anyway (IRIS: two eyes, its own sensor height;
// CyclopsGaze: one eye, sensor mounted below it). A head-to-head behavioral
// comparison of the two sets is an OPEN task — docs/ENGINEERING_LOG.md CG-S13.
static constexpr float GAZE_X_GAIN_DEFAULT = 1.7f;
static constexpr float GAZE_Y_GAIN_DEFAULT = 1.7f;
static constexpr float GAZE_X_BIAS_DEFAULT = 0.0f;
static constexpr float GAZE_Y_BIAS_DEFAULT = 1.26f;

// Time with no qualifying face before autoMove (idle wander) resumes.
// PS_CFG:LOST_MS=n retunes it live. (Live IRIS runs 8500 — tuned for a
// conversational robot that should hold the operator's gaze through pauses, not
// for a bench rig where a fast wander-resume is what you actually want to watch.)
static constexpr unsigned long PS_LOST_MS_DEFAULT = 3000;

// ─────────────────────────────────────────────────────────────────────────────
// Eye appearance sets (CG-S15)
// ─────────────────────────────────────────────────────────────────────────────
//
// How long each eye set is shown before auto-rotating to the next one, and
// whether rotation runs at all. Both are runtime values retunable over serial
// with the `EYE:` protocol (main.cpp) -- deliberately a SEPARATE command
// namespace from `PS_CFG:`, because PS_CFG is IRIS-verbatim and adding keys to
// it would break the diffability this repo exists to preserve.
//   EYE:AUTO=0/1   stop/start rotation
//   EYE:MS=n       change the interval
//   EYE:next       advance immediately
//   EYE:<name>     select by name, and turn rotation off
//   EYE?           list sets, show current + rotation state
static constexpr uint32_t EYE_ROTATE_MS_DEFAULT  = 20000;
static constexpr bool     EYE_AUTO_ROTATE_DEFAULT = true;

// One-time bench calibration logging. When 1, the per-face serial line also
// prints raw sensor register values (rawX/rawY/rawScore) so the operator can
// read the true max Y (confirm NATIVE_H 480 vs 640) and raw score vs the
// rescaled confidence without a scope. Set to 0 for normal operation (audit
// 3.5). Kept ON by default until the first bench pass confirms the assumptions.
#define CG_CALIB_RAW 1

// ─────────────────────────────────────────────────────────────────────────────
// Displays / eyes
// ─────────────────────────────────────────────────────────────────────────────
//
// DUAL_EYE: uncomment to drive a second GC9A01A. Single-eye is the default and
// needs no define. See docs/WIRING.md "Dual-Eye (optional)" for the pin table.
//
// Why both eyes share SPI0 (MOSI 11 / SCK 13) instead of the IRIS-style second
// bus: on this board Serial1 (SEN0626) already owns pins 0 and 1. Teensy 4.1's
// SPI1 collides with BOTH -- its hardware CS is pin 0 and its default MISO is
// pin 1 -- so a second bus cannot be used without breaking the sensor UART.
// A shared SPI0 with a separate CS per display sidesteps that entirely (SPI0's
// MISO, pin 12, is free and unused by the write-only displays). Updates are
// synchronous and renderFrame() drives one eye per call, so there is no bus
// contention; the only cost is per-eye refresh rate roughly halving.
//
// #define DUAL_EYE

#ifdef DUAL_EYE

// Eye sets, dual-eye build. Outer index = set (rotation / EYE:next order),
// inner = per-display, so both displays get the same look. The set NAME is read
// back out of EyeDefinition::name (eyes.h), so there is no parallel name table
// to drift. Uncomment a row plus its #include at the top of this file; the array
// size is deduced, so there is no count to keep in step.
//
// Leave exactly one row uncommented for a fixed look. Uncomment two or more and
// EYE:/auto-rotation cycles them (EYE_AUTO_ROTATE_DEFAULT below).
//
// mapRadius: every bundled eye uses 240, checked against all 23 upstream eyes at
// CG-S16, so no combination here can break the one real constraint (gaze state
// in EyeController is stored in mapRadius units and is NOT rescaled on a swap).
// Eyeball radius may differ freely between sets: EyeController reads
// definition->radius per frame (EyeController.h:233), it caches nothing.
// Note demon and doe expose ::left / ::right rather than ::eye.
std::array<EyeDefinition, 2> eyeDefinitions[] = {
    {{nordicBlue::eye, nordicBlue::eye}},
//  {{bigBlue::eye,    bigBlue::eye}},
//  {{cat::eye,        cat::eye}},
//  {{demon::left,     demon::right}},
//  {{doe::left,       doe::right}},
//  {{doomRed::eye,    doomRed::eye}},
//  {{dragon::eye,     dragon::eye}},
//  {{fish::eye,       fish::eye}},
//  {{hazel::eye,      hazel::eye}},
//  {{skull::eye,      skull::eye}},
};
constexpr size_t EYE_SET_COUNT{std::size(eyeDefinitions)};

// Both on SPI0 (MOSI=11, SCK=13), separate CS. mirror flags match IRIS
// left/right: eye 0 mirror=true (also gets the EyeController eyeIndex==0
// software X-flip), eye 1 mirror=false. The two eyes then track together.
//        CS  DC MOSI SCK RST  ROT MIRROR USE_FB ASYNC
GC9A01A_Config eyeInfo[] = {
    {10, 2, 11, 13,  3, 0, true,  true, false},  // eye 0 (primary)
    { 9, 8, 11, 13,  6, 0, false, true, false},  // eye 1 (second display)
};

// CG-S17: 20 MHz -> 10 MHz, to test the display jitter/flicker seen at CG-S16 on
// all-dupont breadboard wiring. Dupont jumpers are a poor transmission line at
// 20 MHz: unshielded, no ground return paired with SCK/MOSI, breadboard stray
// capacitance. This is the cheap discriminator, not a fix. If the flicker clears,
// the cause is signal integrity and the real fix is physical (shorter leads, a
// ground return alongside the clock, eventually solder), after which this can go
// back up. If the flicker is unchanged, look at the 3.3 V rail measured at the
// DISPLAY's own VCC pin under load (the CG-S5 failure mode), then at the frame
// rate. Cost of the halving: CG-S14 measured a full 115,200-byte frame as a
// ~21 FPS bus-time ceiling at 20 MHz, so this roughly halves that headroom.
// updateChangedAreasOnly(true) (GC9A01A_Display.cpp:22) is what keeps the normal
// case well under a full-frame push.
constexpr uint32_t SPI_SPEED{10'000'000};

EyeController<2, GC9A01A_Display> *eyes{};
GC9A01A_Display *displayMain{};
GC9A01A_Display *displaySecond{};

void initEyes(bool autoMove, bool autoBlink, bool autoPupils) {
  auto &defs = eyeDefinitions[0];
  displayMain   = new GC9A01A_Display(eyeInfo[0], SPI_SPEED);
  displaySecond = new GC9A01A_Display(eyeInfo[1], SPI_SPEED);
  const DisplayDefinition<GC9A01A_Display> main{displayMain, defs[0]};
  const DisplayDefinition<GC9A01A_Display> second{displaySecond, defs[1]};
  eyes = new EyeController<2, GC9A01A_Display>({main, second}, autoMove, autoBlink, autoPupils);
}

#else  // single eye (default)

// Eye sets, single-eye build (the default). One row per selectable look, one
// entry per row because there is one display. Outer index = set, in EYE:next and
// auto-rotation order. See the DUAL_EYE branch above for the mapRadius and
// per-frame-radius notes.
//
// To change the eye: comment this row out, uncomment another, and uncomment that
// eye's #include at the top of this file. The array size is deduced, so nothing
// else needs editing.
//
// Exactly one row uncommented means nothing rotates, which is the shipped
// default and what a face-tracking demo wants. Uncomment two or more to get
// switching, either on the EYE_ROTATE_MS_DEFAULT timer or by EYE:next over
// serial. Note demon and doe expose ::left / ::right rather than ::eye.
std::array<EyeDefinition, 1> eyeDefinitions[] = {
    {{nordicBlue::eye}},
//  {{bigBlue::eye}},
//  {{cat::eye}},
//  {{demon::left}},
//  {{doe::left}},
//  {{doomRed::eye}},
//  {{dragon::eye}},
//  {{fish::eye}},
//  {{hazel::eye}},
//  {{skull::eye}},
};
constexpr size_t EYE_SET_COUNT{std::size(eyeDefinitions)};

// CS=10 DC=2 MOSI=11 SCK=13 RST=3  rotation=0 mirror=true useFrameBuffer=true asyncUpdates=false
GC9A01A_Config eyeInfo[] = {
    {10, 2, 11, 13, 3, 0, true, true, false},
};

// CG-S17: 20 MHz -> 10 MHz, to test the display jitter/flicker seen at CG-S16 on
// all-dupont breadboard wiring. Dupont jumpers are a poor transmission line at
// 20 MHz: unshielded, no ground return paired with SCK/MOSI, breadboard stray
// capacitance. This is the cheap discriminator, not a fix. If the flicker clears,
// the cause is signal integrity and the real fix is physical (shorter leads, a
// ground return alongside the clock, eventually solder), after which this can go
// back up. If the flicker is unchanged, look at the 3.3 V rail measured at the
// DISPLAY's own VCC pin under load (the CG-S5 failure mode), then at the frame
// rate. Cost of the halving: CG-S14 measured a full 115,200-byte frame as a
// ~21 FPS bus-time ceiling at 20 MHz, so this roughly halves that headroom.
// updateChangedAreasOnly(true) (GC9A01A_Display.cpp:22) is what keeps the normal
// case well under a full-frame push.
constexpr uint32_t SPI_SPEED{10'000'000};

EyeController<1, GC9A01A_Display> *eyes{};
GC9A01A_Display *displayMain{};

void initEyes(bool autoMove, bool autoBlink, bool autoPupils) {
  auto &defs = eyeDefinitions[0];
  displayMain = new GC9A01A_Display(eyeInfo[0], SPI_SPEED);
  const DisplayDefinition<GC9A01A_Display> main{displayMain, defs[0]};
  eyes = new EyeController<1, GC9A01A_Display>({main}, autoMove, autoBlink, autoPupils);
}

#endif  // DUAL_EYE
