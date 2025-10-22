#include <Arduino.h>
#include <esp32_can.h>
#include "lecture.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "ui.h"
#include <lvgl.h>
#include <TFT_eSPI.h>
#include "TFTConfiguration.h"

namespace
{
  lv_display_t *lvglDisplay = nullptr;
  QueueHandle_t clusterQueue = nullptr;

  constexpr uint16_t kArcMaxValue = 240U;
  constexpr uint16_t kSpeedRawMax = 4095U;
  constexpr uint8_t kOpacityOn = 255U;
  constexpr uint8_t kOpacityOff = 0U;

  TaskHandle_t canProcessTaskHandle = nullptr;
  
  constexpr uint32_t DRAW_BUF_SIZE = (320U * 240U / 10U * (LV_COLOR_DEPTH / 8U));
  uint32_t draw_buf[DRAW_BUF_SIZE / 4];
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

// If logging is enabled, it will inform the user about what is happening in the library
void log_print(lv_log_level_t level, const char * buf) {
  LV_UNUSED(level);
  Serial.println(buf);
  Serial.flush();
}

// Get the Touchscreen data
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

void CanProcessTask(void *parameter)
{
  while (true)
  {
    if (clusterQueue != nullptr)
    {
      Cluster_t latestCluster{};
      while (xQueueReceive(clusterQueue, &latestCluster, 0) == pdTRUE)
      {
        UpdateClusterUi(latestCluster);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(2));
  }
}

void setup()
{
  Serial.begin(115200);
  String LVGL_Arduino = String("LVGL Library Version: ") + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();
  Serial.println(LVGL_Arduino);
  
  Wire.begin();
  delay(100);

  // Initialize CAN
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

  // Start LVGL
  lv_init();
  // Register print function for debugging (if logging is enabled)
  #if LV_USE_LOG
  lv_log_register_print_cb(log_print);
  #endif

  // Initialize the TFT display using the TFT_eSPI library
  lvglDisplay = lv_tft_espi_create(screenWidth, screenHeight, draw_buf, sizeof(draw_buf));
  lv_display_set_rotation(lvglDisplay, LV_DISPLAY_ROTATION_90);

  // Initialize touchscreen
  TS.Calibrate(372, 3695, 501, 3838);

  // Initialize an LVGL input device object (Touchscreen)
  lv_indev_t *indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, my_touchpad_read);
  lv_indev_set_display(indev, lvglDisplay);

  Serial.println("LVGL initialized.");

  // Initialize UI
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

  Serial.println("Setup done, starting CAN processing task...");
  
  // Create CAN processing task
  xTaskCreatePinnedToCore(
      CanProcessTask,
      "CanProcess",
      4096,
      nullptr,
      1,
      &canProcessTaskHandle,
      1);
}

void loop()
{
  lv_task_handler();  // let the GUI do its work
  lv_tick_inc(5);     // tell LVGL how much time has passed
  delay(5);           // let this time pass
}