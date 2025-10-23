# CAN Lecture RX/TX – AI Agent Quick Guide

Purpose: Get an AI coding agent productive fast in this dual-target ESP32 CAN project. Keep RX and TX separated, respect build filters, and use the DBC-generated types for all CAN payloads.

## Targets and build
- Two PlatformIO envs in `platformio.ini` with strict filters:
  - `env:rx_board` builds `src/rx/main.cpp` with LVGL/TFT (excludes `src/tx/`)
  - `env:tx_board` builds `src/tx/main.cpp` only (no LVGL)
- Never merge mains. Shared code lives in `src/common/`. Use compile-time guards (RX_BOARD/TX_BOARD) defined in `build_flags`.
- Typical commands:
  - Build/Upload/Monitor RX: `pio run -e rx_board`, `pio run -e rx_board -t upload`, `pio device monitor -e rx_board`
  - Build/Upload/Monitor TX: same with `tx_board` (serial ports set in `platformio.ini`).

## Big picture (RX)
- Data flow: CAN ISR → EventQueue → SystemController → MessageRouter → {UiController, IOModule}; HealthMonitor polls router last-seen.
- State machine: Boot → DisplayInit → WaitingForData → Active ⇄ Degraded (stale) → Fault (unrecoverable). Recovery is automatic when frames resume.

## CAN and DBC
- DBC source: `tools/Lecture.dbc`. Generate `lib/Generated/lib/lecture.{c,h}` via `tools/c-coderdbc/build/coderdbc.exe` (readonly mode). Don’t edit generated files.
- Single include: `src/generated_lecture_dbc.c`. Use `Cluster_t`, `Pack_Cluster_lecture`, `Unpack_Cluster_lecture`.
- Driver: `lib/CanDriver/esp32_can.*` (pins 35/5, 500 kbps). RX uses mailbox filter `watchFor(0x65)`; fallback to general callback if needed.
- TX specifics: no LVGL; always `CAN0.setDebuggingMode(true)` and send `Cluster` using `Pack_Cluster_lecture`.

## MessageRouter (core pattern)
- `src/common/MessageRouter.{h,cpp}` provides pub/sub with sticky last value and `millis()` timestamp.
- Controller publishes `Cluster_t` on each frame; consumers subscribe in task context.
- Add new consumers by subscribing; add new messages by extending `CanInterface` events + router topics.

## UI and IO (RX only)
- UI stack: LVGL v9.1 + TFT_eSPI; SquareLine outputs in `lib/Ui/` (`ui.h`, `ui_Screen1.*`).
- Update widgets in subscribers: `ui_Arc1` for speed (map 12-bit to 0–240 arc), left/right labels via opacity 0/255. Call `lv_timer_handler()` in loop; `lv_tick_inc()` via tick hook.
- IO relays: `IOModule` drives left/right indicators with a 1 Hz blink state independent of bus rate; default OFF if stale. Pins in `include/IOPins.h`.

## Conventions and pitfalls
- Keep `build_src_filter` strict or you’ll link the wrong code. Place shared code under `src/common/` only.
- ISR stays minimal: validate frame, unpack via `Unpack_Cluster_lecture`, queue an event. Publish to router only from main/task context.
- Use DBC types exclusively—don’t invent frame structs.
- When extending: add EventQueue producer in `CanInterface`, a typed topic in `MessageRouter`, and subscribe in modules.

## Useful references
- Architecture: `ARCHITECTURE.md` and `README.md` (implemented design and flows)
- Generated DBC wrapper: `src/generated_lecture_dbc.c`
- LVGL config and display: `lib/lvgl_conf/src/lv_conf.h`, `include/TFTConfiguration.h`, `src/common/TFTConfiguration.cpp`
- CAN driver abstraction: `lib/CanDriver/`

Questions or gaps? Tell us what’s unclear (e.g., mailbox setup, UI widget names, blink timing), and we’ll refine this guide.

# REMOVED BELOW (superseded by concise guide)
 
