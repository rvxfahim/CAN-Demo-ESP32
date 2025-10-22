# RX Firmware Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                         MAIN.CPP (Entry Point)                   │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────────┐   │
│  │EventQueue│  │CanInterface│ │UiController│ │HealthMonitor │   │
│  └────┬─────┘  └─────┬──────┘ └─────┬──────┘ └──────┬───────┘   │
│       │              │              │               │            │
│       └──────────────┴──────────────┴───────────────┘            │
│                             │                                    │
│                    ┌────────▼─────────┐                          │
│                    │ SystemController │                          │
│                    │  (State Machine) │                          │
│                    └──────────────────┘                          │
└─────────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────────┐
│                        STATE MACHINE FLOW                         │
│                                                                   │
│   ┌──────┐   I2C+CAN Init   ┌─────────────┐   LVGL Init         │
│   │ Boot ├──────────────────►DisplayInit  ├──────────┐           │
│   └───┬──┘      Success      └─────────────┘  Success│           │
│       │                                               │           │
│       │ Fail                                     ┌────▼────────┐  │
│       │                                          │WaitingForData│  │
│       │                                          └────┬─────────┘  │
│       │                                               │            │
│       │                                    ClusterFrame Event      │
│       │                                               │            │
│   ┌───▼───┐◄──────────────────────────────────┐  ┌──▼──────┐     │
│   │ Fault │         Error/InitFail            │  │ Active  │     │
│   └───────┘◄─────────────┐                    │  └──┬──┬───┘     │
│                           │                    │     │  │         │
│                           │                    │     │  │ Frame   │
│                      ┌────┴────────┐  Timeout  │     │  │ Timeout │
│                      │  Degraded   ├───────────┘     │  │         │
│                      │(Stale Data) │◄────────────────┘  │         │
│                      └─────────────┘   ClusterFrame     │         │
│                                         (Recovery)       │         │
│                                                          │         │
│                      HealthMonitor checks every loop ────┘         │
└──────────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────────┐
│                        EVENT FLOW (ISR → MAIN)                    │
│                                                                   │
│  CAN ISR                   EventQueue                 Main Loop  │
│    │                           │                          │       │
│    │  CanMsgHandler()          │                          │       │
│    ├──► Validate Frame         │                          │       │
│    │    Unpack DBC             │                          │       │
│    │    MakeClusterFrame()     │                          │       │
│    └───► PushFromISR() ────────┤                          │       │
│                                 │   Pop()                  │       │
│                                 ├────────────────────────► │       │
│                                 │                          │       │
│                                 │   SystemController::     │       │
│                                 │   Dispatch(event)        │       │
│                                 │         │                │       │
│                                 │         ▼                │       │
│                                 │   UiController::         │       │
│                                 │   ApplyCluster()         │       │
│                                 │         │                │       │
│                                 │         ▼                │       │
│                                 │   LVGL Widgets Updated   │       │
│                                 │                          │       │
│  Timer Check (every loop)       │                          │       │
│    │                            │                          │       │
│    │  HealthMonitor::           │                          │       │
│    │  CheckTimeout()            │                          │       │
│    └───► MakeFrameTimeout() ───┤                          │       │
│              (if expired)       │                          │       │
└──────────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────────┐
│                    MODULE RESPONSIBILITIES                        │
│                                                                   │
│  EventQueue:         Type-safe event bus (FreeRTOS queue)        │
│                      - Push/Pop events                            │
│                      - ISR-safe operations                        │
│                                                                   │
│  CanInterface:       CAN hardware abstraction                    │
│                      - Setup GPIO/pins/filters                    │
│                      - ISR callback validates & unpacks           │
│                      - Pushes ClusterFrame events                 │
│                                                                   │
│  UiController:       Display & GUI management                    │
│                      - LVGL/TFT/Touch initialization              │
│                      - Widget updates (arc, turn signals)         │
│                      - Degraded/Fault overlays                    │
│                      - Timer servicing                            │
│                                                                   │
│  HealthMonitor:      Data freshness watchdog                     │
│                      - Tracks last frame timestamp                │
│                      - Emits FrameTimeout events                  │
│                      - 500ms threshold                            │
│                                                                   │
│  SystemController:   State machine orchestrator                  │
│                      - Owns SystemState enum                      │
│                      - Dispatches events to modules               │
│                      - Manages state transitions                  │
│                      - Coordinates boot sequence                  │
└──────────────────────────────────────────────────────────────────┘
```

## Key Design Principles

1. **Separation of Concerns**: Each module has one clear responsibility
2. **Event-Driven**: ISR → Events → State Machine (no direct coupling)
3. **ISR Safety**: All ISR callbacks only validate & queue, heavy work in main loop
4. **Testability**: Modules can be unit tested independently
5. **Maintainability**: Adding features only requires modifying relevant module
6. **State Machine**: Clear lifecycle management with explicit transitions

## Example: ClusterFrame Processing Path

```
1. TX Board sends CAN frame (ID 0x65, 3 bytes)
2. ESP32 CAN controller triggers interrupt
3. CanInterface::CanMsgHandler() called (ISR context)
   - Validates ID/DLC/IDE
   - Unpacks via DBC: Unpack_Cluster_lecture()
   - Creates ClusterFrame event
   - Pushes to EventQueue (ISR-safe)
4. Main loop: EventQueue.Pop() retrieves event
5. SystemController::Dispatch() receives event
   - Checks current state (Active/WaitingForData/Degraded)
   - Calls UiController::ApplyCluster()
   - Calls HealthMonitor::NotifyFrame()
6. UiController updates:
   - Speed arc value (0-240 range)
   - Left turn indicator opacity (0 or 255)
   - Right turn indicator opacity (0 or 255)
   - Hides degraded warning if visible
7. HealthMonitor resets watchdog timer
8. Next loop iteration: HealthMonitor::CheckTimeout()
   - If >500ms since last frame → pushes FrameTimeout event
   - SystemController transitions Active → Degraded
   - UiController::ShowDegraded() displays warning
```
