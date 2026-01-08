#!/bin/bash
# Quick rebuild and test script

set -e

echo "==================================="
echo "  SDL3CPlusPlus - Rebuild & Test"
echo "==================================="
echo ""

cd "$(dirname "$0")"
BUILD_DIR="build/Release"

# Build
echo "Building application..."
cd "$BUILD_DIR"
make -j$(nproc)
echo "✓ Build complete"
echo ""

# Test with config
echo "Starting application with seed_runtime.json..."
echo "Press Ctrl+C to exit, or wait for error messages"
echo "---"
./sdl3_app --json-file-in ./config/seed_runtime.json
