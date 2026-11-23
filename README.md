# CAN Bus Demo with ESP32, LVGL, and DBC Code Generation

A demonstration project showcasing **CAN bus communication** between two ESP32 boards with **LVGL GUI**, **DBC-based code generation**, and a **modular firmware architecture**. This project is ideal for learning CAN protocols, embedded UI development, and code generation workflows.

> **Lecture note:** the RX firmware is complete, but `src/tx/main.cpp` is deliberately an exercise: `SendClusterFrame()` is left as a TODO for students. Until it is implemented, the TX board sends empty frames and the RX board stays in `Waiting for CAN data` / `Degraded`.

---

<!-- Demo GIF: relative path so it renders both on GitHub and in the docs site -->
<p align="center">
   <img src="https://raw.githubusercontent.com/rvxfahim/CAN-Demo-ESP32/main/docs/_static/demo.gif" alt="CAN demo" />
</p>

## 🎯 What This Project Demonstrates

1. **CAN Bus Communication**  
   - Dual ESP32 setup: one **TX board** (transmitter) and one **RX board** (receiver)
   - 500 kbps CAN communication using ESP32's built-in CAN controller
   - Message filtering, unpacking, and event-driven processing

2. **DBC-Driven Code Generation**  
   - Define CAN messages in a standard `.dbc` file
   - Auto-generate C code for encoding/decoding using `c-coderdbc`
   - Type-safe message handling with generated structs

3. **LVGL Graphical UI (RX Board)**  
   - Real-time display of CAN data on TFT touchscreen
   - SquareLine Studio integration for UI design
   - Arc gauge for speed, turn signal indicators

4. **Modular Firmware Architecture**  
   - Event-driven state machine with message routing
   - Pub/sub pattern for decoupled components
   - Health monitoring with automatic degradation and recovery

---

## 🚀 Quick Start (fresh machine)

These instructions assume a **fresh machine**. The setup scripts install/check the host toolchain, but they do **not** build the DBC generator or generate code — students run `build_coderdbc` and `generate_code` themselves (step 3). PlatformIO downloads the ESP32 toolchain on the first build. You also need a host C++17 compiler and CMake, because **generating the CAN code from the DBC is a required part of the lab** — the generated files are intentionally not committed.

### What you need

**Hardware**

- 2x ESP32 boards (e.g., ESP32-EVB or similar)
- 2x CAN transceivers (e.g., SN65HVD230 or MCP2551), unless your board has one built in
- TFT display with touch for the RX board
- CAN bus wiring (CAN_H / CAN_L); termination is built into the boards
- USB cables for both boards

**Software**

