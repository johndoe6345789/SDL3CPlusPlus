#!/bin/bash
# Launch script for standalone_cubes demo
# Run this from a terminal with display access (not SSH)

cd "$(dirname "$0")"

echo "=========================================="
echo "bgfx Standalone Cubes Demo"
echo "=========================================="
echo ""
echo "Controls:"
echo "  ESC - Exit"
echo ""
echo "Rendering 121 rotating colored cubes (11x11 grid)"
echo "Window: 1280x720"
echo "Renderer: Metal (macOS native)"
echo ""
echo "Starting..."
echo ""

./build/standalone_cubes

EXIT_CODE=$?

if [ $EXIT_CODE -ne 0 ]; then
    echo ""
    echo "=========================================="
    echo "Error: Demo exited with code $EXIT_CODE"
    echo "=========================================="
    echo ""
    echo "If you see 'bgfx::init failed', make sure you're running"
    echo "this from a terminal with display access (not SSH)."
    echo ""
    echo "On macOS, open Terminal.app and run:"
    echo "  cd $(pwd)"
    echo "  ./run_cubes.sh"
    echo ""
fi

exit $EXIT_CODE
