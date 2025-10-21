// TFTConfiguration.cpp
#include "TFTConfiguration.h"
#include <lvgl.h>
const uint16_t screenWidth = 320;
const uint16_t screenHeight = 240;
const size_t lvglBufferSizePixels = static_cast<size_t>(screenWidth) * static_cast<size_t>(screenHeight) / 10U;
lv_color_t buf[lvglBufferSizePixels];
TFT_eSPI tft = TFT_eSPI(screenWidth, screenHeight);
NS2009 TS(false, false);