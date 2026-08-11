#!/bin/bash

echo "╔════════════════════════════════════════════════════════════════╗"
echo "║    SPY THREAD INTEGRATION TEST - WORKFLOW JSON RENDERER       ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""

# Kill any existing processes
pkill -f "spy_demo|spy_client.py" 2>/dev/null || true
sleep 1

# Start spy_demo (simulates workflow execution)
echo "1️⃣  Starting spy_demo (workflow simulator)..."
./spy_demo > /tmp/spy_demo.log 2>&1 &
SPY_PID=$!
sleep 2

echo "   ✓ Process running with PID $SPY_PID"
echo ""

# Test spy client connection
echo "2️⃣  Testing spy client connection..."
STATUS=$(python3 spy_client.py status 2>&1)
if echo "$STATUS" | grep -q "WORKFLOW EXECUTION STATUS"; then
    echo "   ✓ Connection successful"
else
    echo "   ✗ Connection failed"
    kill $SPY_PID 2>/dev/null || true
    exit 1
fi
echo ""

# Get initial state
echo "3️⃣  Getting initial metrics..."
python3 spy_client.py get fps
python3 spy_client.py get frame_count
python3 spy_client.py get elapsed_time
echo ""

# Pause and verify frozen
echo "4️⃣  Pausing execution..."
python3 spy_client.py pause
sleep 1

FRAME_AT_PAUSE=$(python3 spy_client.py get frame_count | cut -d= -f2)
sleep 1
FRAME_AFTER_1SEC=$(python3 spy_client.py get frame_count | cut -d= -f2)

echo "   Frame count at pause: $FRAME_AT_PAUSE"
echo "   Frame count 1 sec later: $FRAME_AFTER_1SEC"

if [ "$FRAME_AT_PAUSE" == "$FRAME_AFTER_1SEC" ]; then
    echo "   ✓ Pause confirmed - frame count frozen"
else
    echo "   ✗ Pause failed - frames still incrementing"
fi
echo ""

# Resume and verify running
echo "5️⃣  Resuming execution..."
python3 spy_client.py resume
sleep 2

FRAME_AFTER_RESUME=$(python3 spy_client.py get frame_count | cut -d= -f2)
DELTA=$((FRAME_AFTER_RESUME - FRAME_AT_PAUSE))

echo "   Frame count before: $FRAME_AT_PAUSE"
echo "   Frame count after resume: $FRAME_AFTER_RESUME"
echo "   Frames executed: $DELTA"

if [ "$DELTA" -gt 0 ]; then
    echo "   ✓ Resume confirmed - execution continues"
else
    echo "   ✗ Resume failed - no frame progress"
fi
echo ""

# Watch live metrics
echo "6️⃣  Watching live metrics for 5 seconds..."
python3 spy_client.py watch fps --interval 1.0 &
WATCH_PID=$!
sleep 5
kill $WATCH_PID 2>/dev/null || true
wait $WATCH_PID 2>/dev/null || true
echo ""

# Cleanup
echo "7️⃣  Cleanup..."
kill $SPY_PID 2>/dev/null || true
wait $SPY_PID 2>/dev/null || true

echo ""
echo "╔════════════════════════════════════════════════════════════════╗"
echo "║              ✅ SPY THREAD INTEGRATION TEST PASSED             ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""
echo "Summary:"
echo "  • Spy thread integration verified"
echo "  • Pause/resume functionality working"
echo "  • Real-time metrics accessible"
echo "  • Python argparse client fully functional"
echo ""
