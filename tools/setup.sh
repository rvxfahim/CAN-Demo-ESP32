#!/usr/bin/env bash
# Fresh-machine toolchain setup for macOS and Linux.
#
# This script installs/checks the host toolchain (Git, Python 3, CMake, a C++17
# compiler, pipx, and PlatformIO Core) and reports whether the c-coderdbc
# submodule and the generated files are present.
#
# It does NOT build c-coderdbc, generate lib/Generated/, or build the firmware.
# Students run tools/build_coderdbc.sh and tools/generate_code.sh themselves
# once the toolchain is ready.
#
# By default it only checks and prints install hints; pass --install-deps to
# let it install missing packages (may require sudo).
#
# Usage:
#   ./tools/setup.sh
#   ./tools/setup.sh --install-deps
#   ./tools/setup.sh --docs
set -uo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT" || exit 1

INSTALL_DEPS=0
BUILD_DOCS=0

usage() {
    cat <<'EOF'
Usage: ./tools/setup.sh [options]

Installs/checks the toolchain only. It does not build coderdbc or generate
lib/Generated/; use tools/build_coderdbc.sh and tools/generate_code.sh for that.

Options:
  --install-deps   Install missing system packages and PlatformIO Core (may use sudo).
  --docs           Build the Sphinx documentation in .venv-docs.
  -h, --help       Show this help.
EOF
}

for arg in "$@"; do
    case "$arg" in
        --install-deps) INSTALL_DEPS=1 ;;
        --docs) BUILD_DOCS=1 ;;
        -h|--help) usage; exit 0 ;;
        --check-only|--skip-build|--skip-generate|--regenerate)
            echo "Error: $arg is no longer supported." >&2
            echo "This script only installs/checks the toolchain. Generate the code with:" >&2
            echo "  ./tools/build_coderdbc.sh" >&2
            echo "  ./tools/generate_code.sh" >&2
            exit 2
            ;;
        *)
            echo "Unknown option: $arg" >&2
            usage
            exit 2
            ;;
    esac
done

have() { command -v "$1" >/dev/null 2>&1; }

add_path_if_exists() {
    [ -d "$1" ] || return 0
    case ":$PATH:" in
        *":$1:"*) ;;
        *) PATH="$1:$PATH"; export PATH ;;
    esac
}

# Tools installed by pipx, Homebrew, the VS Code extension, or winget may not be
# on the PATH inherited by this script (especially in a non-login shell).
add_known_paths() {
    add_path_if_exists "$HOME/.platformio/penv/bin"
    add_path_if_exists "$HOME/.platformio/penv/Scripts"
    add_path_if_exists "$HOME/.local/bin"
    add_path_if_exists "$HOME/AppData/Local/Microsoft/WindowsApps"
    add_path_if_exists "/opt/homebrew/bin"
    add_path_if_exists "/usr/local/bin"
    add_path_if_exists "/Applications/CMake.app/Contents/bin"
    add_path_if_exists "/c/Program Files/CMake/bin"
    add_path_if_exists "/snap/bin"
}

detect_python() {
    if have python3 && python3 --version >/dev/null 2>&1; then
        echo "python3"
    elif have python && python --version 2>&1 | grep -q 'Python 3'; then
        echo "python"
    elif have py && py --version >/dev/null 2>&1; then
        echo "py"
    fi
}

