#include "EventQueue.h"

EventQueue::EventQueue() : queueHandle_(nullptr)
{
}

EventQueue::~EventQueue()
{
  if (queueHandle_ != nullptr)
  {
    vQueueDelete(queueHandle_);
    queueHandle_ = nullptr;
  }
}

bool EventQueue::Init(uint16_t queueLength)
{
  queueHandle_ = xQueueCreate(queueLength, sizeof(Event));
  return queueHandle_ != nullptr;
}

bool EventQueue::Push(const Event& event)
{
  if (queueHandle_ == nullptr)
  {
    return false;
  }
  return xQueueSend(queueHandle_, &event, 0) == pdTRUE;
}

bool EventQueue::PushFromISR(const Event& event)
{
  if (queueHandle_ == nullptr)
  {
    return false;
  }
  BaseType_t higherPriorityTaskWoken = pdFALSE;
  const bool result = xQueueSendFromISR(queueHandle_, &event, &higherPriorityTaskWoken) == pdTRUE;
  portYIELD_FROM_ISR(higherPriorityTaskWoken);
  return result;
}

bool EventQueue::Pop(Event& event, TickType_t timeout)
{
  if (queueHandle_ == nullptr)
  {
    return false;
  }
  return xQueueReceive(queueHandle_, &event, timeout) == pdTRUE;
}
