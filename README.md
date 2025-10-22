# RX Firmware State-Machine Refactor Plan

This document instructs the implementation agent on how to reorganize the RX firmware into a modular state-machine architecture. Follow each step in order and keep modules decoupled via clearly defined interfaces.

## 1. Target Architecture
- **SystemController module**: owns the high-level `SystemState` machine and orchestrates transitions.
- **EventQueue module**: central buffer that converts CAN callbacks and timers into typed events consumed by the controller.
- **CanInterface module**: wraps CAN initialization, filters, ISR callbacks, and parsing of `Cluster_t` frames.
- **UiController module**: encapsulates LVGL/TFT/Touch init plus all widget updates based on sanitized data.
- **HealthMonitor module**: tracks freshness of incoming data (timeouts) and escalates warnings/faults.
- **Application entry points (`setup`/`loop`)**: become thin shims that delegate to the modules above.

Each module should have its own header/source pair inside `src/` (or `lib/` if preferred) so the resulting code base is easy to test and maintain.

## 2. State Definitions
Create an enum class in `SystemController` describing the lifecycle:
1. `Boot` – aggregate hardware init; enter `DisplayInit` when all subsystems report OK, otherwise `Fault`.
2. `DisplayInit` – configure LVGL, TFT, touch, and UI assets; success → `WaitingForData`, failure → `Fault`.
3. `WaitingForData` – start frame timeout timer; stay idle until `ClusterFrame` event arrives.
4. `Active` – consume cluster frames, refresh UI, reset watchdog on every valid frame.
5. `Degraded` – triggered by watchdog expiry; show stale-data warning while still processing late frames.
6. `Fault` – display fatal overlay and suspend non-critical tasks; allow manual retry hook if needed.

Transitions are driven exclusively by events generated in `EventQueue`.

## 3. Event Model
Define `enum class EventType` covering:
- `InitOk`, `InitFail` (with subsystem identifier)
- `ClusterFrame` (payload: latest `Cluster_t`)
- `FrameTimeout`
- `Error` (payload: error code or enum)

Implement `struct Event` carrying the type and optional payloads. `EventQueue` should provide `Push(Event)` and `Pop(Event&)` helpers wrapping FreeRTOS queues.

## 4. Module Responsibilities
- **CanInterface**
  - Expose `bool Init(EventQueue&)` for boot-time configuration.
  - In ISR callback, parse `Cluster_t` only when ID/DLC/IDE match, then push `ClusterFrame`.
  - Guard against queue saturation using `xQueueOverwriteFromISR` equivalents.

- **UiController**
  - Provide `bool Init()` (display + touch + LVGL + `ui_init`).
  - Encapsulate helper such as `void ApplyCluster(const Cluster_t&)`, `void ShowDegraded()`, `void ShowFault()`.
  - Keep static conversion utilities private to this module.

- **HealthMonitor**
  - Maintain `TickType_t lastFrameTick`.
  - Expose `void NotifyFrame()` and `bool CheckTimeout(EventQueue&)` to emit `FrameTimeout` events at configured intervals (e.g., 500 ms).

- **SystemController**
  - Hold current state and per-state entry/exit hooks.
  - Provide `void Dispatch(const Event&)` to interpret events and call module methods.
  - Call `HealthMonitor::CheckTimeout` on every loop tick.

## 5. Refactoring Steps
1. Extract current CAN setup and callback logic from `src/rx/main.cpp` into `CanInterface`.
2. Move LVGL/TFT/touch setup plus `UpdateClusterUi` helpers into `UiController`.
3. Build the `EventQueue` abstraction around the existing FreeRTOS queue.
4. Introduce `HealthMonitor` with configurable timeout constants.
5. Implement `SystemController` wiring the modules together and handling state transitions.
6. Rewrite `setup()` to instantiate the modules, run boot sequence, and enqueue initial events.
7. Rewrite `loop()` to pump events from `EventQueue`, call `SystemController::Dispatch`, service LVGL timers, and run the health check.
8. Add comments documenting non-obvious logic per the project style (concise, purpose-driven).

## 6. Testing & Validation
- Provide unit tests or at least integration tests for state transitions where feasible.
- On hardware, verify:
  - Successful boot reaches `Active` when CAN traffic is present.
  - Removing CAN traffic triggers `Degraded` overlay within the timeout window.
  - Reintroducing traffic recovers to `Active` without reboot.
  - Fault paths display the correct UI and halt updates.

## 7. Deliverables
- Updated source files under `src/` (or dedicated subfolders) matching the modular layout.
- Revised `platformio.ini` only if new files or build flags require it.
- Brief summary of implemented modules and any deviations from this plan.

## 8. Reference Material
- When low-level return types or HAL semantics are unclear, consult the ESP-IDF headers under `C:\Users\fahim\.platformio\packages\framework-arduinoespressif32\tools\sdk\esp32` (also mounted in the workspace).

Follow these instructions to keep the refactor modular, testable, and aligned with the defined state-machine design.
