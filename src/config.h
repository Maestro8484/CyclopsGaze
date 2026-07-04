#pragma once

static constexpr char FIRMWARE_VERSION[] = "CG-S2";

#include "eyes/eyes.h"
#include "eyes/240x240/nordicBlue.h"
#include "eyes/EyeController.h"

#define USE_GC9A01A
#include "displays/GC9A01A_Display.h"

// Single eye
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
