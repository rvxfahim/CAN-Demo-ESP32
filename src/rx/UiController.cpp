#include "UiController.h"
#include <Arduino.h>
#include <cstring>

UiController::UiController() 
  : lvglDisplay_(nullptr), degradedLabel_(nullptr), faultLabel_(nullptr)
{
}

bool UiController::Init()
{
  // Defer all LVGL/TFT/UI initialization to the UI task to ensure single-thread access.
  return true;
}

bool UiController::StartTask(uint16_t dataQueueLen, uint16_t msgQueueLen,
                             UBaseType_t priority, uint16_t stackWords)
{
  // Create queues
  if (uiDataQueue_ == nullptr)
  {
    uiDataQueue_ = xQueueCreate(dataQueueLen, sizeof(UiData));
  }
  if (uiMsgQueue_ == nullptr)
  {
    uiMsgQueue_ = xQueueCreate(msgQueueLen, sizeof(UiMessage));
  }

  if (uiDataQueue_ == nullptr || uiMsgQueue_ == nullptr)
  {
    return false;
  }

  // Spawn task pinned (core selection left to scheduler)
  if (uiTaskHandle_ == nullptr)
  {
    BaseType_t ok = xTaskCreate(
      UiTaskEntry_, "ui_task", stackWords, this, priority, &uiTaskHandle_);
    return ok == pdPASS;
  }

  return true;
}

bool UiController::EnqueueUiData(const UiData& data)
{
  if (uiDataQueue_ == nullptr) return false;
  // Use overwrite semantics if queue length == 1
  if (uxQueueSpacesAvailable(uiDataQueue_) == 0)
  {
    // queue full: overwrite last item
    return xQueueOverwrite(uiDataQueue_, &data) == pdTRUE;
  }
  return xQueueSend(uiDataQueue_, &data, 0) == pdTRUE;
}

bool UiController::EnqueueMessage(const UiMessage& msg)
{
  if (uiMsgQueue_ == nullptr) return false;
  return xQueueSend(uiMsgQueue_, &msg, 0) == pdTRUE;
}

void UiController::ApplyCluster(const Cluster_t& cluster)
{
  // Hide degraded warning when receiving valid data
  if (degradedLabel_ != nullptr)
  {
    lv_obj_add_flag(degradedLabel_, LV_OBJ_FLAG_HIDDEN);
  }

  // Update speed gauge
  if (ui_Arc1 != nullptr)
  {
    const uint16_t arcValue = ConvertSpeedToArcValue(cluster.speed);
    lv_arc_set_value(ui_Arc1, static_cast<int32_t>(arcValue));
  }

  // Update left turn indicator
  if (ui_LeftTurnLabel != nullptr)
  {
    const uint8_t leftAlpha = cluster.Left_Turn_Signal ? kOpacityOn : kOpacityOff;
    lv_obj_set_style_text_opa(ui_LeftTurnLabel, leftAlpha, LV_PART_MAIN | LV_STATE_DEFAULT);
  }

  // Update right turn indicator
  if (ui_RightTurnLabel != nullptr)
  {
    const uint8_t rightAlpha = cluster.Right_Turn_Signal ? kOpacityOn : kOpacityOff;
    lv_obj_set_style_text_opa(ui_RightTurnLabel, rightAlpha, LV_PART_MAIN | LV_STATE_DEFAULT);
  }
}

void UiController::ShowDegraded()
{
  if (degradedLabel_ != nullptr)
  {
    lv_obj_clear_flag(degradedLabel_, LV_OBJ_FLAG_HIDDEN);
  }
}

void UiController::ShowFault()
{
  if (faultLabel_ != nullptr)
  {
    lv_obj_clear_flag(faultLabel_, LV_OBJ_FLAG_HIDDEN);
  }
}

void UiController::ShowDashboardScreen()
{
  // Load the main dashboard screen (Screen1)
  if (ui_Screen1 != nullptr)
  {
    lv_disp_load_scr(ui_Screen1);
  }
}

