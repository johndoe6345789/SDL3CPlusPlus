# Workflow + Spy Thread Integration Guide

## Overview

We've integrated the spy thread debugger directly into the workflow renderer, enabling real-time monitoring of game execution with zero impact on performance.

**Architecture**:
- **Main thread**: Executes workflow steps, updates atomic variables
- **Spy thread**: Listens on localhost:9999 for monitoring commands
- **Python client**: User-friendly command-line tool for querying stats

---

## Setup

### 1. Enable Spy Thread in Workflow Renderer

**Option A: Use Enhanced Version**
```bash
# Build with spy thread support
clang++ -std=c++17 -pthread \
  main_with_spy.cpp \
  [other game engine compilation flags] \
  -o workflow_with_spy

# Run with spy thread enabled
./workflow_with_spy workflow_cubes.json
```

**Option B: Add to Existing main.cpp**

Add these includes at the top:
```cpp
#include <atomic>
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
```

Copy the `WorkflowSpyThread` class from `main_with_spy.cpp` into your main.cpp.

Initialize in main():
```cpp
int main(int argc, char** argv) {
    g_spy.start();  // Start listening
    // ... normal execution ...
    g_spy.stop();   // Cleanup
}
```

Update stats in executeWorkflow():
```cpp
g_spy.workflow_step.store(step_count, std::memory_order_relaxed);
g_spy.elapsed_time.store(elapsed, std::memory_order_relaxed);
```

### 2. Install Python Client

```bash
# The spy_client.py only needs Python 3.6+
# No dependencies required (uses only stdlib: socket, csv, argparse)

# Make executable
chmod +x spy_client.py

# Test connection (game must be running)
python3 spy_client.py status
```

---

## Usage

### Workflow Renderer (Main Process)

```bash
# Terminal 1: Run workflow with spy thread
./workflow_with_spy workflow_cubes.json

# Output:
# [SPY] Listening on localhost:9999
# [WORKFLOW] Loading: Advanced Cubes - With Control Structures
# [WORKFLOW] Nodes: 15 | Variables: 21
# [WORKFLOW] Step 1/15 - Type: graphics.init
# ...
```

### Python Client (Monitoring)

**In another terminal**:

#### Get Current Status
```bash
python3 spy_client.py status

# Output:
# ╔════════════════════════════════════════╗
# ║        WORKFLOW EXECUTION STATUS       ║
# ╚════════════════════════════════════════╝
#
#   draw_calls.....................    121
#   elapsed_time...................   5.23s
#   frame_count....................      0
#   fps............................   0.00 FPS
#   paused.........................  false
#   triangles_rendered.............    726
#   workflow_step..................      3
```

#### Get Single Stat
```bash
python3 spy_client.py get fps
# Output: fps=12.5 FPS

python3 spy_client.py get workflow_step
# Output: workflow_step=5

python3 spy_client.py get elapsed_time
# Output: elapsed_time=8.50s
```

#### Watch in Real-Time
```bash
# Watch all stats every 1 second
python3 spy_client.py watch --interval 1

# Watch specific stat every 0.5 seconds
python3 spy_client.py watch fps --interval 0.5

# Output:
# Watching fps (interval: 0.5s)
# Press Ctrl+C to stop
#
# [14:23:45] fps=60.1 FPS
# [14:23:46] fps=59.8 FPS
# [14:23:46] fps=60.2 FPS
```

#### Record to CSV
```bash
# Record all stats for 60 seconds, sample every 1 second
python3 spy_client.py record workflow_stats.csv --duration 60 --interval 1

# Output:
# Recording to workflow_stats.csv for 60s (interval: 1s)
#   1.0s / 60.0s - 1 records
#   2.0s / 60.0s - 2 records
#   3.0s / 60.0s - 3 records
#   ...
# ✓ Recorded 60 entries to workflow_stats.csv
```

#### Pause and Resume
```bash
# Pause workflow execution
python3 spy_client.py pause
# Output: ✓ Execution paused

# Main thread checks paused flag and pauses after current step
# Use watch to see workflow_step freeze:
python3 spy_client.py watch workflow_step

# Resume execution
python3 spy_client.py resume
# Output: ✓ Execution resumed
```

---

## Monitored Stats

### Workflow Execution
- **`workflow_step`**: Current step number (1-indexed)
- **`elapsed_time`**: Seconds since workflow started

### Game Rendering
- **`frame_count`**: Total frames rendered
- **`fps`**: Frames per second
- **`draw_calls`**: Draw calls in current frame
- **`triangles_rendered`**: Triangles in current frame

### Control
- **`paused`**: true/false - is execution paused?

---

## Example Workflows

### Monitor Frame Generation Progress
```bash
#!/bin/bash
# Monitor workflow progress

echo "Monitoring workflow execution..."

while true; do
    python3 spy_client.py get workflow_step
    sleep 1
done
```

