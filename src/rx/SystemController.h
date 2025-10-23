/**
 * @file SystemController.h
 * @brief Top-level state machine coordinating RX boot, UI, CAN consumption, and health.
 *
 * States: Boot → DisplayInit → WaitingForData → Active ⇄ Degraded → Fault.
 * Transitions are event-driven via EventQueue, with automatic recovery on fresh frames.
 */
#ifndef SYSTEM_CONTROLLER_H
#define SYSTEM_CONTROLLER_H

#include "EventQueue.h"
#include "CanInterface.h"
#include "UiController.h"
#include "HealthMonitor.h"
#include "common/MessageRouter.h"

/** Enumerates high-level runtime states. */
enum class SystemState : uint8_t
{
  Boot,
  DisplayInit,
  WaitingForData,
  Active,
  Degraded,
  Fault
};

/**
 * @class SystemController
 * @brief Drives the main RX control flow by consuming EventQueue and publishing to MessageRouter.
 */
class SystemController
{
public:
  SystemController(EventQueue& eventQueue, CanInterface& canInterface, 
                   UiController& uiController, HealthMonitor& healthMonitor,
                   MessageRouter& messageRouter);

  /** Run one-time boot sequence (display, driver init). */
  bool RunBootSequence();
  /** Handle a single event. Non-blocking; may transition state. */
  void Dispatch(const Event& event);
  /** Periodic tick for housekeeping (health checks, UI timers). */
  void Update();
  /** Current state accessor. */
  SystemState GetState() const { return currentState_; }

private:
  void TransitionTo(SystemState newState);
  void OnEnterBoot();
  void OnEnterDisplayInit();
  void OnEnterWaitingForData();
  void OnEnterActive();
  void OnEnterDegraded();
  void OnEnterFault();

  EventQueue& eventQueue_;
  CanInterface& canInterface_;
  UiController& uiController_;
  HealthMonitor& healthMonitor_;
  MessageRouter& messageRouter_;
  SystemState currentState_;
  uint8_t bootStepsCompleted_;
};

#endif // SYSTEM_CONTROLLER_H