install_deps() {
    local sudo_cmd=""

    case "$OS_NAME" in
        Darwin)
            if ! xcode-select -p >/dev/null 2>&1; then
                echo "Xcode Command Line Tools are missing."
                echo "Running 'xcode-select --install'; complete the GUI installer, then rerun."
                xcode-select --install || true
                return 1
            fi
            if ! have brew; then
                echo "Homebrew is not installed. Install it from https://brew.sh and rerun with --install-deps."
                return 1
            fi
            brew install git python cmake pipx || return 1
            ;;
        Linux)
            if [ "$(id -u)" -eq 0 ]; then
                sudo_cmd=""
            elif have sudo; then
                sudo_cmd="sudo"
            else
                echo "Root or sudo is required to install system packages."
                return 1
            fi

            if have apt-get; then
                $sudo_cmd apt-get update || return 1
                $sudo_cmd env DEBIAN_FRONTEND=noninteractive apt-get install -y git python3 python3-venv build-essential cmake || return 1
                # pipx is not packaged on every release; the pip fallback below handles that.
                $sudo_cmd env DEBIAN_FRONTEND=noninteractive apt-get install -y pipx || true
            elif have dnf; then
                $sudo_cmd dnf install -y git python3 gcc-c++ make cmake || return 1
                $sudo_cmd dnf install -y pipx || true
            elif have pacman; then
                $sudo_cmd pacman -S --needed --noconfirm git python cmake base-devel || return 1
                $sudo_cmd pacman -S --needed --noconfirm python-pipx || true
            else
                echo "Unsupported Linux package manager. Install Git, Python 3, CMake, and a C++ compiler manually."
                return 1
            fi
            ;;
        *)
            echo "Unsupported OS for automatic dependency installation: $OS_NAME"
            return 1
            ;;
    esac

    add_known_paths
    local python_cmd
    python_cmd="$(detect_python)"
    if [ -z "$python_cmd" ]; then
        echo "Python was not found after installation. Open a new terminal and rerun."
        return 1
    fi

    if ! have pipx; then
        echo "Installing pipx..."
        "$python_cmd" -m pip install --user pipx || return 1
        add_known_paths
    fi

    if ! have pio; then
        echo "Installing PlatformIO Core with pipx..."
        if have pipx; then
            pipx install platformio || pipx upgrade platformio || return 1
        else
            "$python_cmd" -m pipx install platformio || "$python_cmd" -m pipx upgrade platformio || return 1
        fi
        add_known_paths
        if have pipx; then
            pipx ensurepath >/dev/null 2>&1 || true
        fi
    fi

    echo "Dependency installation finished."
    echo "Open a new terminal before running tools directly; this script re-probes known locations for its own run."
    return 0
}

ok()   { printf '  [ok]   %s\n' "$1"; }
miss() { printf '  [miss] %s\n' "$1"; }

missing_required=0
GENERATED_FILE="lib/Generated/lib/lecture.c"

OS_NAME="$(uname -s 2>/dev/null || echo unknown)"

# This script is for macOS and Linux. Windows has a PowerShell equivalent.
case "$OS_NAME" in
    MINGW*|MSYS*|CYGWIN*)
        echo "Windows detected. Use the PowerShell setup instead:"
        echo "  powershell -ExecutionPolicy Bypass -File .\\tools\\setup.ps1"
        echo "Generate code with tools\\build_coderdbc.ps1, then tools\\generate_code.ps1."
        exit 1
        ;;
esac

add_known_paths
PYTHON_CMD="$(detect_python)"

if [ "$INSTALL_DEPS" -eq 1 ]; then
    echo "Installing missing dependencies..."
    if ! install_deps; then
        echo "Dependency installation did not complete; continuing with checks." >&2
    fi
    add_known_paths
    PYTHON_CMD="$(detect_python)"
    echo
fi

echo "CAN-Demo-ESP32 setup check"
echo "Repository: $REPO_ROOT"
echo

echo "Required tools:"
if have git; then
    ok "git ($(git --version 2>/dev/null | head -n 1))"
else
    miss "git"; missing_required=1
fi

if have pio; then
    ok "PlatformIO Core ($(pio --version 2>/dev/null | head -n 1))"
else
    miss "PlatformIO Core CLI (pio)"; missing_required=1
fi

if [ -n "$PYTHON_CMD" ]; then
    ok "python ($($PYTHON_CMD --version 2>&1))"
else
    miss "Python 3 (needed for the PlatformIO CLI and docs)"
    # PlatformIO can be installed by the VS Code extension with its own Python;
    # only make this fatal when the CLI itself is missing too.
    if ! have pio; then
        missing_required=1
    fi
fi
echo

echo "Code generation toolchain (used by tools/build_coderdbc.sh):"
if have cmake; then
    ok "cmake ($(cmake --version 2>/dev/null | head -n 1))"
