#ifndef HEALTH_MONITOR_H
#define HEALTH_MONITOR_H

#include "freertos/FreeRTOS.h"
#include "EventQueue.h"

class HealthMonitor
{
public:
  HealthMonitor();

  void NotifyFrame();
  bool CheckTimeout(EventQueue& eventQueue);
  void Reset();
  void SetTimeoutMs(uint32_t timeoutMs);

private:
  TickType_t lastFrameTick_;
  uint32_t timeoutMs_ = 1500; // Increased default timeout to reduce flicker to Degraded/Waiting
};

#endif // HEALTH_MONITOR_H
