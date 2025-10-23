/**
 * @file UiController.h
 * @brief LVGL-based UI controller with optional dedicated RTOS task and queues.
 *
 * Provides a small message bus for UI commands and an overwrite-queue for current
 * instrument cluster data. Exposes helpers to switch screens and log messages.
 */
#ifndef UI_CONTROLLER_H
#define UI_CONTROLLER_H

#include <lvgl.h>
#include "lecture.h"
#include "TFTConfiguration.h"
#include "ui.h"
#include <deque>
#include <string>
#include <cstring>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

// Lightweight data payload for UI updates (overwrite-queue semantics)
/**
 * @struct UiData
 * @brief Current dashboard state mapped for LVGL widgets.
 */
struct UiData
{
  uint16_t speedArc;   // 0..240 mapped arc value
  bool leftActive;     // left turn signal active
  bool rightActive;    // right turn signal active
};

/** UI message command types. */
enum class UiMessageType : uint8_t
{
  ShowDashboard,
  ShowLog,
  AddLogLine,
  ShowDegraded,
  ShowFault
};

/**
 * @struct UiMessage
 * @brief UI command with optional text payload (for logs).
 */
struct UiMessage
{
  UiMessageType type;
  char text[96]; // for AddLogLine

  static UiMessage MakeShowDashboard() { return UiMessage{UiMessageType::ShowDashboard, {0}}; }
  static UiMessage MakeShowLog() { return UiMessage{UiMessageType::ShowLog, {0}}; }
  static UiMessage MakeShowDegraded() { return UiMessage{UiMessageType::ShowDegraded, {0}}; }
  static UiMessage MakeShowFault() { return UiMessage{UiMessageType::ShowFault, {0}}; }
  static UiMessage MakeAddLog(const char* s)
  {
    UiMessage m{UiMessageType::AddLogLine, {0}};
    if (s)
    {
      strncpy(m.text, s, sizeof(m.text) - 1);
      m.text[sizeof(m.text) - 1] = '\0';
    }
    return m;
  }
};

/**
 * @class UiController
 * @brief Initializes LVGL, manages screens, and runs an optional UI task processing queues.
 */
class UiController
{
public:
  UiController();

  bool Init();
  // Start dedicated UI task with queues; call after Init()
  bool StartTask(uint16_t dataQueueLen = 1, uint16_t msgQueueLen = 10,
                 UBaseType_t priority = 2, uint16_t stackWords = 20*1024);

  // Queue-based API (thread-safe):
  bool EnqueueUiData(const UiData& data); // overwrite latest
  bool EnqueueMessage(const UiMessage& msg); // send command

  // Legacy direct UI APIs (only safe before StartTask):
  void ApplyCluster(const Cluster_t& cluster);
  void ShowDegraded();
  void ShowFault();
  void ServiceTimers();

  // Screen control helpers
  void ShowDashboardScreen();  // Loads Screen1 (dashboard)
  void ShowLogScreen();        // Loads Screen2 (LogBox)

  // Logging API for Screen2
  void AddLogLine(const char* line);
  void AddLogLine(const std::string& line);

  // Mapping helper for speed to arc value
  static uint16_t ConvertSpeedToArcValue(uint16_t rawSpeed);

private:
  static uint32_t LvglTickGetCb();
  static void DisplayFlushCb(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map);
  static void TouchpadReadCb(lv_indev_t* indev, lv_indev_data_t* data);

  lv_display_t* lvglDisplay_;
  lv_obj_t* degradedLabel_;
  lv_obj_t* faultLabel_;

  // RTOS UI task + queues
  QueueHandle_t uiDataQueue_ = nullptr; // length 1, uses overwrite
  QueueHandle_t uiMsgQueue_ = nullptr;  // length N
  TaskHandle_t uiTaskHandle_ = nullptr;
  static void UiTaskEntry_(void* pv);
  void UiTaskLoop_();
  void HandleUiMessage_(const UiMessage& msg);
  void ApplyUiData_(const UiData& data);
  void UpdateBlink_(const UiData& data);

  // Buffer for last N log lines displayed in Screen2 LogBox
  static constexpr size_t kMaxLogLines_ = 10;
  std::deque<std::string> logLines_;
  void UpdateLogBox_();

  static constexpr uint16_t kArcMaxValue = 240U;
  static constexpr uint16_t kSpeedRawMax = 4095U;
  static constexpr uint8_t kOpacityOn = 255U;
  static constexpr uint8_t kOpacityOff = 0U;

  // Blink control
  static constexpr uint32_t kBlinkPeriodMs_ = 500;
  uint32_t lastBlinkTickMs_ = 0;
  bool leftBlinkOn_ = false;
  bool rightBlinkOn_ = false;
};

#endif // UI_CONTROLLER_H
