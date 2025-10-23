/**
 * @file EventQueue.h
 * @brief FreeRTOS-backed event queue used to bridge ISR -> task and orchestrate system state.
 *
 * The queue transports initialization results, decoded Cluster frames, timeouts and error codes.
 * PushFromISR() is safe from interrupt context; Pop() drains events in the main loop or tasks.
 */
#ifndef EVENT_QUEUE_H
#define EVENT_QUEUE_H

#include <cstdint>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "lecture.h"

/** High-level event categories transported through EventQueue. */
enum class EventType : uint8_t
{
  InitOk,
  InitFail,
  ClusterFrame,
  FrameTimeout,
  Error
};

/** Subsystems that may emit Init/Error events. */
enum class Subsystem : uint8_t
{
  CAN,
  Display,
  Touch,
  LVGL,
  UI
};

/**
 * @struct Event
 * @brief Variant payload for queue transport.
 */
struct Event
{
  EventType type;
  union {
    Subsystem subsystem;       // For InitOk/InitFail/Error
    Cluster_t clusterData;     // For ClusterFrame
    uint32_t errorCode;        // For Error
  } payload;

  Event() : type(EventType::InitOk) { payload.subsystem = Subsystem::CAN; }
  
  static Event MakeInitOk(Subsystem sys)
  {
    Event e;
    e.type = EventType::InitOk;
    e.payload.subsystem = sys;
    return e;
  }

  static Event MakeInitFail(Subsystem sys)
  {
    Event e;
    e.type = EventType::InitFail;
    e.payload.subsystem = sys;
    return e;
  }

  static Event MakeClusterFrame(const Cluster_t& cluster)
  {
    Event e;
    e.type = EventType::ClusterFrame;
    e.payload.clusterData = cluster;
    return e;
  }

  static Event MakeFrameTimeout()
  {
    Event e;
    e.type = EventType::FrameTimeout;
    return e;
  }

  static Event MakeError(uint32_t code)
  {
    Event e;
    e.type = EventType::Error;
    e.payload.errorCode = code;
    return e;
  }
};

/**
 * @class EventQueue
 * @brief Thin wrapper around FreeRTOS queue for typed Event messages.
 */
class EventQueue
{
public:
  EventQueue();
  ~EventQueue();

  /** Initialize internal FreeRTOS queue. */
  bool Init(uint16_t queueLength = 10);
  /** Enqueue an event from task context. */
  bool Push(const Event& event);
  /** Enqueue an event from ISR context. */
  bool PushFromISR(const Event& event);
  /** Dequeue an event with optional timeout. */
  bool Pop(Event& event, TickType_t timeout = 0);

private:
  QueueHandle_t queueHandle_;
};

#endif // EVENT_QUEUE_H
