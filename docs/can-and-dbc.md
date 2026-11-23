# CAN and DBC

This project uses a DBC-driven workflow to encode/decode CAN frames consistently across the RX and TX targets.

## Sources and generated code

- DBC source: `tools/Lecture.dbc`
- Generated C driver (do not edit): `lib/Generated/lib/lecture.{c,h}`
- Single include in the project: `src/generated_lecture_dbc.c` (pulls in the generated library)

Note: the generator produces a driver named `lecture` (that's the `-drvname` value used here). Include
`lib/Generated/lib/lecture.h` in your code to get the generated types and helper functions (for example
`Cluster_t`, `Pack_Cluster_lecture(...)`, `Unpack_Cluster_lecture(...)`). The project also provides
`src/generated_lecture_dbc.c` which includes the generated C file directly for the build.

Driver abstraction lives under `lib/CanDriver/` and supports the ESP32 internal CAN controller and common external controllers.

## Regenerating from DBC

`lib/Generated/` is **not committed**; it is generated locally and listed in `.gitignore`. Generate it after cloning and regenerate after every change to `tools/Lecture.dbc` — this is part of the exercise.

The full per-OS instructions live in the Getting Started page (the README is the single source of truth): https://rvxfahim.github.io/CAN-Demo-ESP32/getting-started.html

Quick reference:

- macOS/Linux: `./tools/build_coderdbc.sh`, then `./tools/generate_code.sh`
- Windows: `powershell -ExecutionPolicy Bypass -File .\tools\build_coderdbc.ps1`, then `powershell -ExecutionPolicy Bypass -File .\tools\generate_code.ps1`
- The setup scripts (`setup.sh` / `setup.ps1`) only install/check the toolchain; they do not build the generator or generate code.
- Requirements: CMake and a C++17 compiler.
- The `build_coderdbc` scripts initialize `tools/c-coderdbc` when needed, and the `generate_code` scripts find the generator in both `build/` and `build/<Config>/` (Visual Studio writes `build\Release\coderdbc.exe`).

Do **not** commit `lib/Generated/`; commit only the DBC change.

The code should always use the generated types and pack/unpack helpers:

- Type: `Cluster_t`
- Pack: `Pack_Cluster_lecture(...)`
- Unpack: `Unpack_Cluster_lecture(...)`

## RX mailbox and topics

- RX config: mailbox filter listening to ID `0x65` via `CAN0.watchFor(0x65)`
- Data flow: ISR → EventQueue → SystemController → MessageRouter → UI/IOModule
- Router publishes `Cluster_t` with sticky last value and a millis() timestamp. Consumers subscribe to receive updates.

## TX specifics

- No LVGL; keep `CAN0.setDebuggingMode(true)` during development
- Send `Cluster_t` using `Pack_Cluster_lecture` on each cycle

See also `ARCHITECTURE.md` for the broader system overview and `include/TFTConfiguration.h` for display configuration (RX only).
