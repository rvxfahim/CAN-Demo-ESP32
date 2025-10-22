#include "SystemController.h"
#include <Arduino.h>
#include <Wire.h>

SystemController::SystemController(EventQueue& eventQueue, CanInterface& canInterface,
                                   UiController& uiController, HealthMonitor& healthMonitor,
                                   MessageRouter& messageRouter)
  : eventQueue_(eventQueue),
    canInterface_(canInterface),
    uiController_(uiController),
    healthMonitor_(healthMonitor),
    messageRouter_(messageRouter),
    currentState_(SystemState::Boot),
    bootStepsCompleted_(0)
{
}

bool SystemController::RunBootSequence()
{
  OnEnterBoot();

  // Initialize I2C (required for touchscreen)
  Wire.begin();

  // Initialize CAN interface
  if (!canInterface_.Init(eventQueue_))
  {
    Event event = Event::MakeInitFail(Subsystem::CAN);
    eventQueue_.Push(event);
    TransitionTo(SystemState::Fault);
    return false;
  }

  Event event = Event::MakeInitOk(Subsystem::CAN);
  eventQueue_.Push(event);

  // Transition to display initialization (log only for now)
  TransitionTo(SystemState::DisplayInit);
  OnEnterDisplayInit();

  // Initialize UI
  if (!uiController_.Init())
  {
    Event failEvent = Event::MakeInitFail(Subsystem::Display);
    eventQueue_.Push(failEvent);
    TransitionTo(SystemState::Fault);
    return false;
  }

  Event okEvent = Event::MakeInitOk(Subsystem::Display);
  eventQueue_.Push(okEvent);

  // Start UI task so all subsequent LVGL operations happen in a single thread
  if (!uiController_.StartTask())
  {
    Event failEvent = Event::MakeInitFail(Subsystem::LVGL);
    eventQueue_.Push(failEvent);
    TransitionTo(SystemState::Fault);
    return false;
  }

  // Now it's safe to send UI messages
  uiController_.EnqueueMessage(UiMessage::MakeShowLog());
  uiController_.EnqueueMessage(UiMessage::MakeAddLog("System booting..."));
  uiController_.EnqueueMessage(UiMessage::MakeAddLog("Initializing display..."));

  // Boot successful, transition to waiting for data
  TransitionTo(SystemState::WaitingForData);
  OnEnterWaitingForData();

  // Increase stale-data timeout to reduce aggressive fallback
  healthMonitor_.SetTimeoutMs(1500);

  return true;
}

void SystemController::Dispatch(const Event& event)
{
  switch (event.type)
  {
    case EventType::InitOk:
      bootStepsCompleted_++;
      break;

    case EventType::InitFail:
      TransitionTo(SystemState::Fault);
      break;

    case EventType::ClusterFrame:
      if (currentState_ == SystemState::WaitingForData)
      {
        TransitionTo(SystemState::Active);
      }
      else if (currentState_ == SystemState::Degraded)
      {
        // Recover from degraded state
        TransitionTo(SystemState::Active);
      }
      // Always publish to message router so all subscribers receive the latest Cluster
      messageRouter_.PublishCluster(event.payload.clusterData, millis());
      break;

    case EventType::FrameTimeout:
      if (currentState_ == SystemState::Active)
      {
        TransitionTo(SystemState::Degraded);
      }
      break;

    case EventType::Error:
      TransitionTo(SystemState::Fault);
      break;

    default:
      break;
  }
}

void SystemController::Update()
{
  // Check for timeout in Active state
  if (currentState_ == SystemState::Active || currentState_ == SystemState::Degraded)
  {
    healthMonitor_.CheckTimeout(eventQueue_);
  }
}

void SystemController::TransitionTo(SystemState newState)
{
  if (currentState_ == newState)
  {
    return;
  }

  currentState_ = newState;

  switch (newState)
  {
    case SystemState::Boot:
      OnEnterBoot();
      break;
    case SystemState::DisplayInit:
      OnEnterDisplayInit();
      break;
    case SystemState::WaitingForData:
      OnEnterWaitingForData();
      break;
    case SystemState::Active:
      OnEnterActive();
      break;
    case SystemState::Degraded:
      OnEnterDegraded();
      break;
    case SystemState::Fault:
      OnEnterFault();
      break;
  }
}

void SystemController::OnEnterBoot()
{
  bootStepsCompleted_ = 0;
  Serial.begin(115200);
  Serial.println("System booting...");
  // Defer UI changes until UI task is started
}

void SystemController::OnEnterDisplayInit()
{
  Serial.println("Initializing display...");
  // UI messages will be sent after the UI task is started
}

void SystemController::OnEnterWaitingForData()
{
  Serial.println("Waiting for CAN data...");
  healthMonitor_.Reset();
  uiController_.EnqueueMessage(UiMessage::MakeShowLog());
  uiController_.EnqueueMessage(UiMessage::MakeAddLog("Waiting for CAN data..."));
}

void SystemController::OnEnterActive()
{
  Serial.println("System active");
  uiController_.EnqueueMessage(UiMessage::MakeShowDashboard());
}

void SystemController::OnEnterDegraded()
{
  Serial.println("WARNING: Stale data detected");
  uiController_.EnqueueMessage(UiMessage::MakeShowDegraded());
  uiController_.EnqueueMessage(UiMessage::MakeShowLog());
  uiController_.EnqueueMessage(UiMessage::MakeAddLog("WARNING: Stale data detected"));
}

void SystemController::OnEnterFault()
{
  Serial.println("FAULT: System halted");
  uiController_.EnqueueMessage(UiMessage::MakeShowFault());
  uiController_.EnqueueMessage(UiMessage::MakeShowLog());
  uiController_.EnqueueMessage(UiMessage::MakeAddLog("FAULT: System halted"));
}
