#pragma once

static constexpr char FIRMWARE_VERSION[] = "CG-S5";

#include "eyes/eyes.h"
#include "eyes/240x240/nordicBlue.h"
#include "eyes/EyeController.h"

#define USE_GC9A01A
#include "displays/GC9A01A_Display.h"

// ─────────────────────────────────────────────────────────────────────────────
// Tracking tunables (bench-adjustable — see NOTES.md bench protocol)
// ─────────────────────────────────────────────────────────────────────────────

// Confidence gate. A face must report box_confidence > PS_CONF_GATE to be
// tracked. CG-S5: raised from 45 to 152, derived from DFRobot's OWN documented
// threshold for this sensor family -- the SEN0626 wiki setup guide
// (wiki.dfrobot.com/sen0626/docs/23024) states "a score >=60 is considered
// valid" and its sample code calls gfd.setFaceDetectThres(60). The CG-S3
// default of 45 (box_confidence, matching IRIS's own unrelated psConfGate
// constant) mapped to a raw SEN0626 score of just ~19/100 -- roughly a third of
// the vendor's own validity floor, likely accepting noisy/marginal detections.
// 152 = floor(60*255/100) - 1, so with strict '>' a raw score of exactly 60
// passes and 59 does not. Lower only if bench data shows DFRobot's own
// threshold is too strict for this specific unit (audit 3.7).
static constexpr uint8_t PS_CONF_GATE = 152;

// Multiplies targetX/targetY before setTargetPosition. 1.0 = the IRIS-matched
// (production-tuned) gaze range. Raise (>1) to make the eye reach its travel
// limits with a face nearer frame-center; lower (<1) to damp the range. The
// controller clamps the result to the unit circle, so values >1 are safe
// (audit 3.8).
static constexpr float GAZE_GAIN = 1.0f;

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
// needs no define. See 05_WIRING.md "Dual-Eye (optional)" for the pin table.
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
