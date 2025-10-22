# CAN Lecture RX Firmware — Modular Architecture (with Message Router)

This README describes the implemented RX firmware architecture. It replaces the earlier refactor plan with the current, working design that introduces a MessageRouter for modular fan-out of CAN messages and a dedicated IO relay module for blinkers.

## High-level data flow

1) CAN ISR (esp32_can) → EventQueue
   - `CanInterface` sets up pins, bitrate, and mailbox filtering for `Cluster` message (ID 0x65).
   - ISR validates ID/DLC/IDE and unpacks the frame into `Cluster_t` (generated from DBC), then pushes `Event::ClusterFrame` to a FreeRTOS queue (`EventQueue`).

2) SystemController (state machine) → MessageRouter
   - `SystemController::Dispatch(EventType::ClusterFrame)` handles state transitions (`WaitingForData` → `Active`, recovery from `Degraded`).
   - It unconditionally publishes the `Cluster_t` to the `MessageRouter` with a millisecond timestamp.

3) MessageRouter (central pub/sub)
   - Typed topic for `Cluster_t` with subscribe/unsubscribe and a sticky last-value cache.
   - Maintains a last-seen timestamp for freshness checks.

4) Subscribers consume from the router
   - `UiController`: subscribed via a small adapter, maps `Cluster_t` to `UiData` and enqueues it to the UI task (LVGL thread).
   - `IOModule` (blinkers): subscribed; drives two relay GPIOs for left/right indicators with its own blink state machine (independent of message rate).
   - `HealthMonitor`: pull model; no longer notified by subscribers. It queries the router’s last-seen timestamp to detect staleness and emits `FrameTimeout` events.

5) SystemController::Update()
   - Periodically asks `HealthMonitor` to check for timeouts (pulling timestamps from the router) and transitions to `Degraded` on staleness.

## State machine

`SystemState` lifecycle is unchanged but simplified by the router:

- Boot → DisplayInit → WaitingForData → Active
- Active ⇄ Degraded on `FrameTimeout` and recovery when frames resume
- Fault for unrecoverable errors

`SystemController` still owns transitions and UI screen changes; modules subscribe to data via the router rather than being hard-wired in the controller.

## Modules (responsibilities and locations)

- CanInterface (`src/rx/CanInterface.{h,cpp}`)
  - Configures CAN and registers ISR.
  - Unpacks `Cluster_t` in ISR and pushes `Event::ClusterFrame` to `EventQueue`.

- EventQueue (`src/rx/EventQueue.{h,cpp}`)
  - FreeRTOS queue wrapper for typed events from ISR and other producers.

- MessageRouter (`src/common/MessageRouter.{h,cpp}`)
  - Typed subscription API: `SubscribeCluster`, `PublishCluster`.
  - Sticky last-value cache and `GetLastSeenMs()` for freshness.
  - All subscribers run in task context (publish is called from main loop via controller dispatch).

- UiController (`src/rx/UiController.{h,cpp}`)
  - Initializes LVGL, TFT_eSPI, touch, and SquareLine-generated UI.
  - Dedicated UI task with queues; subscribed to router for `Cluster` updates (via adapter in `main.cpp`).

- IOModule (`src/rx/IOModule.{h,cpp}`)
  - Drives two relay outputs for left/right turn indicators.
  - Subscribes to `Cluster` via router.
  - Internal blink state machine at 1 Hz (500 ms ON / 500 ms OFF), decoupled from message rate.
  - Safe default OFF if no update for >1s.
  - Relay pins configurable in `include/IOPins.h` (defaults: GPIO 25 and 26).

- HealthMonitor (`src/rx/HealthMonitor.{h,cpp}`)
  - Pull model for staleness: `CheckTimeout(EventQueue&, const MessageRouter&)` reads `GetLastSeenMs()` and emits `FrameTimeout` if stale (default 1500 ms, configurable).

- SystemController (`src/rx/SystemController.{h,cpp}`)
  - Orchestrates boot, display init, waiting/active/degraded/fault.
  - Publishes `Cluster_t` to router on each frame event and handles state transitions.

- Entry points (`src/rx/main.cpp`)
  - Instantiates modules and runs the boot sequence.
  - Subscribes UI to router; starts IO module subscription.
  - Calls `ioModule.Update(millis())` for blink timing and `systemController.Update()` for health.

## Message formats and DBC integration

- Source of truth: `tools/Lecture.dbc`.
- Code generation: `lib/Generated/lib/lecture.{c,h}` (do not edit generated files).
- Wrapper: `src/generated_lecture_dbc.c` includes generated code; use `Cluster_t`, `Pack_Cluster_lecture`, `Unpack_Cluster_lecture`.

## Extending the system

- To add a new consumer of `Cluster` data, subscribe via `MessageRouter::SubscribeCluster(cb, ctx)` and process updates in task context.
- For additional CAN messages/IDs:
  - Extend `CanInterface` to push new events.
  - Add typed topics (similar to `Cluster`) in `MessageRouter` or expose per-ID raw subscriptions.
  - Have modules subscribe to those topics rather than plumbing through `SystemController`.

## Build, upload, monitor (PlatformIO)

- Build RX: `pio run -e rx_board`
- Upload RX: `pio run -e rx_board -t upload`
- Monitor RX: `pio device monitor -e rx_board`

Serial ports and speeds are set in `platformio.ini` per environment.

## Notable implementation details

- ISR is minimal and never calls subscribers; it only validates, unpacks, and queues events.
- Router timestamps are in milliseconds (`millis()`), providing a single source of truth for freshness.
- UI and IO operate fully from router data; controller no longer pushes UI payloads directly.
- Blinkers are immune to CAN update rate; they align phase on rising edges and toggle at a fixed cadence.

## Troubleshooting

- If the UI doesn’t update: ensure UI task started before router subscription (it is subscribed after `RunBootSequence()` in `main.cpp`).
- If blinkers don’t actuate: verify relay pins in `include/IOPins.h` match hardware; confirm `Cluster.Left_Turn_Signal`/`Right_Turn_Signal` bits from TX.
- If system drops to Degraded: router last-seen likely stale; check TX is sending `Cluster` frames and bus wiring.

## References

- LVGL v9.1
- esp32_can (custom wrapper in `lib/CanDriver`)
- DBC generator under `tools/c-coderdbc/`
