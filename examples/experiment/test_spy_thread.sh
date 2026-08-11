#!/bin/bash

# Test script for spy thread debugger
# This demonstrates the spy thread in action

echo "=== Spy Thread Debugger Test ==="
echo ""
echo "Starting spy_demo in background..."

# Start the demo in background
./spy_demo &
DEMO_PID=$!

# Wait for spy thread to initialize
sleep 2

echo "✓ Demo is running (PID: $DEMO_PID)"
echo ""
echo "Testing spy thread commands..."
echo ""

# Test 1: Get FPS
echo "[Test 1] Query FPS:"
echo "get fps" | nc localhost 9999
echo ""

# Test 2: Get all stats
echo "[Test 2] Query all stats:"
echo "status" | nc localhost 9999 | head -6
echo ""

# Test 3: List available commands
echo "[Test 3] List commands:"
echo "list_commands" | nc localhost 9999
echo ""

# Test 4: Pause and resume
echo "[Test 4] Pause/Resume test:"
echo "pause" | nc localhost 9999
sleep 1
echo "get frame_count" | nc localhost 9999
sleep 1
echo "resume" | nc localhost 9999
sleep 1
echo "get frame_count" | nc localhost 9999
echo ""

echo "Waiting for demo to complete (30 seconds total)..."
wait $DEMO_PID

echo ""
echo "=== Test Complete ==="