else
    miss "cmake"; missing_required=1
fi

if have c++ || have g++ || have clang++; then
    ok "C++ compiler"
else
    miss "C++ compiler (c++/g++/clang++)"; missing_required=1
fi
echo

echo "Project status:"
if [ -f "tools/c-coderdbc/src/CMakeLists.txt" ]; then
    ok "tools/c-coderdbc submodule"
else
    miss "tools/c-coderdbc submodule (tools/build_coderdbc.sh will initialize it)"
fi
if [ -f "$GENERATED_FILE" ]; then
    ok "generated code ($GENERATED_FILE)"
else
    miss "$GENERATED_FILE (run the student workflow below)"
fi
echo

if [ "$missing_required" -ne 0 ]; then
    echo "Install hints:"
    case "$OS_NAME" in
        Darwin)
            echo "  macOS:"
            echo "    xcode-select --install        # Git and a C++ compiler"
            echo "    brew install python cmake     # Python and CMake"
            echo "    python3 -m pip install --user pipx"
            echo "    python3 -m pipx ensurepath && python3 -m pipx install platformio"
            ;;
        Linux)
            echo "  Debian/Ubuntu:"
            echo "    sudo apt update && sudo apt install -y git python3 python3-venv build-essential cmake"
            echo "  Fedora:"
            echo "    sudo dnf install -y git python3 gcc-c++ make cmake"
            echo "  Arch:"
            echo "    sudo pacman -S --needed git python cmake base-devel"
            echo "  PlatformIO:"
            echo "    sudo apt install pipx   # or: python3 -m pip install --user pipx"
            echo "    pipx install platformio"
            ;;
        *)
            echo "  Install git, Python 3, CMake, a C++17 compiler, and PlatformIO Core for your platform."
            ;;
    esac
    echo "  Official PlatformIO install guide:"
    echo "    https://docs.platformio.org/en/latest/core/installation/index.html"
    echo "  The VS Code PlatformIO IDE extension is a valid alternative to the CLI."
    echo "  CMake and a C++17 compiler are required by tools/build_coderdbc.sh."
    echo "  Or rerun with: ./tools/setup.sh --install-deps"
    echo
fi

if have pio; then
    echo "Serial ports:"
    pio device list 2>/dev/null || true
    echo
fi

if [ "$OS_NAME" = "Linux" ] && have id; then
    if ! id -nG "$(id -un)" 2>/dev/null | tr ' ' '\n' | grep -qx dialout; then
        echo "Hint: add your user to the dialout group for upload access:"
        echo "  sudo usermod -aG dialout \"\$USER\"   # then log out and back in"
        echo
    fi
fi

cat <<'EOF'
Student workflow (run once the required tools above are available; this script
stops after toolchain setup):
  1. Build the DBC code generator:
       ./tools/build_coderdbc.sh
  2. Generate the CAN code from tools/Lecture.dbc:
       ./tools/generate_code.sh
  3. Implement the application layer using the generated API in
     lib/Generated/lib/lecture.h. On the TX board, complete
     SendClusterFrame() in src/tx/main.cpp (the lecture exercise).

See README.md ("Build, flash, and monitor") when your implementation is ready.
EOF

# ---------------------------------------------------------------------------
# Optional documentation build
# ---------------------------------------------------------------------------
if [ "$BUILD_DOCS" -eq 1 ]; then
    echo
    echo "Building documentation..."
    if [ -n "$PYTHON_CMD" ]; then
        if "$PYTHON_CMD" -m venv .venv-docs >/dev/null 2>&1; then
            .venv-docs/bin/python -m pip install --upgrade pip
            .venv-docs/bin/python -m pip install -r docs/requirements.txt
            .venv-docs/bin/python -m sphinx -b html docs docs/_build/html
            echo "Documentation written to docs/_build/html"
        else
            echo "Could not create .venv-docs. On Debian/Ubuntu install python3-venv." >&2
            exit 1
        fi
    else
        echo "Skipping documentation: python3 not found." >&2
    fi
fi

if [ "$missing_required" -ne 0 ]; then
    exit 1
fi
exit 0
