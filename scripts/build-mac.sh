#!/bin/bash
# Granny Delay - macOS build script (Xcode command line tools + CMake)
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$PROJECT_DIR/build"
JUCE_PATH="$PROJECT_DIR/JUCE"
CONFIGURATION="Release"
CLEAN=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --juce-path) JUCE_PATH="$2"; shift 2 ;;
        --config) CONFIGURATION="$2"; shift 2 ;;
        --clean) CLEAN=true; shift ;;
        *) echo "Unknown option: $1"; exit 1 ;;
    esac
done

if [ ! -f "$JUCE_PATH/CMakeLists.txt" ]; then
    echo "ERROR: JUCE not found at $JUCE_PATH"
    echo "Run: git clone --depth 1 --branch 7.0.12 https://github.com/juce-framework/JUCE.git JUCE"
    exit 1
fi

[ "$CLEAN" = true ] && rm -rf "$BUILD_DIR"

cmake -S "$PROJECT_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$CONFIGURATION" -DJUCE_PATH="$JUCE_PATH"
cmake --build "$BUILD_DIR" --config "$CONFIGURATION" --parallel 4

echo "Done. Built plugins:"
find "$BUILD_DIR" -name "GrannyDelay.vst3" -o -name "GrannyDelay.component" | sed 's/^/  /'
