#ifndef IO_MODULE_H
#define IO_MODULE_H

#include <cstdint>
#include <Arduino.h>
#include "lecture.h"
#include "common/MessageRouter.h"
#include "IOPins.h"

// Simple IO module controlling two relay outputs for left/right blinkers.
// Subscribes to typed Cluster messages from MessageRouter.

class IOModule
{
public:
  IOModule();

  bool Init(uint8_t leftPin = IO_LEFT_RELAY_PIN, uint8_t rightPin = IO_RIGHT_RELAY_PIN, bool activeHigh = true);

  // Register subscription; call after router.Init()
  bool Start(MessageRouter& router);
  void Stop(MessageRouter& router);

  // Call periodically from loop for blink timing
  void Update(uint32_t nowMs);

private:
  static void ClusterCb_(const Cluster_t& msg, uint32_t tsMs, void* ctx);
  void OnCluster_(const Cluster_t& msg, uint32_t tsMs);
  void ApplyOutputs_();

  uint8_t leftPin_ = IO_LEFT_RELAY_PIN;
  uint8_t rightPin_ = IO_RIGHT_RELAY_PIN;
  bool activeHigh_ = true;

  // State from latest CAN
  volatile bool leftRequested_ = false;
  volatile bool rightRequested_ = false;
  volatile uint32_t lastUpdateMs_ = 0;

  // Blink state
    // Blink period: 1 Hz (500ms ON, 500ms OFF)
    static constexpr uint32_t kBlinkPeriodMs_ = 1000;
  uint32_t nextToggleMs_ = 0;
  bool leftOn_ = false;
  bool rightOn_ = false;
};

#endif // IO_MODULE_H
