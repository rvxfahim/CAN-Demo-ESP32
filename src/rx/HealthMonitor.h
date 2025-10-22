#ifndef HEALTH_MONITOR_H
#define HEALTH_MONITOR_H

#include "freertos/FreeRTOS.h"
#include "EventQueue.h"

class MessageRouter; // forward declare for pull model

class HealthMonitor
{
public:
  HealthMonitor();

  // Pull model: check router's last-seen timestamp for staleness
  bool CheckTimeout(EventQueue& eventQueue, const MessageRouter& router);
  void Reset();
  void SetTimeoutMs(uint32_t timeoutMs);

private:
  uint32_t timeoutMs_ = 1500; // Increased default timeout to reduce flicker to Degraded/Waiting
  // Emit only once when crossing the timeout threshold; cleared when fresh data arrives
  bool inTimeout_ = false;
};

#endif // HEALTH_MONITOR_H
