/**
 * @file HealthMonitor.h
 * @brief Monitors freshness of CAN data and emits timeout events.
 *
 * Polls MessageRouter last-seen timestamp and generates a single crossing event
 * when data becomes stale; resets on fresh frames.
 */
#ifndef HEALTH_MONITOR_H
#define HEALTH_MONITOR_H

#include "freertos/FreeRTOS.h"
#include "EventQueue.h"

class MessageRouter; // forward declare for pull model

/**
 * @class HealthMonitor
 * @brief Simple watchdog for Cluster flow staleness.
 */
class HealthMonitor
{
public:
  HealthMonitor();

  // Pull model: check router's last-seen timestamp for staleness
  /**
   * @brief Check for timeout vs last-seen timestamp and enqueue events.
   * @return true if a timeout crossing was detected and an event enqueued.
   */
  bool CheckTimeout(EventQueue& eventQueue, const MessageRouter& router);
  /** Reset internal timeout latch. */
  void Reset();
  /** Configure timeout threshold in milliseconds. */
  void SetTimeoutMs(uint32_t timeoutMs);

private:
  uint32_t timeoutMs_ = 1500; // Increased default timeout to reduce flicker to Degraded/Waiting
  // Emit only once when crossing the timeout threshold; cleared when fresh data arrives
  bool inTimeout_ = false;
};

#endif // HEALTH_MONITOR_H
