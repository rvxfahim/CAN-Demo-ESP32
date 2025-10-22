# CAN Lecture RX/TX Firmware - AI Coding Instructions

## Project Overview
Dual-target ESP32 firmware for CAN bus education: **RX board** (receiver with LVGL GUI) and **TX board** (transmitter). Built with PlatformIO, uses DBC-generated code for CAN message encoding/decoding.

## Architecture

### Dual-Board Build System
- **Critical**: Two separate environments in `platformio.ini`:
  - `env:rx_board` - Builds `src/rx/main.cpp` + LVGL/TFT stack (excludes `src/tx/`)
  - `env:tx_board` - Builds `src/tx/main.cpp` only (excludes `src/rx/` and TFT libs)
- **Shared code**: `src/common/` and `src/generated_lecture_dbc.c` included in both
- Use `build_src_filter` to control which sources compile per board - never remove filters

### CAN Message Flow
1. **DBC source of truth**: `tools/Lecture.dbc` defines `Cluster` message (ID 0x65, 3 bytes)
   - Signals: `speed` (12-bit), `Left_Turn_Signal` (1-bit), `Right_Turn_Signal` (1-bit)
2. **Code generation**: Run `coderdbc` (in `tools/c-coderdbc/`) to regenerate `lib/Generated/lib/lecture.{c,h}` from DBC
3. **Single include**: `src/generated_lecture_dbc.c` wraps the generated code - DO NOT modify generated files directly
4. **TX side**: Calls `Pack_Cluster_lecture()` to serialize `Cluster_t` struct into CAN frame bytes
5. **RX side**: ISR callback `CanMsgHandler()` validates frame ID/DLC/IDE, then `Unpack_Cluster_lecture()` deserializes into `Cluster_t`

### RX Board State Machine (Planned Refactor - See README.md)
Current implementation is monolithic; refactoring to modular state machine:
- **States**: Boot → DisplayInit → WaitingForData → Active ⇄ Degraded ⇄ Fault
- **Event-driven**: FreeRTOS queue (`clusterQueue`) bridges CAN ISR to main loop
- **Modules** (target architecture):
  - `CanInterface` - CAN setup, ISR, DBC unpacking
  - `UiController` - LVGL/TFT/Touch init + widget updates
  - `HealthMonitor` - Timeout detection for stale data
  - `SystemController` - State transitions based on events
  - `EventQueue` - Typed events (InitOk/Fail, ClusterFrame, FrameTimeout, Error)

### GUI Architecture (RX Only)
- **Tool**: SquareLine Studio project in `GUI_src/` generates `lib/Ui/*.{c,h}`
- **Key files**: `ui.h` (entry point), `ui_Screen1.{c,h}` (dashboard screen)
- **Integration**: `src/rx/main.cpp` calls `ui_init()`, then updates widgets (`ui_Arc1`, `ui_LeftTurnLabel`, `ui_RightTurnLabel`) in `UpdateClusterUi()`
- **Display driver**: LVGL v9.1 with TFT_eSPI backend, custom flush/touch callbacks (`my_disp_flush`, `my_touchpad_read`)
- **Configuration**: `lib/lvgl_conf/src/lv_conf.h` (LVGL settings), `include/TFTConfiguration.h` + `src/common/TFTConfiguration.cpp` (hardware setup)

## Critical Conventions

### File Organization
- **Per-board mains**: `src/rx/main.cpp` and `src/tx/main.cpp` - NEVER merge
- **Shared utilities**: Place in `src/common/` (currently has `TFTConfiguration.cpp`)
- **Custom libs**: `lib/CanDriver/` (CAN abstraction), `lib/Generated/` (DBC output), `lib/Ui/` (GUI), `lib/TouchLibrary/` (NS2009 driver)
- **Build guards**: Use `#ifdef RX_BOARD` / `#ifdef TX_BOARD` (defined in `build_flags`) for conditional compilation in shared files

### CAN Driver Usage (esp32_can library)
```cpp
// Standard initialization pattern
CAN0.setCANPins(GPIO_NUM_35, GPIO_NUM_5);  // Fixed pins for this hardware
CAN0.begin(500000);  // 500 kbps bus speed

// RX: Use mailbox filtering when possible
int mb = CAN0.watchFor(Cluster_CANID);
if (mb >= 0) {
    CAN0.setCallback(mb, CanMsgHandler);
} else {
    CAN0.setGeneralCallback(CanMsgHandler);  // Fallback
}

// TX: Always enable debugging for diagnostics
CAN0.setDebuggingMode(true);
```

