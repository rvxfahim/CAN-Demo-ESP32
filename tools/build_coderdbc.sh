#!/bin/bash
set -e

# Change to the directory of this script, then to c-coderdbc
cd "$(dirname "$0")/c-coderdbc"

echo "Building c-coderdbc..."
cmake -S src -B build
cmake --build build --config release

# Check if the executable was successfully built
if [ -f "build/coderdbc" ] || [ -f "build/coderdbc.exe" ]; then
    echo "✓ coderdbc built successfully!"
else
    echo "❌ Error: coderdbc executable not found in build directory."
    exit 1
fi
