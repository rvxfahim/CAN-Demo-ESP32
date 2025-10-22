#ifndef SYSTEM_CONTROLLER_H
#define SYSTEM_CONTROLLER_H

#include "EventQueue.h"
#include "CanInterface.h"
#include "UiController.h"
#include "HealthMonitor.h"
#include "common/MessageRouter.h"

enum class SystemState : uint8_t
{
  Boot,
  DisplayInit,
  WaitingForData,
  Active,
  Degraded,
  Fault
};

class SystemController
{
public:
  SystemController(EventQueue& eventQueue, CanInterface& canInterface, 
                   UiController& uiController, HealthMonitor& healthMonitor,
                   MessageRouter& messageRouter);

  bool RunBootSequence();
  void Dispatch(const Event& event);
  void Update();
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
