/**
 * @file TFTConfiguration.h
 * @brief LVGL + TFT_eSPI display and touch global objects and dimensions.
 */
#ifndef TFTCONFIGURATION_H
#define TFTCONFIGURATION_H

#include <cstddef>
#include <TFT_eSPI.h>
#include <lvgl.h>
#include <NS2009.h>

extern const uint16_t screenWidth;
extern const uint16_t screenHeight;
extern const size_t lvglBufferSizePixels;
extern lv_color_t buf[];
extern TFT_eSPI tft;
extern NS2009 TS;

#endif // TFTCONFIGURATION_H