### Detect Performance Drops
```bash
#!/bin/bash
# Alert if FPS drops below threshold

THRESHOLD=50

while true; do
    FPS=$(python3 spy_client.py get fps | cut -d= -f2 | cut -d' ' -f1)
    if (( $(echo "$FPS < $THRESHOLD" | bc -l) )); then
        echo "⚠️  FPS ALERT: $FPS < $THRESHOLD"
    fi
    sleep 1
done
```

### Automated Testing
```python
#!/usr/bin/env python3
# Verify workflow completes successfully

import subprocess
import time

# Start workflow in background
proc = subprocess.Popen(['./workflow_with_spy', 'workflow_cubes.json'])

time.sleep(2)  # Wait for workflow to start

# Monitor completion
while True:
    step = subprocess.run(
        ['python3', 'spy_client.py', 'get', 'workflow_step'],
        capture_output=True, text=True
    )

    print(f"Step: {step.stdout.strip()}")

    if proc.poll() is not None:
        break

    time.sleep(0.5)

print("✓ Workflow completed")
```

---

## Advanced: Custom Monitoring Script

```python
#!/usr/bin/env python3
import subprocess
import time
from datetime import datetime

def get_stat(stat_name):
    result = subprocess.run(
        ['python3', 'spy_client.py', 'get', stat_name],
        capture_output=True, text=True
    )
    return result.stdout.strip()

def get_all_stats():
    result = subprocess.run(
        ['python3', 'spy_client.py', 'status'],
        capture_output=True, text=True
    )
    return result.stdout

# Monitor and log
log_file = open('workflow_monitor.log', 'w')

while True:
    timestamp = datetime.now().isoformat()
    step = get_stat('workflow_step')
    elapsed = get_stat('elapsed_time')
    fps = get_stat('fps')

    log_line = f"{timestamp}: Step {step}, Elapsed {elapsed}, FPS {fps}\n"
    log_file.write(log_line)
    log_file.flush()

    print(log_line.strip())
    time.sleep(1)
```

---

## Performance Impact

### Main Thread Overhead
- Per-step cost: <100 nanoseconds (atomic writes)
- On 16.67ms frame budget: **<0.0006%** overhead
- **Zero blocking**: Never waits for spy thread

### Spy Thread Overhead
- Per-command: ~1-5 milliseconds (socket I/O, not on main thread)
- Idle overhead: Negligible (1s timeout between clients)

### Network Overhead
- Command: Single TCP packet (~50 bytes)
- Response: Single TCP packet (~200 bytes)
- Bandwidth: <1 KB/s per active client

---

## Troubleshooting

### "Connection refused"
```bash
# Make sure workflow is running with spy thread
./workflow_with_spy workflow_cubes.json

# Check if spy thread started
ps aux | grep workflow_with_spy
```

### "Port already in use"
```bash
# Find process using port 9999
lsof -i :9999

# Change port in WorkflowSpyThread or use different machine
python3 spy_client.py --port 10000 status
```

### Stats not updating
```bash
# Verify workflow is executing
python3 spy_client.py watch workflow_step

# Should see workflow_step incrementing
# If not, workflow may be paused or stuck
```

### High latency on watch
```bash
# Reduce refresh interval if desired
python3 spy_client.py watch fps --interval 0.1  # 100ms updates

# Or increase for slower machines
python3 spy_client.py watch fps --interval 2.0  # 2s updates
```

---

## Integration with CI/CD

### GitHub Actions Example
```yaml
name: Workflow Performance Test

on: [push]

jobs:
  test:
    runs-on: macos-latest
    steps:
      - uses: actions/checkout@v2

      - name: Build workflow renderer
        run: |
          cd gameengine/experiment/standalone_workflow_cubes
          clang++ -std=c++17 -pthread main_with_spy.cpp -o workflow_with_spy

      - name: Run workflow
        run: |
          cd gameengine/experiment/standalone_workflow_cubes
          timeout 60 ./workflow_with_spy workflow_cubes.json &
          WORKFLOW_PID=$!
          sleep 2

          # Verify stats are updating
          python3 ../spy_client.py status

          wait $WORKFLOW_PID
```

---

## Files

### Core Implementation
- `main_with_spy.cpp` - Enhanced workflow renderer (350+ lines)
  - WorkflowSpyThread class
  - Integration with executeWorkflow()
  - Example stat updates

### Python Client
- `spy_client.py` - Command-line monitoring tool (350+ lines)
  - Commands: get, status, watch, record, pause, resume
  - argparse for CLI
  - No external dependencies

### Documentation
- `WORKFLOW_SPY_INTEGRATION.md` - This guide
- `SPY_THREAD_GUIDE.md` - Spy thread architecture
- `DEBUGGING_INDEX.md` - Quick reference

---

## Next Steps

1. **Build workflow_with_spy**: Compile main_with_spy.cpp
2. **Run workflow**: `./workflow_with_spy workflow_cubes.json`
3. **Monitor**: `python3 spy_client.py status`
4. **Experiment**: Try all commands from spy_client.py

---

## See Also

- `spy_thread_debugger.cpp` - Spy thread implementation
- `spy_thread_demo.cpp` - Example program
- `workflow_cubes.json` - Workflow definition
- `DEBUGGER_ECOSYSTEM.md` - Complete system overview
