#!/usr/bin/env bash
# Generates lib/Generated/ from tools/Lecture.dbc using the coderdbc executable.
#
# Run tools/build_coderdbc.sh first. This script initializes the
# tools/c-coderdbc submodule when it is missing.
#
# Usage:
#   ./tools/generate_code.sh
set -euo pipefail

TOOLS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(dirname "$TOOLS_DIR")"
SUBMODULE_DIR="$TOOLS_DIR/c-coderdbc"
DBC_FILE="$TOOLS_DIR/Lecture.dbc"
OUT_DIR="$REPO_ROOT/lib/Generated"

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

if [ ! -f "$DBC_FILE" ]; then
    echo "Error: $DBC_FILE is missing." >&2
    exit 1
fi

ensure_submodule

# Support both single-config and multi-config CMake generators.
CODERDBC_BIN=""
for candidate in \
    "$SUBMODULE_DIR/build/coderdbc" \
    "$SUBMODULE_DIR/build/coderdbc.exe" \
    "$SUBMODULE_DIR/build/Release/coderdbc" \
    "$SUBMODULE_DIR/build/Release/coderdbc.exe" \
    "$SUBMODULE_DIR/build/Debug/coderdbc" \
    "$SUBMODULE_DIR/build/Debug/coderdbc.exe"; do
    if [ -f "$candidate" ]; then
        CODERDBC_BIN="$candidate"
        break
    fi
done

if [ -z "$CODERDBC_BIN" ]; then
    echo "Error: coderdbc executable not found under tools/c-coderdbc/build/." >&2
    echo "Run ./tools/build_coderdbc.sh first." >&2
    exit 1
fi

echo "Running coderdbc on Lecture.dbc..."
cd "$SUBMODULE_DIR"
"$CODERDBC_BIN" -dbc "$DBC_FILE" -out "$OUT_DIR" -drvname lecture -rw -nodeutils

if [ ! -f "$OUT_DIR/lib/lecture.c" ]; then
    echo "Error: code generation reported success but $OUT_DIR/lib/lecture.c is missing." >&2
    exit 1
fi

echo "Code generation successful. Files written to $OUT_DIR"
