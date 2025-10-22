#include <Arduino.h>
#include <esp32_can.h>
#include <cstring>
#include "lecture.h"
#include "driver/twai.h"

namespace
{
constexpr uint32_t kBusSpeed = 500000U;
constexpr uint32_t kSendPeriodMs = 100U;
constexpr uint16_t kMaxSpeed = 4095U;
constexpr uint16_t kSpeedStep = 64U;
constexpr uint32_t kSerialBaudRate = 115200U;
constexpr uint32_t kStatusCheckPeriodMs = 1000U;  // Check every second
constexpr uint32_t kPattern1DurationMs = 10000U;    // Both indicators ON duration
}

enum class TestPattern : uint8_t
{
  BothIndicatorsOn = 1,          // both indicators ON, speed 0
  SpeedSwingNoIndicators = 2,    // swing speed 0..max..0..0, indicators OFF
  SpeedSwingLeftIndicator = 3,   // swing speed with LEFT indicator ON
  SpeedSwingRightIndicator = 4   // swing speed with RIGHT indicator ON
};

// TX pattern state
static TestPattern g_pattern = TestPattern::BothIndicatorsOn;
static uint16_t g_speed = 0U;
static bool g_left = true;
static bool g_right = true;
static bool g_speedIncreasing = true;  // for swing pattern
static uint32_t g_patternStartMs = 0U;  // timestamp when current pattern started

// Returns true when a full swing completes back to zero
static bool UpdateSpeedSwing()
{
  if (g_speedIncreasing)
  {
    if (g_speed >= (kMaxSpeed - kSpeedStep))
    {
      g_speed = static_cast<uint16_t>(g_speed + kSpeedStep);
      if (g_speed > kMaxSpeed) g_speed = kMaxSpeed;
      g_speedIncreasing = false;
    }
    else
    {
      g_speed = static_cast<uint16_t>(g_speed + kSpeedStep);
    }
  }
  else
  {
    if (g_speed <= kSpeedStep)
    {
      g_speed = 0U;
      g_speedIncreasing = true;
      return true;  // finished a full swing
    }
    else
    {
      g_speed = static_cast<uint16_t>(g_speed - kSpeedStep);
    }
  }
  return false;
}

static void CheckCANStatus()
{
  twai_status_info_t status_info;
  
  if (twai_get_status_info(&status_info) == ESP_OK)
  {
    const char* stateStr = "UNKNOWN";
    switch (status_info.state)
    {
      case TWAI_STATE_RUNNING:    stateStr = "RUNNING"; break;
      case TWAI_STATE_BUS_OFF:    stateStr = "BUS-OFF"; break;
      case TWAI_STATE_RECOVERING: stateStr = "RECOVERING"; break;
      default: break;
    }
    
    Serial.printf("[CAN Status] State: %s, TX Errors: %u, RX Errors: %u, Msgs in TX Queue: %u\n",
                  stateStr,
                  status_info.tx_error_counter,
                  status_info.rx_error_counter,
                  status_info.msgs_to_tx);
    
    // Warn about different states
    if (status_info.state == TWAI_STATE_BUS_OFF)
    {
      Serial.println("[CAN WARNING] BUS-OFF - No ACK from RX! Watchdog will auto-recover.");
    }
    else if (status_info.state == TWAI_STATE_RECOVERING)
    {
      Serial.println("[CAN INFO] Recovering from Bus-Off state...");
    }
    else if (status_info.tx_error_counter >= 128)
    {
      Serial.println("[CAN WARNING] Error Passive - High error rate, check RX node!");
    }
    else if (status_info.msgs_to_tx >= 12)  // More than 75% full
    {
      Serial.printf("[CAN WARNING] TX Queue nearly full (%u/16)!\n", status_info.msgs_to_tx);
    }
  }
}

