#!/bin/bash
set -e

TOOLS_DIR="$(cd "$(dirname "$0")" && pwd)"
WORKSPACE_DIR="$(dirname "$TOOLS_DIR")"
DBC_FILE="$TOOLS_DIR/Lecture.dbc"
OUT_DIR="$WORKSPACE_DIR/lib/Generated"

CODERDBC_BIN="$TOOLS_DIR/c-coderdbc/build/coderdbc"
if [ ! -f "$CODERDBC_BIN" ] && [ -f "${CODERDBC_BIN}.exe" ]; then
    CODERDBC_BIN="${CODERDBC_BIN}.exe"
fi

if [ ! -f "$CODERDBC_BIN" ]; then
    echo "❌ Error: coderdbc executable not found. Please run build_coderdbc.sh first."
    exit 1
fi

echo "Running coderdbc on Lecture.dbc..."
cd "$TOOLS_DIR/c-coderdbc"
"$CODERDBC_BIN" -dbc "$DBC_FILE" -out "$OUT_DIR" -drvname lecture -rw -nodeutils

if [ $? -eq 0 ]; then
    echo "✓ Code generation successful! Files generated in $OUT_DIR"
else
    echo "❌ Code generation failed."
    exit 1
fi
