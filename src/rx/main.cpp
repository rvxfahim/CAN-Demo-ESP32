#include <Arduino.h>
#include <Wire.h>
#include <esp32_can.h>
#include <lvgl.h>
#include "lecture.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "ui.h"
#include "TFTConfiguration.h"

namespace
{
lv_display_t *lvglDisplay = nullptr;
QueueHandle_t clusterQueue = nullptr;

constexpr uint16_t kArcMaxValue = 240U;
constexpr uint16_t kSpeedRawMax = 4095U;
constexpr uint8_t kOpacityOn = 255U;
constexpr uint8_t kOpacityOff = 0U;
}

static uint16_t ConvertSpeedToArcValue(uint16_t rawSpeed)
{
  const uint16_t clamped = (rawSpeed > kSpeedRawMax) ? kSpeedRawMax : rawSpeed;
  return static_cast<uint16_t>((static_cast<uint32_t>(clamped) * kArcMaxValue) / kSpeedRawMax);
}

static void UpdateClusterUi(const Cluster_t &cluster)
{
  if (ui_Arc1 != nullptr)
  {
    const uint16_t arcValue = ConvertSpeedToArcValue(cluster.speed);
    lv_arc_set_value(ui_Arc1, static_cast<int32_t>(arcValue));
  }

  if (ui_LeftTurnLabel != nullptr)
  {
    const uint8_t leftAlpha = cluster.Left_Turn_Signal ? kOpacityOn : kOpacityOff;
    lv_obj_set_style_text_opa(ui_LeftTurnLabel, leftAlpha, LV_PART_MAIN | LV_STATE_DEFAULT);
  }

  if (ui_RightTurnLabel != nullptr)
  {
    const uint8_t rightAlpha = cluster.Right_Turn_Signal ? kOpacityOn : kOpacityOff;
    lv_obj_set_style_text_opa(ui_RightTurnLabel, rightAlpha, LV_PART_MAIN | LV_STATE_DEFAULT);
  }
}

static uint32_t lvgl_tick_get_cb()
{
  return static_cast<uint32_t>(millis());
}

void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
  const uint32_t w = static_cast<uint32_t>(lv_area_get_width(area));
  const uint32_t h = static_cast<uint32_t>(lv_area_get_height(area));

  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushColors(reinterpret_cast<uint16_t *>(px_map), w * h, true);
  tft.endWrite();

  lv_display_flush_ready(disp);
}

void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data)
{
  const bool touched = TS.CheckTouched();

  if (!touched)
  {
    data->state = LV_INDEV_STATE_RELEASED;
    return;
  }

  TS.Scan();
  data->state = LV_INDEV_STATE_PRESSED;
  data->point.x = static_cast<int16_t>(TS.Y);
  data->point.y = static_cast<int16_t>(TS.X);
}

void CanMsgHandler(CAN_FRAME *frame)
{
  if (frame == nullptr)
  {
    return;
  }

  if ((frame->id != Cluster_CANID) || (frame->length < Cluster_DLC) || (frame->extended != Cluster_IDE))
  {
    return;
  }

  Cluster_t cluster{};
  Unpack_Cluster_lecture(&cluster, frame->data.bytes, frame->length);

  if (clusterQueue != nullptr)
  {
    xQueueOverwrite(clusterQueue, &cluster);
  }
}
void setup()
{
  Wire.begin();
  clusterQueue = xQueueCreate(1, sizeof(Cluster_t));

  CAN0.setCANPins(GPIO_NUM_35, GPIO_NUM_5);
  CAN0.begin(500000);
  const int clusterMailbox = CAN0.watchFor(Cluster_CANID);
  if (clusterMailbox >= 0)
  {
    CAN0.setCallback(static_cast<uint8_t>(clusterMailbox), CanMsgHandler);
  }
  else
  {
    CAN0.setGeneralCallback(CanMsgHandler);
  }

  lv_init();
  lv_tick_set_cb(lvgl_tick_get_cb);

  tft.begin();
  tft.setRotation(3);
  TS.Calibrate(372, 3695, 501, 3838);

  lvglDisplay = lv_display_create(screenWidth, screenHeight);
  lv_display_set_default(lvglDisplay);
  lv_display_set_flush_cb(lvglDisplay, my_disp_flush);
  lv_display_set_color_format(lvglDisplay, LV_COLOR_FORMAT_RGB565);
  lv_display_set_buffers(lvglDisplay, buf, nullptr, lvglBufferSizePixels * sizeof(lv_color_t),
                         LV_DISPLAY_RENDER_MODE_PARTIAL);

  lv_indev_t *indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, my_touchpad_read);
  lv_indev_set_display(indev, lvglDisplay);

  ui_init();

  if (ui_LeftTurnLabel != nullptr)
  {
    lv_obj_set_style_text_opa(ui_LeftTurnLabel, kOpacityOff, LV_PART_MAIN | LV_STATE_DEFAULT);
  }

  if (ui_RightTurnLabel != nullptr)
  {
    lv_obj_set_style_text_opa(ui_RightTurnLabel, kOpacityOff, LV_PART_MAIN | LV_STATE_DEFAULT);
  }

  if (ui_Arc1 != nullptr)
  {
    lv_arc_set_value(ui_Arc1, 0);
  }
}

void loop()
{
  if (clusterQueue != nullptr)
  {
    Cluster_t latestCluster{};
    while (xQueueReceive(clusterQueue, &latestCluster, 0) == pdTRUE)
    {
      UpdateClusterUi(latestCluster);
    }
  }

  lv_timer_handler();
  delay(5);
}