static void SendClusterFrame(uint16_t speed, bool leftTurn, bool rightTurn)
{
  Cluster_t cluster{};
  cluster.speed = speed;
  cluster.Left_Turn_Signal = static_cast<uint8_t>(leftTurn);
  cluster.Right_Turn_Signal = static_cast<uint8_t>(rightTurn);

  uint8_t payload[Cluster_DLC] = {0U};
  uint8_t dlc = 0U;
  uint8_t ide = 0U;
  Pack_Cluster_lecture(&cluster, payload, &dlc, &ide);

  CAN_FRAME frame{};
  frame.id = Cluster_CANID;
  frame.extended = ide;
  frame.length = dlc;
  frame.rtr = 0U;
  memcpy(frame.data.bytes, payload, dlc);

  const bool sent = CAN0.sendFrame(frame);

  if (Serial)
  {
    Serial.printf("[CAN] TX Cluster: speed=%u left=%u right=%u dlc=%u result=%s\n",
                  static_cast<unsigned>(cluster.speed),
                  static_cast<unsigned>(cluster.Left_Turn_Signal),
                  static_cast<unsigned>(cluster.Right_Turn_Signal),
                  static_cast<unsigned>(frame.length),
                  sent ? "ok" : "fail");
  }
}

void setup()
{
  Serial.begin(kSerialBaudRate);
  delay(10);
  CAN0.setCANPins(GPIO_NUM_35, GPIO_NUM_5);
  CAN0.setDebuggingMode(true);  // Enable alerts and debug output
  CAN0.begin(kBusSpeed);

  if (Serial)
  {
    Serial.printf("[CAN] Initialized at %lu bps\n", static_cast<unsigned long>(kBusSpeed));
    Serial.println("[TX] Auto-cycling patterns: (1) Both indicators ON -> (2) Speed swing (no indicators)");
  }
}

void loop()
{
  static uint32_t lastSendTick = 0U;
  static uint32_t lastStatusCheck = 0U;
  const uint32_t now = millis();
  
  // Periodic CAN status check
  if ((now - lastStatusCheck) >= kStatusCheckPeriodMs)
  {
    lastStatusCheck = now;
    CheckCANStatus();
  }
  
  if ((now - lastSendTick) < kSendPeriodMs)
  {
    delay(1);
    return;
  }

  lastSendTick = now;
  
  // Initialize pattern start time once
  if (g_patternStartMs == 0U) {
    g_patternStartMs = now;
  }

  // Update state based on selected pattern
  switch (g_pattern)
  {
    case TestPattern::BothIndicatorsOn:
      g_speed = 0U;
      g_left = true;
      g_right = true;
      // Advance to next pattern after fixed duration
      if ((now - g_patternStartMs) >= kPattern1DurationMs)
      {
        g_pattern = TestPattern::SpeedSwingNoIndicators;
        g_speed = 0U;
        g_speedIncreasing = true;
        g_left = false;
        g_right = false;
        g_patternStartMs = now;
        if (Serial) Serial.println("[TX] -> Pattern 2: Speed swing (no indicators)");
      }
      break;

    case TestPattern::SpeedSwingNoIndicators:
      g_left = false;
      g_right = false;
      if (UpdateSpeedSwing())
      {
        // Completed a full swing back to zero -> go to next pattern (Left indicator swing)
        g_pattern = TestPattern::SpeedSwingLeftIndicator;
        if (Serial) Serial.println("[TX] -> Pattern 3: Speed swing (LEFT indicator)");
      }
      break;

    case TestPattern::SpeedSwingLeftIndicator:
      g_left = true;
      g_right = false;
      if (UpdateSpeedSwing())
      {
        // After finishing left-indicator swing, move to right-indicator swing
        g_pattern = TestPattern::SpeedSwingRightIndicator;
        if (Serial) Serial.println("[TX] -> Pattern 4: Speed swing (RIGHT indicator)");
      }
      break;

    case TestPattern::SpeedSwingRightIndicator:
      g_left = false;
      g_right = true;
      if (UpdateSpeedSwing())
      {
        // After finishing right-indicator swing, loop back to pattern 1
        g_pattern = TestPattern::BothIndicatorsOn;
        g_patternStartMs = now;
        if (Serial) Serial.println("[TX] -> Pattern 1: Both indicators ON");
      }
      break;
    default:
      break;
  }

  SendClusterFrame(g_speed, g_left, g_right);
}
