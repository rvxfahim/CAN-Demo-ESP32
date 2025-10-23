# CAN and DBC

This project uses a DBC-driven workflow to encode/decode CAN frames consistently across the RX and TX targets.

## Sources and generated code

- DBC source: `tools/Lecture.dbc`
- Generated C wrapper (do not edit): `lib/Generated/lib/lecture.{c,h}`
- Single include in the project: `src/generated_lecture_dbc.c` (pulls in the generated library)

Driver abstraction lives under `lib/CanDriver/` and supports the ESP32 internal CAN controller and common external controllers.

## Regenerating from DBC (Windows)

1. Ensure the DBC is saved at `tools/Lecture.dbc`.
2. Run the generator (Windows only, provided in repo):

   - Tool: `tools/c-coderdbc/build/coderdbc.exe`
   - Output folder (autodetected by script): `lib/Generated/` (subfolders are managed by the generator)

3. Commit the updated files under `lib/Generated/`.

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