### LVGL/UI Update Pattern
- **Opacity toggling**: Turn indicators use `lv_obj_set_style_text_opa()` with `kOpacityOn` (255) / `kOpacityOff` (0)
- **Arc scaling**: Speed gauge uses `lv_arc_set_value()` with custom mapping (`ConvertSpeedToArcValue()` scales 12-bit speed to 0-240 arc range)
- **Main loop**: Call `lv_timer_handler()` regularly (currently every 5ms) + `lv_tick_inc()` via callback
- **Null checks**: Always verify widget pointers (e.g., `ui_Arc1 != nullptr`) before updating - SquareLine may regenerate

### DBC Workflow
1. Edit `tools/Lecture.dbc` in CANdb++ or text editor
2. Navigate to `tools/c-coderdbc/build/`
3. Run: `coderdbc.exe --dbc ../../Lecture.dbc --out ../../.. --drvname lecture --gen r`
   - `--out` path is relative to `c-coderdbc/build/`, targets repo root
   - `--gen r` = readonly mode (overwrites lib files, preserves config)
4. Verify `lib/Generated/lib/lecture.{c,h}` updated
5. Rebuild both environments to catch API changes

## Developer Workflows

### Build & Upload
```bash
# Build specific board
pio run -e rx_board
pio run -e tx_board

# Upload (serial ports in platformio.ini: COM8=RX, COM9=TX)
pio run -e rx_board -t upload
pio run -e tx_board -t upload

# Monitor serial output (TX board has verbose logging)
pio device monitor -p COM9 -b 115200
```

### Debugging Tips
- **TX diagnostics**: Check `CheckCANStatus()` output every 1s - warns about BUS-OFF (no ACK), error passive, queue fullness
- **RX stale data**: Future `HealthMonitor` will emit warnings if no `Cluster` frame for >500ms
- **LVGL issues**: Enable `LV_USE_LOG` in `lv_conf.h`, redirect to Serial in `lv_log_register_print_cb()`
- **CAN ISR**: Keep `CanMsgHandler()` minimal - validate, unpack, queue. Heavy processing in `loop()`.

### Common Pitfall: Build Filters
**Problem**: Adding new files without updating `build_src_filter` causes link errors or code not executing.  
**Solution**: Explicitly include parent directories in filters if adding subfolders under `src/`.

## Integration Points

### External Dependencies
- **PlatformIO libs**: `lvgl/lvgl@9.1.0`, `bodmer/TFT_eSPI@^2.5.34` (RX only)
- **Arduino ESP32 framework**: Provides `Wire`, `millis()`, `Serial`, FreeRTOS wrappers
- **ESP-IDF headers**: Located at `~/.platformio/packages/framework-arduinoespressif32/tools/sdk/esp32/include/` (see workspace folder)
  - Use for low-level TWAI (CAN controller) access: `#include "driver/twai.h"`

### Cross-Component Communication
- **CAN ISR → Main Loop**: FreeRTOS queue (`clusterQueue`) with `xQueueOverwrite()` in ISR, `xQueueReceive()` in loop
- **DBC Structs**: Always use `Cluster_t` from `lecture.h` - don't create custom frame formats
- **GUI → Application**: Currently one-way (app updates GUI). For user input (future), handle events in `ui_events.h` hooks

## Active Development
- **Current task**: Refactoring RX firmware to state-machine architecture per `README.md` plan
- **Key files to modify**: `src/rx/main.cpp` (split into modules), add `SystemController.{cpp,h}`, `EventQueue.{cpp,h}`, etc.
- **Preserve**: CAN callback signature, LVGL display/input driver interfaces, DBC integration
- **Testing criteria**: Boot to Active state, Degraded warning on CAN loss, recovery without reboot

## References
- DBC code generator docs: `tools/c-coderdbc/README.md`
- LVGL v9 API: https://docs.lvgl.io/9.1/
- ESP32 CAN library: Check `lib/CanDriver/esp32_can.h` for API (custom wrapper, not official Espressif)
