#!/bin/bash
# Test script for validation improvements

cd /home/rewrich/Documents/GitHub/SDL3CPlusPlus/build/Release

echo "=== Testing application startup with validation ==="
echo ""

# Test 1: Check --help output
echo "Test 1: Running with --help"
timeout 2 ./sdl3_app --help 2>&1 || echo "(timed out or exited with code $?)"
echo ""

# Test 2: Try with config file
echo "Test 2: Running with config file (5 second timeout)"
timeout 5 ./sdl3_app --json-file-in ./config/seed_runtime.json 2>&1 &
PID=$!
sleep 2

# Check if process is still running
if ps -p $PID > /dev/null 2>&1; then
    echo "Process started successfully (PID: $PID)"
    kill -9 $PID 2>/dev/null
    wait $PID 2>/dev/null
else
    echo "Process exited early"
    wait $PID 2>/dev/null
    echo "Exit code: $?"
fi
echo ""

# Test 3: Check for error in a non-existent config
echo "Test 3: Testing with non-existent config"
./sdl3_app --json-file-in ./config/nonexistent.json 2>&1
echo ""

echo "=== Tests complete ==="
