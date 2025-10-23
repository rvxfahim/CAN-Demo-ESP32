// Refactored RX firmware using modular state-machine architecture
// Each subsystem is encapsulated in its own module with clear interfaces

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
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

// Forward declarations for FreeRTOS task
static void ProcessingTask(void* param);
static TaskHandle_t sProcessingTaskHandle = nullptr;

struct RouterSinks
{
  UiController* ui;
};

RouterSinks sinks{&uiController};

static void RouterUiCb(const Cluster_t& c, uint32_t tsMs, void* ctx)
{
  auto* s = static_cast<RouterSinks*>(ctx);
  if (!s || !s->ui) return;
  UiData ui{
    UiController::ConvertSpeedToArcValue(c.speed),
    static_cast<bool>(c.Left_Turn_Signal),
    static_cast<bool>(c.Right_Turn_Signal)
  };
  s->ui->EnqueueUiData(ui);
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
  messageRouter.SubscribeCluster(&RouterUiCb, &sinks);
  // UI task is started inside SystemController::RunBootSequence()

  // Create high-frequency processing task pinned to core 0
  // Runs the former loop() work at ~1 tick cadence
  xTaskCreatePinnedToCore(
      ProcessingTask,
      "RxProcessTask",
      4096,
      nullptr,
      2,
      &sProcessingTaskHandle,
      0  // core 0
  );
}

void loop()
{
  // Main Arduino loop sleeps; work handled by ProcessingTask on core 0
  vTaskDelay(pdMS_TO_TICKS(10000));
}

// Task pinned to core 0 executing the former loop() operations
namespace {
static void ProcessingTask(void* /*param*/)
{
  for(;;)
  {
    if (systemController != nullptr)
    {
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
    }

    // High-frequency cadence: sleep 1 tick to yield CPU
    vTaskDelay(1);
  }
}
}