// Refactored RX firmware using modular state-machine architecture
// Each subsystem is encapsulated in its own module with clear interfaces

#include <Arduino.h>
#include "EventQueue.h"
#include "CanInterface.h"
#include "UiController.h"
#include "HealthMonitor.h"
#include "SystemController.h"
#include "common/MessageRouter.h"
#include "IOModule.h"

namespace
{
EventQueue eventQueue;
CanInterface canInterface;
UiController uiController;
HealthMonitor healthMonitor;
MessageRouter messageRouter;
IOModule ioModule;
SystemController* systemController = nullptr;

struct RouterSinks
{
  UiController* ui;
  HealthMonitor* health;
};

RouterSinks sinks{&uiController, &healthMonitor};

static void RouterUiHealthCb(const Cluster_t& c, uint32_t tsMs, void* ctx)
{
  auto* s = static_cast<RouterSinks*>(ctx);
  if (!s || !s->ui || !s->health) return;
  UiData ui{
    UiController::ConvertSpeedToArcValue(c.speed),
    static_cast<bool>(c.Left_Turn_Signal),
    static_cast<bool>(c.Right_Turn_Signal)
  };
  s->ui->EnqueueUiData(ui);
  s->health->NotifyFrame();
}
}

void setup()
{
  // Initialize event queue
  if (!eventQueue.Init(10))
  {
    // Fatal: cannot proceed without event queue
    while (true)
    {
      delay(1000);
    }
  }

  // Init message router
  messageRouter.Init(8);

  // Create system controller and run boot sequence
  systemController = new SystemController(eventQueue, canInterface, uiController, healthMonitor, messageRouter);
  
  if (!systemController->RunBootSequence())
  {
    // Boot failed, system is in Fault state
    // Controller will display fault message
  }

  // Initialize and start IO module (relays); subscribe to router
  ioModule.Init();
  ioModule.Start(messageRouter);
  // Subscribe UI+Health to router after UI task is running
  messageRouter.SubscribeCluster(&RouterUiHealthCb, &sinks);
  // UI task is started inside SystemController::RunBootSequence()
}

void loop()
{
  if (systemController == nullptr)
  {
    return;
  }

  // Process all pending events
  Event event;
  while (eventQueue.Pop(event, 0))
  {
    systemController->Dispatch(event);
  }

  // Update health monitoring (checks for timeouts)
  systemController->Update();

  // Update IO blink timing
  ioModule.Update(millis());

  // Brief delay to prevent watchdog timeout
  delay(5);
}