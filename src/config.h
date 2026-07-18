#pragma once

static constexpr char FIRMWARE_VERSION[] = "CG-S12";

#include "eyes/eyes.h"
#include "eyes/240x240/nordicBlue.h"
#include "eyes/EyeController.h"

#define USE_GC9A01A
#include "displays/GC9A01A_Display.h"

// ─────────────────────────────────────────────────────────────────────────────
// Tracking tunables (bench-adjustable — see docs/BENCH_PROTOCOL.md)
// ─────────────────────────────────────────────────────────────────────────────

// Confidence gate. A face must report box_confidence > PS_CONF_GATE to be
// tracked. CG-S12 (⚠ UNVERIFIED, re-bench #1 priority): moved 152 -> 60 because
// SEN0626Sensor now emits the RAW DFRobot score (0-100), not the old 0-255 remap
// (see SEN0626Sensor.cpp CG-S12 note). 60 IS DFRobot's own documented validity
// floor -- the SEN0626 wiki setup guide (wiki.dfrobot.com/sen0626/docs/23024)
// states "a score >=60 is considered valid" and its sample code calls
// gfd.setFaceDetectThres(60). With strict '>' a raw score of 60 passes and 59
// does not. This is the SAME effective threshold as the old bench-VERIFIED
// 152/255 (≈0.60) -- only the scale changed -- but it has NOT been re-observed on
// the bench since the change. Lower only if bench data shows the vendor floor is
// too strict for this specific unit (audit 3.7).
static constexpr uint8_t PS_CONF_GATE = 60;

// ── Per-axis signed gain + bias (CG-S12, synced from IRIS S212c) ─────────────
// Gaze shaping model: targetN = rawN * GAIN + BIAS, where rawN = (cN/127.5)-1 is
// the sensor-space target in [-1,+1] (cN = box center on the shim's 0-255 scale).
// The SIGN of the gain sets direction and the MAGNITUDE sets range, so one knob
// covers both "it's mirrored" and "it barely moves". setTargetPosition clamps the
// result to the unit circle, so |gain|>1 saturates gracefully (audit 3.8).
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
static constexpr float GAZE_X_GAIN = 1.7f;
static constexpr float GAZE_Y_GAIN = 1.7f;
static constexpr float GAZE_X_BIAS = 0.0f;
static constexpr float GAZE_Y_BIAS = 1.26f;

// Time with no qualifying face before autoMove (idle wander) resumes.
static constexpr unsigned long FACE_LOST_MS = 3000;

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

std::array<std::array<EyeDefinition, 2>, 1> eyeDefinitions{{
    {nordicBlue::eye, nordicBlue::eye},
}};

// Both on SPI0 (MOSI=11, SCK=13), separate CS. mirror flags match IRIS
// left/right: eye 0 mirror=true (also gets the EyeController eyeIndex==0
// software X-flip), eye 1 mirror=false. The two eyes then track together.
//        CS  DC MOSI SCK RST  ROT MIRROR USE_FB ASYNC
GC9A01A_Config eyeInfo[] = {
    {10, 2, 11, 13,  3, 0, true,  true, false},  // eye 0 (primary)
    { 9, 8, 11, 13,  6, 0, false, true, false},  // eye 1 (second display)
};

constexpr uint32_t SPI_SPEED{20'000'000};

EyeController<2, GC9A01A_Display> *eyes{};
GC9A01A_Display *displayMain{};
GC9A01A_Display *displaySecond{};

void initEyes(bool autoMove, bool autoBlink, bool autoPupils) {
  auto &defs = eyeDefinitions.at(0);
  displayMain   = new GC9A01A_Display(eyeInfo[0], SPI_SPEED);
  displaySecond = new GC9A01A_Display(eyeInfo[1], SPI_SPEED);
  const DisplayDefinition<GC9A01A_Display> main{displayMain, defs[0]};
  const DisplayDefinition<GC9A01A_Display> second{displaySecond, defs[1]};
  eyes = new EyeController<2, GC9A01A_Display>({main, second}, autoMove, autoBlink, autoPupils);
}

#else  // single eye (default)

std::array<std::array<EyeDefinition, 1>, 1> eyeDefinitions{{
    {nordicBlue::eye},
}};

// CS=10 DC=2 MOSI=11 SCK=13 RST=3  rotation=0 mirror=true useFrameBuffer=true asyncUpdates=false
GC9A01A_Config eyeInfo[] = {
    {10, 2, 11, 13, 3, 0, true, true, false},
};

constexpr uint32_t SPI_SPEED{20'000'000};

EyeController<1, GC9A01A_Display> *eyes{};
GC9A01A_Display *displayMain{};

void initEyes(bool autoMove, bool autoBlink, bool autoPupils) {
  auto &defs = eyeDefinitions.at(0);
  displayMain = new GC9A01A_Display(eyeInfo[0], SPI_SPEED);
  const DisplayDefinition<GC9A01A_Display> main{displayMain, defs[0]};
  eyes = new EyeController<1, GC9A01A_Display>({main}, autoMove, autoBlink, autoPupils);
}

#endif  // DUAL_EYE
