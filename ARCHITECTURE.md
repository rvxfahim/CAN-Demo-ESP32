# Firmware Architecture (Implemented)

This document reflects the current modular architecture with a central MessageRouter and an event-driven state machine on the RX board, plus a minimal TX board that publishes CAN frames from DBC-generated types.

## RX: Top-Level Layout

```
┌─────────────────────────────────────────────────────────────────┐
│                         MAIN.CPP (Entry Point)                   │
│  ┌──────────┐  ┌──────────────┐  ┌──────────────┐              │
│  │EventQueue│  │ CanInterface │  │ SystemController │          │
│  └────┬─────┘  └──────┬───────┘  └────────┬───────┘            │
│       │                │                   │                    │
│       └────────────────┴───────────────────┘                    │
│                           │                                     │
│                    ┌──────▼───────┐                              │
│                    │ MessageRouter│  (pub/sub + last-seen)       │
│                    └──────┬───────┘                              │
│                           │                                     │
│      ┌────────────────────┼──────────────────────┐               │
│  ┌───▼───────┐        ┌───▼───────┐        ┌────▼────────┐      │
│  │UiController│        │ IOModule  │        │HealthMonitor│      │
│  └────────────┘        └───────────┘        └─────────────┘      │
└─────────────────────────────────────────────────────────────────┘
```

## RX: State Machine Flow

```
Boot → DisplayInit → WaitingForData → Active
Active ⇄ Degraded (stale data)
Any → Fault (unrecoverable)

Triggers:
- First Cluster frame → WaitingForData → Active
- HealthMonitor timeout → Active → Degraded
- New Cluster after Degraded → Degraded → Active (auto-recovery)
```

## RX: Event/Data Flow

```
CAN ISR → EventQueue → SystemController → MessageRouter → Subscribers

1) ISR (CanInterface::CanMsgHandler)
   - Validates ID/DLC/IDE for Cluster (0x65, 3 bytes, std)
   - Unpacks to Cluster_t via Unpack_Cluster_lecture()
   - Pushes Event::ClusterFrame to EventQueue (ISR-safe)

2) Main loop
   - Pop event; SystemController::Dispatch(event)
   - Publish Cluster_t to MessageRouter with millis() timestamp
   - Update state transitions

3) Subscribers
   - UiController: maps Cluster_t → widgets; services lv_timer_handler()
   - IOModule: drives relay GPIOs; internal 1 Hz blink independent of bus rate
   - HealthMonitor: pull last-seen from router; emits FrameTimeout to queue
```

## RX: Module Responsibilities

- EventQueue (`src/rx/EventQueue.{h,cpp}`)
  - Typed FreeRTOS queue with ISR-safe push; factories for events

- CanInterface (`src/rx/CanInterface.{h,cpp}`)
  - Init CAN (pins 35/5, 500 kbps), mailbox filter for 0x65
  - ISR: validate → unpack DBC → push ClusterFrame event

- MessageRouter (`src/common/MessageRouter.{h,cpp}`)
  - Pub/sub for Cluster topic; sticky last value; last-seen timestamp (ms)

- UiController (`src/rx/UiController.{h,cpp}`)
  - LVGL/TFT/Touch init; updates `ui_Arc1`, left/right labels
  - Subscribes to router; runs UI task/timers

- IOModule (`src/rx/IOModule.{h,cpp}`)
  - Relay control for turn indicators; subscribes to router
  - Blink cadence decoupled from CAN message rate

- HealthMonitor (`src/rx/HealthMonitor.{h,cpp}`)
  - Pull model: checks router last-seen; emits FrameTimeout on staleness

- SystemController (`src/rx/SystemController.{h,cpp}`)
  - Orchestrates states; publishes Cluster_t to router; handles recovery

## RX: Design Principles

1. Event-driven isolation: ISR only validates/unpacks/queues
2. Central truth: Router caches last value + timestamp for freshness
3. Decoupled consumers: UI/IO subscribe; no direct controller wiring
4. Robustness: HealthMonitor detects loss and triggers Degraded; auto-recovery
5. Portability: DBC-generated types (`Cluster_t`) are the single frame model

## RX: Example — Cluster Processing Path

```
1) TX sends Cluster (0x65, 3 bytes)
2) ISR validates + Unpack_Cluster_lecture() → EventQueue::MakeClusterFrame()
3) Main pops event → SystemController publishes Cluster_t to MessageRouter
4) UiController subscriber updates:
   - lv_arc_set_value(ui_Arc1, map(speed_raw, 0..4095 → 0..240))
   - lv_obj_set_style_text_opa(left/right, 0 or 255)
5) IOModule toggles relays based on left/right with 1 Hz cadence
6) HealthMonitor sees last-seen aging; if > threshold → FrameTimeout → Degraded
7) New frame arrives → router timestamp updated → Active
```

---

## TX: Overview

TX board is a small producer of `Cluster` frames (ID 0x65), without LVGL or UI. It runs independently and can be used as a signal generator for RX testing.

- Entry point: `src/tx/main.cpp`
- Build env: `env:tx_board` in `platformio.ini` (no LVGL/TFT)
- DBC types: `Cluster_t`, packed via `Pack_Cluster_lecture`
- Driver usage:
  - `CAN0.setCANPins(GPIO_NUM_35, GPIO_NUM_5);`
  - `CAN0.begin(500000);`
  - `CAN0.setDebuggingMode(true);` (verbose diagnostics)
- Frame cadence: typically 20–100 Hz during tests
- Scope for extension: additional messages can be added by packing other DBC-defined signals

## Build/Env Notes

- Two PlatformIO environments: `rx_board` (UI + CAN) and `tx_board` (CAN only)
- Shared code goes under `src/common/` and DBC wrapper `src/generated_lecture_dbc.c`
- Per-board selection is controlled by `build_src_filter` and `build_flags` (RX_BOARD / TX_BOARD)

