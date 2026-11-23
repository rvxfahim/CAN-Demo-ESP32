#!/usr/bin/env bash
# Builds the c-coderdbc DBC code generator.
#
# Initializes the tools/c-coderdbc git submodule when it is missing, then runs
# CMake. Run tools/generate_code.sh afterwards to generate lib/Generated/.
#
# Usage:
#   ./tools/build_coderdbc.sh
set -euo pipefail

TOOLS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(dirname "$TOOLS_DIR")"
SUBMODULE_DIR="$TOOLS_DIR/c-coderdbc"

ensure_submodule() {
    if [ -f "$SUBMODULE_DIR/src/CMakeLists.txt" ]; then
        return 0
    fi

    echo "tools/c-coderdbc is missing; initializing the git submodule..."
    if ! command -v git >/dev/null 2>&1; then
        echo "Error: git is not installed, so the c-coderdbc submodule cannot be initialized." >&2
        echo "Install Git and rerun, or clone the repository with --recursive." >&2
        exit 1
    fi
    if [ ! -e "$REPO_ROOT/.git" ]; then
        echo "Error: this does not look like a git checkout, so the submodule cannot be initialized." >&2
        echo "Clone the repository with: git clone --recursive https://github.com/rvxfahim/CAN-Demo-ESP32.git" >&2
        exit 1
    fi
    if ! git -C "$REPO_ROOT" submodule update --init --recursive; then
        echo "Error: failed to initialize tools/c-coderdbc." >&2
        echo "Check your network connection, or run: git submodule update --init --recursive" >&2
        exit 1
    fi
    if [ ! -f "$SUBMODULE_DIR/src/CMakeLists.txt" ]; then
        echo "Error: tools/c-coderdbc is still empty after 'git submodule update'." >&2
        exit 1
    fi
}

if ! command -v cmake >/dev/null 2>&1; then
    echo "Error: cmake not found." >&2
    echo "Run ./tools/setup.sh --install-deps, or install CMake and a C++17 compiler." >&2
    exit 1
fi

ensure_submodule

echo "Building c-coderdbc..."
cd "$SUBMODULE_DIR"
cmake -S src -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

# Different CMake generators put the binary in different places:
#   - Single-config (Makefiles, Ninja, MinGW Makefiles): build/coderdbc[.exe]
#   - Multi-config (Visual Studio, Xcode):              build/<Config>/coderdbc[.exe]
CODERDBC_BIN=""
for candidate in \
    build/coderdbc \
    build/coderdbc.exe \
    build/Release/coderdbc \
    build/Release/coderdbc.exe \
    build/Debug/coderdbc \
    build/Debug/coderdbc.exe; do
    if [ -f "$candidate" ]; then
        CODERDBC_BIN="$candidate"
        break
    fi
done

if [ -n "$CODERDBC_BIN" ]; then
    echo "coderdbc built successfully: $CODERDBC_BIN"
else
    echo "Error: coderdbc executable not found under build/." >&2
    echo "Expected build/coderdbc[.exe] or build/<Config>/coderdbc[.exe]." >&2
    exit 1
fi
