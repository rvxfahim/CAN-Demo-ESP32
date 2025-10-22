#include "HealthMonitor.h"

HealthMonitor::HealthMonitor() : lastFrameTick_(0)
{
}

void HealthMonitor::NotifyFrame()
{
  lastFrameTick_ = xTaskGetTickCount();
}

bool HealthMonitor::CheckTimeout(EventQueue& eventQueue)
{
  const TickType_t currentTick = xTaskGetTickCount();
  const TickType_t elapsedTicks = currentTick - lastFrameTick_;
  const TickType_t timeoutTicks = pdMS_TO_TICKS(timeoutMs_);

  if (elapsedTicks >= timeoutTicks && lastFrameTick_ != 0)
  {
    Event event = Event::MakeFrameTimeout();
    eventQueue.Push(event);
    return true;
  }

  return false;
}

void HealthMonitor::Reset()
{
  lastFrameTick_ = 0;
}

void HealthMonitor::SetTimeoutMs(uint32_t timeoutMs)
{
  timeoutMs_ = timeoutMs;
}