void UiController::ShowLogScreen()
{
  // Load the log screen (Screen2) and ensure LogBox shows current lines
  if (ui_Screen2 != nullptr)
  {
    lv_disp_load_scr(ui_Screen2);
    UpdateLogBox_();
  }
}

void UiController::AddLogLine(const char* line)
{
  if (line == nullptr) return;
  AddLogLine(std::string(line));
}

void UiController::AddLogLine(const std::string& line)
{
  if (line.empty()) return;
  // Maintain ring buffer up to kMaxLogLines_
  if (logLines_.size() >= kMaxLogLines_)
  {
    logLines_.pop_front();
  }
  logLines_.push_back(line);
  UpdateLogBox_();
}

void UiController::UpdateLogBox_()
{
  if (ui_LogBox == nullptr) return;

  // Concatenate lines with newlines
  std::string text;
  text.reserve(256);
  for (size_t i = 0; i < logLines_.size(); ++i)
  {
    text += logLines_[i];
    if (i + 1 < logLines_.size()) text += '\n';
  }
  lv_textarea_set_text(ui_LogBox, text.c_str());
}

void UiController::ServiceTimers()
{
  lv_timer_handler();
}

void UiController::UiTaskEntry_(void* pv)
{
  UiController* self = static_cast<UiController*>(pv);
  if (self)
  {
    self->UiTaskLoop_();
  }
  vTaskDelete(nullptr);
}