| Tool | Needed for | Notes |
| --- | --- | --- |
| Git | Cloning the repository | Required |
| [PlatformIO](https://platformio.org/) Core CLI (`pio`) **or** the VS Code PlatformIO IDE extension | Build, upload, monitor | The commands below use the CLI |
| Python 3.9+ | Installing PlatformIO Core CLI; building docs | Bundled with the VS Code extension |
| CMake + a C++17 compiler | Building `coderdbc` and generating code | Required — the generated files are not committed |
| USB-UART driver | Windows only | Install the driver for your board's bridge chip (CP210x, FTDI, or CH340 are common) |

### 1. Install the tools

> **Shortcut — let the setup script install the toolchain (Linux/macOS):** the repository ships `tools/setup.sh`. Clone first (step 2), then run:
>
> ```bash
> ./tools/setup.sh --install-deps
> ```
>
> It installs the required packages (Git if missing, Python 3, CMake, and a C++17 compiler) with `apt`/`dnf`/`pacman` or Homebrew, installs `pipx` and PlatformIO Core, and reports whether the code-generator submodule and generated code are present. It does **not** build `coderdbc`, generate code, or build the firmware — those are the student steps in section 3. **If you use it, you can skip the manual installation steps in this section** — only the Linux serial-permissions step below still applies.
>
> Prerequisites for the script itself: on Linux, root or `sudo`; on macOS, [Homebrew](https://brew.sh/) plus the Xcode Command Line Tools (`xcode-select --install`, which may open a GUI installer). On Windows, after cloning, run `.\tools\setup.ps1 -InstallDeps` instead (it uses `winget` and may show UAC prompts).
>
> The manual steps below are for managed machines or when the script cannot install packages.

**Git** — install from [git-scm.com](https://git-scm.com/downloads). Verify with `git --version`.

**PlatformIO** — choose one:

- **VS Code extension (easiest):** install **PlatformIO IDE** from the VS Code marketplace. Build, upload, and monitor from the PlatformIO toolbar. The extension does not always put `pio` on your system PATH; if the commands below say `pio: command not found`, install the CLI as well or use the extension's PlatformIO terminal.
- **PlatformIO Core CLI:** install in an isolated environment with `pipx`:

  ```bash
  # macOS / Linux
  python3 -m pip install --user pipx
  python3 -m pipx ensurepath
  python3 -m pipx install platformio
  ```

  ```powershell
  # Windows (PowerShell)
  py -m pip install --user pipx
  py -m pipx ensurepath
  py -m pipx install platformio
  ```

  Open a new terminal afterward and verify with `pio --version`. If `pipx` is unavailable or `pip` is blocked by your OS package manager, use the official instructions at [docs.platformio.org/core/installation](https://docs.platformio.org/en/latest/core/installation/index.html).

**Required for code generation** (a host compiler, separate from the ESP32 cross-compiler PlatformIO downloads):

- **Windows:** install [CMake](https://cmake.org/download/) and **Visual Studio Build Tools** with the **Desktop development with C++** workload.
- **macOS:** run `xcode-select --install` for Git and a compiler; install `cmake` with Homebrew if needed (`brew install cmake`).
- **Linux (Debian/Ubuntu):**

  ```bash
  sudo apt update
  sudo apt install -y git python3 python3-venv build-essential cmake
  ```

  Fedora: `sudo dnf install -y git python3 gcc-c++ make cmake`. Arch: `sudo pacman -S --needed git python cmake base-devel`.

The setup script installs packages only when you pass `--install-deps` / `-InstallDeps`; without that flag it only checks the toolchain and prints install hints. Managed lab machines can therefore keep using the default.

**Linux serial permissions:** add yourself to the `dialout` group so you can upload without `sudo`, then log out and back in:

```bash
sudo usermod -aG dialout "$USER"
```

### 2. Clone the repository

```bash
git clone --recursive https://github.com/rvxfahim/CAN-Demo-ESP32.git
cd CAN-Demo-ESP32
```

`--recursive` is recommended because the DBC code generator lives in the `tools/c-coderdbc` submodule. If you cloned without the flag, `build_coderdbc` initializes it automatically; you can also run this manually:

```bash
git submodule update --init --recursive
```

Do not use GitHub's "Download ZIP" button: the ZIP does not contain the submodule, and the DBC generator cannot be built from it.

### 3. Generate the CAN code (required before building)

The setup scripts install/check the host toolchain — including CMake and a C++17 compiler — but they deliberately **do not** build `coderdbc`, generate code, or build the firmware. That is the student workflow in this step.

If you skipped the setup step, check or install the toolchain first:

- **macOS / Linux:**

  ```bash
  ./tools/setup.sh                  # check only; prints what is missing
  ./tools/setup.sh --install-deps   # fresh machine: install the toolchain
  ```

- **Windows (PowerShell):**

  ```powershell
  powershell -ExecutionPolicy Bypass -File .\tools\setup.ps1
  powershell -ExecutionPolicy Bypass -File .\tools\setup.ps1 -InstallDeps   # fresh machine
  ```

Setup options:

| macOS / Linux | Windows | Effect |
| --- | --- | --- |
| `--install-deps` | `-InstallDeps` | Install missing packages (apt/dnf/pacman, Homebrew, or winget), then pipx + PlatformIO. Opt-in; may use sudo/UAC |
| `--docs` | `-Docs` | Build the Sphinx documentation in `.venv-docs` |

`--install-deps` / `-InstallDeps` is opt-in because it may require `sudo`/UAC and can take several minutes (Visual Studio Build Tools especially). On Windows it uses `winget`; if `winget` is unavailable, or Homebrew is missing on macOS, the script prints manual instructions instead. On macOS, `xcode-select --install` may require completing a GUI installer. Package-manager PATH changes apply to new shells; the script re-probes common install locations, but open a new terminal if a tool still is not found.

Now build the generator and generate `lib/Generated/` from `tools/Lecture.dbc`:

- **macOS / Linux:**

  ```bash
  ./tools/build_coderdbc.sh
  ./tools/generate_code.sh
  ```

- **Windows (PowerShell):**

  ```powershell
  powershell -ExecutionPolicy Bypass -File .\tools\build_coderdbc.ps1
  powershell -ExecutionPolicy Bypass -File .\tools\generate_code.ps1
  ```

Run the two steps in order. The `build_coderdbc` script compiles the generator with CMake and initializes the `tools/c-coderdbc` submodule if it is missing; `generate_code` runs the generator and writes `lib/Generated/`. Re-run both after editing `tools/Lecture.dbc` (or just `generate_code` if only the DBC changed).

After generation, `lib/Generated/lib/lecture.h` provides the CAN types and the `Pack_*` / `Unpack_*` helpers. Use that API to implement the application layer — on the TX board this is the `SendClusterFrame()` exercise (see the lecture note at the top). When your implementation is ready, continue with [Build, flash, and monitor](#6-build-flash-and-monitor). The setup script does not build the firmware for you.

### 4. Wire the boards

1. Identify the **RX board** (with the display) and the **TX board**.
2. Connect each CAN transceiver: `VCC` to 3.3V, `GND` to `GND`, transceiver TXD (CTX) to **GPIO5**, transceiver RXD (CRX) to **GPIO35**. If your board has an onboard transceiver (e.g., ESP32-EVB), connect only CAN_H, CAN_L, and GND.
3. Connect **CAN_H ↔ CAN_H** and **CAN_L ↔ CAN_L** between the two transceivers.
4. Power both boards from USB. Use a common ground if the boards are separately powered.
5. The RX relay outputs are **GPIO32** (left) and **GPIO33** (right); see `include/IOPins.h`.

The TFT wiring depends on your display. The project-owned TFT_eSPI configuration is `include/tft_setup.h`; adjust it for your panel before building if necessary.

### 5. Find the serial ports

Connect **one board at a time** and run:

```bash
pio device list
```

Typical values:

- Windows: `COM8`, `COM9`
- macOS: `/dev/cu.usbserial-0001` (prefer `/dev/cu.*` over `/dev/tty.*`)
- Linux: `/dev/ttyUSB0`, `/dev/ttyACM0`

Pass the port explicitly on each command (shown below), or uncomment `upload_port` / `monitor_port` in `platformio.ini` and set your values. If both boards are connected at once, always pass explicit ports. PlatformIO's auto-detection is only reliable when one board is attached.

### 6. Build, flash, and monitor

Build both environments without uploading:

```bash
pio run -e rx_board -e tx_board
```

**RX board** (replace `COM8` with your RX port):

```bash
pio run -e rx_board -t upload --upload-port COM8
pio device monitor -p COM8 -b 115200
```

**TX board** (replace `COM9` with your TX port; use a second terminal):

```bash
pio run -e tx_board -t upload --upload-port COM9
pio device monitor -p COM9 -b 115200
```

Flash the RX board first, then the TX board. Keep each board's monitor open in its own terminal.

### 7. Verify

With a working TX implementation, the RX serial monitor shows:

```text
System booting...
Initializing display...
Waiting for CAN data...
System active
```

and the TX monitor should confirm that frames are being sent (the exact debug text depends on your implementation).

The RX display updates the speed arc and turn indicators. If frames stop for 1500 ms, the RX transitions to `Degraded` and recovers automatically when frames resume.

> Until the TX exercise is implemented (`SendClusterFrame()` in `src/tx/main.cpp`), the RX monitor stays in `Waiting for CAN data` and then `Degraded`. This is expected, not a wiring failure.

---

## 📖 Understanding the DBC Workflow

### What is a DBC file?

A **DBC** (CAN Database) file is an industry-standard format for defining CAN messages, signals, and their properties. It serves as the **single source of truth** for all CAN communication in this project.

**Location:** `tools/Lecture.dbc`

### The generated code is not committed

The generator's output lives under `lib/Generated/` and is **not committed** — it is listed in `.gitignore`. Generating it is part of the exercise, so a fresh clone must run the generator before building. Regenerate after every change to `tools/Lecture.dbc`.

The example `Lecture.dbc` defines:

- Message `Cluster`, ID `0x65` (101), DLC 3
- `speed` (12-bit): 0–4095
- `Left_Turn_Signal` (1-bit): boolean
- `Right_Turn_Signal` (1-bit): boolean

You can edit the DBC with any text editor or tools such as [CANdb++ Editor](https://vector.com/) or [SavvyCAN](https://www.savvycan.com/).

### Regenerating from the DBC

Requirements: **CMake** and a **C++17 compiler**; the `build_coderdbc` script fetches the `tools/c-coderdbc` submodule if it is missing. This is a host compiler, separate from the ESP32 cross-compiler that PlatformIO downloads.

**macOS / Linux:**

```bash
./tools/build_coderdbc.sh
./tools/generate_code.sh
```

**Windows (PowerShell):**

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\build_coderdbc.ps1
powershell -ExecutionPolicy Bypass -File .\tools\generate_code.ps1
```

Both `build_coderdbc` scripts initialize the `tools/c-coderdbc` submodule when it is missing. The default Visual Studio generator is a multi-config generator and writes `build\Release\coderdbc.exe`; single-config generators such as MinGW Makefiles or Ninja write `build\coderdbc.exe`. The `generate_code` scripts check both locations. The setup scripts do not run these steps.

This regenerates:

- `lib/Generated/lib/lecture.c` and `lecture.h` (generated code, **do not edit**)
- Helper files under `lib/Generated/conf/`, `lib/Generated/inc/`, `lib/Generated/butl/`, and `lib/Generated/usr/`

Do **not** commit `lib/Generated/`; it is ignored by Git. Commit only the DBC change.

### Using the generated API

The generated files are the reference for the exercise. After running the generator, read `lib/Generated/lib/lecture.h` for the message type and fields, the DLC/CAN ID constants, and the pack/unpack helper signatures; `lib/Generated/lib/lecture.c` shows their implementation. The field names come from `tools/Lecture.dbc`.

Use the generated pack/unpack helpers instead of encoding or decoding CAN payload bytes by hand.

---

## 🏗️ Project Structure

```text
CAN-Demo-ESP32/
├── platformio.ini           # Build config (two environments: rx_board, tx_board)
├── tools/
│   ├── Lecture.dbc          # CAN message definitions (source of truth)
│   ├── setup.sh             # macOS/Linux toolchain check/install (no code generation)
│   ├── setup.ps1            # Windows toolchain check/install (no code generation)
│   ├── build_coderdbc.sh    # Build the DBC generator (macOS/Linux)
│   ├── build_coderdbc.ps1   # Build the DBC generator (Windows)
│   ├── generate_code.sh     # Regenerate lib/Generated (macOS/Linux)
│   ├── generate_code.ps1    # Regenerate lib/Generated (Windows)
│   └── c-coderdbc/          # DBC-to-C code generator (git submodule)
├── lib/
│   ├── Generated/           # Generated locally from the DBC (gitignored; do not edit)
│   ├── CanDriver/           # ESP32 CAN driver abstraction
│   ├── Ui/                  # LVGL UI (SquareLine Studio exports)
│   └── TouchLibrary/        # Touch controller drivers
├── src/
│   ├── common/              # Shared code (MessageRouter, etc.)
│   ├── rx/                  # RX board firmware (complete)
│   ├── tx/                  # TX board firmware (student exercise)
│   └── generated_lecture_dbc.c  # Single include wrapper for DBC code
├── include/                 # Global headers (IOPins, TFT config)
├── docs/                    # Sphinx documentation source
└── README.md                # Single source for the Getting Started page
```

The documentation page `docs/getting-started.md` includes this README directly, so the two cannot drift apart. Keep setup instructions here, not in a second copy.

---

## 🔧 Configuration and Customization

### Serial ports

`platformio.ini` ships with the port settings commented out so PlatformIO can auto-detect a single connected board. For a two-board setup, either uncomment and set the values or pass `--upload-port` / `-p` on the command line.

### Build platform

`platformio.ini` pins **`espressif32@6.5.0`**. Newer espressif32 releases (8.x, Arduino core 3.x / ESP-IDF 5) change the SPI API and fail to compile the legacy `lib/CanDriver/mcp2517fd.cpp`. Keep the pin for reproducible fresh builds unless you update that driver.

### CAN parameters

- **Bitrate:** 500 kbps (configurable in `CanInterface.cpp` and `src/tx/main.cpp`)
- **Pins:** GPIO 35 (RX), GPIO 5 (TX) — configured in `CanInterface::Initialize()` and `src/tx/main.cpp`
- **Message ID:** `0x65` for `Cluster` (defined in the DBC)

### Display and UI

- **Screen:** configured in `include/TFTConfiguration.h` and `lib/Ui/`
- **TFT_eSPI setup:** edit `include/tft_setup.h` (project-owned, persistent configuration). Do **not** edit `.pio/libdeps/.../TFT_eSPI/User_Setup*.h`; PlatformIO reinstalls those files.
- **UI design:** edit with [SquareLine Studio](https://squareline.io/), export to `lib/Ui/`
- **Widgets:** arc gauge (`ui_Arc1`), labels (`ui_RightLabel`, `ui_LeftLabel`)

### IO relays (blinkers)

- **Pins:** GPIO 32 (left), GPIO 33 (right) — see `include/IOPins.h`
- **Blink rate:** 1 Hz (500 ms on/off) — adjust in `IOModule::Update()`

### Health monitoring

- **Timeout:** 1500 ms. The RX declares `Degraded` if no valid `Cluster` frames arrive; see `HealthMonitor.cpp`.

---

## 🧪 Troubleshooting

### `pio: command not found`

- With only the VS Code extension, `pio` may not be on the system PATH. Use the PlatformIO toolbar, the extension's PlatformIO terminal, or install PlatformIO Core CLI.
- PlatformIO's per-user install lives under `~/.platformio/penv/bin` (macOS/Linux) or `%USERPROFILE%\.platformio\penv\Scripts` (Windows). The setup scripts add it to PATH for their own run.

### Build fails with `lecture.h` or `lecture.c` not found

The generated code under `lib/Generated/` is missing. Build the generator and generate it:

```bash
./tools/build_coderdbc.sh
./tools/generate_code.sh
```

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\build_coderdbc.ps1
powershell -ExecutionPolicy Bypass -File .\tools\generate_code.ps1
```

This is a compile-time missing-file error, not a linker error.

### `cmake: command not found` or `No CMAKE_CXX_COMPILER could be found`

CMake and a C++17 compiler are required to build `coderdbc` and generate `lib/Generated/`. Install them per the instructions above (or run `./tools/setup.sh --install-deps` / `.\tools\setup.ps1 -InstallDeps`), then rerun `build_coderdbc` and `generate_code`.

### Fresh build fails in `lib/CanDriver/mcp2517fd.cpp`

Errors such as `invalid conversion from 'int' to 'spi_t*'` or `too few arguments to function 'spiFrequencyToClockDiv(spi_t*, uint32_t)'` mean PlatformIO resolved a newer `espressif32` platform (8.x). Confirm both environments in `platformio.ini` say `platform = espressif32@6.5.0`, then rebuild. If a stale `.pio` cache still references 8.x, delete it and rerun:

```bash
# macOS / Linux
rm -rf .pio
pio run -e rx_board -e tx_board
```

```powershell
# Windows PowerShell
Remove-Item -Recurse -Force .pio
pio run -e rx_board -e tx_board
```

### `coderdbc executable not found` after a successful CMake build

The generator has not been built, or CMake placed it outside the paths the helper scripts check. Run `tools\build_coderdbc.ps1` (Windows) or `./tools/build_coderdbc.sh` (macOS/Linux) first; `generate_code` then finds both `build\coderdbc.exe` and `build\<Config>\coderdbc.exe`.

### Upload fails or the port cannot be opened

- Run `pio device list` and confirm the port. On Windows check Device Manager; install the USB-UART driver if the board does not appear.
- Close any other serial monitor that is holding the port.
- On Linux, add your user to `dialout` and log out/in.
- If both boards are connected, pass explicit `--upload-port` / `-p` values; auto-detection may choose the wrong board.
- On Linux/macOS, use the `/dev/cu.*` or `/dev/ttyUSB*`/`/dev/ttyACM*` device that appears when the board is plugged in.

### RX display does not update

- Confirm the TX board is running an implemented `SendClusterFrame()` (see the lecture note at the top).
- Check the TX serial monitor for confirmation that frames are being sent.
- Check CAN_H/CAN_L wiring and common ground.
- Confirm both boards use 500 kbps.

### Blinkers do not work

- Verify the relay pins: GPIO 32 (left) and GPIO 33 (right) in `include/IOPins.h`.
- Verify the TX sets `Left_Turn_Signal` / `Right_Turn_Signal` in the packed message.

### System reports "Degraded"

Expected when no valid frames arrive for 1500 ms. It recovers automatically when the TX resumes. If it never leaves `Degraded`, start with the TX exercise/wiring checks above.

### Documentation build

```bash
python3 -m venv .venv-docs
.venv-docs/bin/python -m pip install -r docs/requirements.txt
.venv-docs/bin/python -m sphinx -b html docs docs/_build/html
```

On Windows use `.venv-docs\Scripts\python.exe`. Doxygen is optional; the Sphinx build still succeeds without it. `./tools/setup.sh --docs` (or `.\tools\setup.ps1 -Docs`) does the same thing.

---

## 📚 Documentation

For **detailed architecture, sequence diagrams, and API references**, see the full documentation:

### 🌐 [**GitHub Pages Documentation**](https://rvxfahim.github.io/CAN-Demo-ESP32/)

Topics covered:

- **Getting Started:** this README (setup, hardware, build, DBC workflow)
- **Architecture:** component diagrams, state machines, data flow
- **CAN & DBC:** in-depth DBC workflow and regeneration steps
- **Testing:** unit test guidelines and test hooks
- **API Reference:** Doxygen-generated class/function documentation

---

## 🤝 Contributing

Contributions welcome! Please:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

---

## 📄 License

This project is licensed under the GPLv3 License. See the `LICENSE` file for details.

---

## 🔗 Key Technologies

- **[PlatformIO](https://platformio.org/)** — cross-platform embedded build system
- **[LVGL v9.1](https://lvgl.io/)** — lightweight graphics library
- **[TFT_eSPI](https://github.com/Bodmer/TFT_eSPI)** — fast TFT display driver
- **[SquareLine Studio](https://squareline.io/)** — drag-and-drop LVGL UI designer
- **[c-coderdbc](https://github.com/astand/c-coderdbc)** — DBC-to-C code generator
- **ESP32 Arduino** — Arduino framework for ESP32

---

## 📞 Questions and Support

- **Documentation:** https://rvxfahim.github.io/CAN-Demo-ESP32/
- **Issues:** [GitHub Issues](https://github.com/rvxfahim/CAN-Demo-ESP32/issues)
- **Repository:** [github.com/rvxfahim/CAN-Demo-ESP32](https://github.com/rvxfahim/CAN-Demo-ESP32)
