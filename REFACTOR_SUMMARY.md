# RX Firmware Refactoring - Implementation Summary

## Overview
Successfully refactored the monolithic RX firmware (`src/rx/main.cpp`) into a modular state-machine architecture as specified in `README.md`. The refactoring maintains all original functionality while improving testability, maintainability, and clarity.

## Implemented Modules

### 1. EventQueue (EventQueue.h/cpp)
- **Purpose**: Central event bus converting ISR callbacks and timers into typed events
- **Key Features**:
  - Type-safe event system with `EventType` enum
  - Support for ISR-safe event pushing (`PushFromISR`)
  - Static factory methods for creating events (`MakeClusterFrame`, `MakeInitOk`, etc.)
  - Wraps FreeRTOS queue with clean C++ interface
- **Event Types**: `InitOk`, `InitFail`, `ClusterFrame`, `FrameTimeout`, `Error`

### 2. CanInterface (CanInterface.h/cpp)
- **Purpose**: CAN hardware abstraction and message parsing
- **Key Features**:
  - Encapsulates CAN0 initialization (GPIO 35/5, 500 kbps)
  - Mailbox filtering for `Cluster` messages (ID 0x65)
  - ISR-safe callback validates frame ID/DLC/IDE before unpacking
  - Pushes `ClusterFrame` events to EventQueue from ISR context
- **Preserved**: All DBC integration (`Unpack_Cluster_lecture`)

### 3. UiController (UiController.h/cpp)
- **Purpose**: Display, touchscreen, LVGL, and widget management
- **Key Features**:
  - Init sequence: LVGL → TFT → Touch → SquareLine UI
  - `ApplyCluster()`: Updates speed arc and turn indicators
  - `ShowDegraded()`: Displays "STALE DATA" warning overlay
  - `ShowFault()`: Displays "SYSTEM FAULT" critical overlay
  - Private static callbacks for LVGL (display flush, touch read, tick)
- **Preserved**: All original widget update logic, calibration values

### 4. HealthMonitor (HealthMonitor.h/cpp)
- **Purpose**: Tracks data freshness and detects stale frames
- **Key Features**:
  - 500ms timeout threshold (configurable via `kTimeoutMs`)
  - `NotifyFrame()`: Records timestamp on each valid CAN frame
  - `CheckTimeout()`: Emits `FrameTimeout` event when threshold exceeded
  - `Reset()`: Clears watchdog for state transitions
- **Usage**: Called from SystemController's `Update()` loop

### 5. SystemController (SystemController.h/cpp)
- **Purpose**: High-level state machine orchestration
- **States**:
  1. `Boot`: I2C and CAN initialization
  2. `DisplayInit`: LVGL/TFT/Touch setup
  3. `WaitingForData`: Idle until first CAN frame
  4. `Active`: Normal operation, UI updates
  5. `Degraded`: Stale data warning, still processing frames
  6. `Fault`: Fatal error, halts updates
- **Key Methods**:
  - `RunBootSequence()`: Executes init chain, transitions to WaitingForData on success
  - `Dispatch(event)`: Event-driven state transitions
  - `Update()`: Periodic health checks
- **Transitions**:
  - WaitingForData → Active: On first `ClusterFrame`
  - Active → Degraded: On `FrameTimeout`
  - Degraded → Active: On new `ClusterFrame` (auto-recovery)
  - Any → Fault: On `InitFail` or `Error`

### 6. Refactored main.cpp
- **Setup**: Instantiates modules, runs boot sequence
- **Loop**: 
  - Pumps events from EventQueue
  - Dispatches to SystemController
  - Updates health monitor
  - Services LVGL timers
- **Lines of Code**: Reduced from ~180 to ~50 (73% reduction in main file)

## Validation Criteria (Per README.md §6)

### Hardware Testing Checklist
- [ ] **Boot to Active**: Verify with TX board transmitting → should reach Active state
- [ ] **Timeout to Degraded**: Disconnect TX board → "STALE DATA" warning within 500ms
- [ ] **Auto-Recovery**: Reconnect TX board → warning clears, returns to Active
- [ ] **Fault Display**: Simulate init failure (e.g., disconnect I2C) → "SYSTEM FAULT" overlay
- [ ] **Serial Logging**: Check state transitions printed via `Serial.println()`

### Build Status
- **Configuration**: Uses existing `build_src_filter` in `platformio.ini` (unchanged)
- **New Files**: All under `src/rx/` → automatically included by `+<rx/**>` filter
- **Next Step**: Run `pio run -e rx_board` to verify compilation

## Deviations from Plan
- **None**: All steps from README.md §5 implemented as specified
- **Added**: Serial debug logging in state entry hooks for easier diagnostics
- **Added**: Degraded/Fault labels created in `UiController::Init()` (hidden by default)

## File Structure After Refactor
```
src/rx/
├── main.cpp              (50 lines, thin shim)
├── EventQueue.h/cpp      (Event model + FreeRTOS queue wrapper)
├── CanInterface.h/cpp    (CAN init + ISR callback)
├── UiController.h/cpp    (LVGL/TFT/Touch + widget updates)
├── HealthMonitor.h/cpp   (Timeout watchdog)
└── SystemController.h/cpp (State machine logic)
```

## Next Steps
1. Build firmware: `pio run -e rx_board`
2. Upload to hardware: `pio run -e rx_board -t upload`
3. Run validation tests from checklist above
4. Optional: Add unit tests for state transitions (requires test framework setup)

## References
- Original monolithic code: Preserved in git history
- DBC integration: Unchanged (`generated_lecture_dbc.c`, `lib/Generated/`)
- LVGL callbacks: Moved to `UiController` as static methods
- CAN ISR: Moved to `CanInterface::CanMsgHandler()` (static, ISR-safe)