void UiController::UiTaskLoop_()
{
  // Initialize LVGL, TFT, Touch, and UI in this task context
  lv_init();
  lv_tick_set_cb(LvglTickGetCb);

  tft.begin();
  tft.setRotation(3);

  // Calibrate touchscreen
  TS.Calibrate(372, 3695, 501, 3838);

  // Create LVGL display
  lvglDisplay_ = lv_display_create(screenWidth, screenHeight);
  if (lvglDisplay_ != nullptr)
  {
    lv_display_set_default(lvglDisplay_);
    lv_display_set_flush_cb(lvglDisplay_, DisplayFlushCb);
    lv_display_set_color_format(lvglDisplay_, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(lvglDisplay_, buf, nullptr,
                           lvglBufferSizePixels * sizeof(lv_color_t),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);
  }

  // Create LVGL input device (touchscreen)
  lv_indev_t* indev = lv_indev_create();
  if (indev != nullptr)
  {
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, TouchpadReadCb);
    lv_indev_set_display(indev, lvglDisplay_);
  }

  // Initialize generated UI
  ui_init();

  // Initialize UI widgets defaults
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

  // Create overlays
  degradedLabel_ = lv_label_create(lv_screen_active());
  if (degradedLabel_ != nullptr)
  {
    lv_label_set_text(degradedLabel_, "STALE DATA");
    lv_obj_set_style_text_color(degradedLabel_, lv_color_hex(0xFFFF00), LV_PART_MAIN);
    lv_obj_set_style_text_font(degradedLabel_, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_align(degradedLabel_, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_add_flag(degradedLabel_, LV_OBJ_FLAG_HIDDEN);
  }
  faultLabel_ = lv_label_create(lv_screen_active());
  if (faultLabel_ != nullptr)
  {
    lv_label_set_text(faultLabel_, "SYSTEM FAULT");
    lv_obj_set_style_text_color(faultLabel_, lv_color_hex(0xFF0000), LV_PART_MAIN);
    lv_obj_set_style_text_font(faultLabel_, &lv_font_montserrat_28, LV_PART_MAIN);
    lv_obj_align(faultLabel_, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(faultLabel_, LV_OBJ_FLAG_HIDDEN);
  }

  UiData latest{0, false, false};
  const TickType_t tick5ms = pdMS_TO_TICKS(5);

  for (;;)
  {
    // Process any pending UI commands quickly
    UiMessage msg;
    while (uiMsgQueue_ && xQueueReceive(uiMsgQueue_, &msg, 0) == pdTRUE)
    {
      HandleUiMessage_(msg);
    }

    // Wait briefly for new data to refresh display
    if (uiDataQueue_)
    {
      UiData incoming;
      if (xQueueReceive(uiDataQueue_, &incoming, tick5ms) == pdTRUE)
      {
        latest = incoming; // overwrite latest sample
        ApplyUiData_(latest);
      }
    }

    // Run blink animation and LVGL timers
    UpdateBlink_(latest);
    lv_timer_handler();
  }
}

void UiController::HandleUiMessage_(const UiMessage& msg)
{
  switch (msg.type)
  {
    case UiMessageType::ShowDashboard:
      ShowDashboardScreen();
      break;
    case UiMessageType::ShowLog:
      ShowLogScreen();
      break;
    case UiMessageType::AddLogLine:
      AddLogLine(msg.text);
      break;
    case UiMessageType::ShowDegraded:
      ShowDegraded();
      break;
    case UiMessageType::ShowFault:
      ShowFault();
      break;
    default:
      break;
  }
}

void UiController::ApplyUiData_(const UiData& data)
{
  // Update speed
  if (ui_Arc1 != nullptr)
  {
    lv_arc_set_value(ui_Arc1, static_cast<int32_t>(data.speedArc));
  }

  // If not active, ensure indicators are off immediately
  if (!data.leftActive && ui_LeftTurnLabel != nullptr)
  {
    lv_obj_set_style_text_opa(ui_LeftTurnLabel, kOpacityOff, LV_PART_MAIN | LV_STATE_DEFAULT);
  }
  if (!data.rightActive && ui_RightTurnLabel != nullptr)
  {
    lv_obj_set_style_text_opa(ui_RightTurnLabel, kOpacityOff, LV_PART_MAIN | LV_STATE_DEFAULT);
  }
}

void UiController::UpdateBlink_(const UiData& data)
{
  // Use a shared time-based phase so both indicators stay perfectly in sync
  // when active at the same time, regardless of individual toggle history.
  const uint32_t now = millis();
  const bool phaseOn = ((now / kBlinkPeriodMs_) % 2U) == 0U;

  if (ui_LeftTurnLabel != nullptr)
  {
    const uint8_t alpha = (data.leftActive && phaseOn) ? kOpacityOn : kOpacityOff;
    lv_obj_set_style_text_opa(ui_LeftTurnLabel, alpha, LV_PART_MAIN | LV_STATE_DEFAULT);
  }

  if (ui_RightTurnLabel != nullptr)
  {
    const uint8_t alpha = (data.rightActive && phaseOn) ? kOpacityOn : kOpacityOff;
    lv_obj_set_style_text_opa(ui_RightTurnLabel, alpha, LV_PART_MAIN | LV_STATE_DEFAULT);
  }
}

uint16_t UiController::ConvertSpeedToArcValue(uint16_t rawSpeed)
{
  const uint16_t clamped = (rawSpeed > kSpeedRawMax) ? kSpeedRawMax : rawSpeed;
  return static_cast<uint16_t>((static_cast<uint32_t>(clamped) * kArcMaxValue) / kSpeedRawMax);
}

uint32_t UiController::LvglTickGetCb()
{
  return static_cast<uint32_t>(millis());
}

void UiController::DisplayFlushCb(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map)
{
  const uint32_t w = static_cast<uint32_t>(lv_area_get_width(area));
  const uint32_t h = static_cast<uint32_t>(lv_area_get_height(area));

  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushColors(reinterpret_cast<uint16_t*>(px_map), w * h, true);
  tft.endWrite();

  lv_display_flush_ready(disp);
}

void UiController::TouchpadReadCb(lv_indev_t* indev, lv_indev_data_t* data)
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
