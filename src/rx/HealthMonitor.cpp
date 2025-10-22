#include "HealthMonitor.h"
#include "common/MessageRouter.h"
#include <Arduino.h>

HealthMonitor::HealthMonitor()
{
}

bool HealthMonitor::CheckTimeout(EventQueue& eventQueue, const MessageRouter& router)
{
  uint32_t lastSeenMs = 0;
  if (!router.GetLastSeenMs(lastSeenMs))
  {
    // No data ever published yet
    inTimeout_ = false; // treat as not timed out yet; avoid spamming
    return false;
  }
  const uint32_t nowMs = millis();
  const uint32_t elapsed = nowMs - lastSeenMs;
  if (elapsed >= timeoutMs_)
  {
    // Only emit once when crossing into timeout state
    if (!inTimeout_)
    {
      inTimeout_ = true;
      Event event = Event::MakeFrameTimeout();
      eventQueue.Push(event);
      return true;
    }
    return false; // already in timeout; don't spam queue
  }

  // Fresh data seen again; clear timeout state
  if (inTimeout_)
  {
    inTimeout_ = false;
  }
  return false;
}

void HealthMonitor::Reset()
{
  inTimeout_ = false;
}

void HealthMonitor::SetTimeoutMs(uint32_t timeoutMs)
{
  timeoutMs_ = timeoutMs;
}
