# Spy Client Quick Start

## Installation

```bash
# No installation needed! Only requires Python 3.6+
# Just make executable:
chmod +x spy_client.py
```

## Commands

### Get Help
```bash
python3 spy_client.py --help
python3 spy_client.py get --help
python3 spy_client.py watch --help
```

### Get Single Stat
```bash
# Get current FPS
python3 spy_client.py get fps

# Get workflow step
python3 spy_client.py get workflow_step

# Get elapsed time
python3 spy_client.py get elapsed_time

# Get frame count
python3 spy_client.py get frame_count

# Get draw calls
python3 spy_client.py get draw_calls

# Get triangles rendered
python3 spy_client.py get triangles_rendered
```

### Get All Stats
```bash
python3 spy_client.py status
```

Output:
```
╔════════════════════════════════════════╗
║        WORKFLOW EXECUTION STATUS       ║
╚════════════════════════════════════════╝

  draw_calls.....................    121
  elapsed_time...................   5.23s
  frame_count....................    600
  fps............................  60.1 FPS
  paused.........................  false
  triangles_rendered.............    726
  workflow_step..................      3
```

### Watch Stats in Real-Time

**Watch all stats every 1 second**:
```bash
python3 spy_client.py watch --interval 1
```

**Watch specific stat every 0.5 seconds**:
```bash
python3 spy_client.py watch fps --interval 0.5
python3 spy_client.py watch workflow_step --interval 1
python3 spy_client.py watch elapsed_time --interval 2
```

Press Ctrl+C to stop.

### Record to CSV

**Record for 60 seconds, sample every 1 second**:
```bash
python3 spy_client.py record stats.csv --duration 60 --interval 1
```

**Record for 30 seconds, high-frequency sampling**:
```bash
python3 spy_client.py record stats.csv --duration 30 --interval 0.1
```

**Record indefinitely (until Ctrl+C)**:
```bash
python3 spy_client.py record stats.csv --duration 999999
```

Output: CSV file with columns for timestamp, elapsed, and all stats

### Pause/Resume

**Pause execution**:
```bash
python3 spy_client.py pause
```

Main thread will pause after completing current step. Watch to verify:
```bash
python3 spy_client.py watch workflow_step
# workflow_step will freeze
```

**Resume execution**:
```bash
python3 spy_client.py resume
```

---

## Common Use Cases

### Monitor Frame Generation
```bash
# Watch FPS while workflow runs
python3 spy_client.py watch fps

# Or record for analysis
python3 spy_client.py record fps_log.csv --duration 120
```

### Track Workflow Progress
```bash
# See which step workflow is on
python3 spy_client.py watch workflow_step --interval 0.5
```

### Verify Completion
```bash
# Get elapsed time when workflow finishes
python3 spy_client.py get elapsed_time
```

### Performance Analysis
```bash
# Record all metrics for 1 minute
python3 spy_client.py record performance.csv --duration 60 --interval 1

# Then analyze CSV in spreadsheet or Python:
# python3 analyze_performance.py performance.csv
```

### Automated Testing
```bash
# Bash script to verify stats
#!/bin/bash

# Check if rendering is happening
TRIANGLES=$(python3 spy_client.py get triangles_rendered | cut -d= -f2)
if [ "$TRIANGLES" -eq "726" ]; then
    echo "✓ Correct number of triangles rendered"
else
    echo "✗ Wrong triangle count: $TRIANGLES"
    exit 1
fi

# Check FPS is reasonable
FPS=$(python3 spy_client.py get fps | cut -d' ' -f1 | cut -d= -f2)
if (( $(echo "$FPS > 30" | bc -l) )); then
    echo "✓ FPS is acceptable: $FPS"
else
    echo "✗ FPS too low: $FPS"
    exit 1
fi
```

---

## Connection Options

### Local Connection (Default)
```bash
# Connects to localhost:9999
python3 spy_client.py status
```

### Different Host
```bash
# Connect to remote machine
python3 spy_client.py --host 192.168.1.100 status
```

### Different Port
```bash
# Use custom port (if game runs on different port)
python3 spy_client.py --port 10000 status
```

### Custom Timeout
```bash
# Increase timeout for slow/remote connections (seconds)
python3 spy_client.py --timeout 10.0 status
```

---

## Advanced Examples

### Monitor for Performance Regression
```bash
#!/bin/bash
# Alert if FPS drops below threshold

python3 spy_client.py pause  # Pause any running workflow

# Record baseline
echo "Recording baseline performance (60s)..."
python3 spy_client.py record baseline.csv --duration 60 --interval 1

# Extract average FPS
AVG_FPS=$(awk -F= '{sum+=$NF; count++} END {print sum/count}' baseline.csv | grep fps)
echo "Baseline FPS: $AVG_FPS"

python3 spy_client.py resume  # Resume
```

### Monitor Multiple Workloads
```bash
#!/bin/bash
# Profile different workflows

for workflow in basic advanced complex; do
    echo "Testing $workflow..."
    ./workflow_with_spy workflows/${workflow}.json &
    sleep 2

    python3 spy_client.py record ${workflow}_profile.csv \
        --duration 30 --interval 0.5

    pkill -f workflow_with_spy
    sleep 1
done
```

### Real-Time Dashboard (Using Python)
```python
#!/usr/bin/env python3
import subprocess
import time
from datetime import datetime
import curses

def get_stats():
    result = subprocess.run(
        ['python3', 'spy_client.py', 'status'],
        capture_output=True, text=True
    )
    return result.stdout

def main(stdscr):
    while True:
        stdscr.clear()
        stats = get_stats()
        stdscr.addstr(f"[{datetime.now().isoformat()}]\n")
        stdscr.addstr(stats)
        stdscr.refresh()
        time.sleep(0.5)

if __name__ == '__main__':
    curses.wrapper(main)
```

---

## Troubleshooting

### Connection Error
```
✗ Connection refused: 127.0.0.1:9999
  Is the game running with spy thread enabled?
```

**Solution**:
```bash
# Make sure workflow is running
./workflow_with_spy workflow_cubes.json
```

### Timeout Error
```
✗ Timeout: Could not reach 127.0.0.1:9999
```

**Solutions**:
- Increase timeout: `python3 spy_client.py --timeout 10.0 status`
- Check firewall settings
- Verify game is still running

### No Stats Updating
```bash
# Verify workflow is executing
python3 spy_client.py watch workflow_step

# Should see incrementing numbers
# If not, workflow may be paused or stuck
```

---

## Tips

1. **Combine commands**: Pipe to other tools
   ```bash
   python3 spy_client.py get fps | grep -o '[0-9]*\.[0-9]*'
   ```

2. **Create shortcuts**: Shell aliases
   ```bash
   alias spy-status='python3 spy_client.py status'
   alias spy-fps='python3 spy_client.py get fps'
   alias spy-watch='python3 spy_client.py watch'
   ```

3. **Monitor in background**: Start watch in tmux
   ```bash
   tmux new-session -d -s monitor
   tmux send-keys -t monitor "python3 spy_client.py watch" Enter
   ```

4. **Export data**: Convert CSV to other formats
   ```bash
   # CSV to JSON
   python3 -c "import csv,json; \
    f=csv.DictReader(open('stats.csv')); \
    print(json.dumps(list(f)))" > stats.json
   ```

---

## See Also

- `WORKFLOW_SPY_INTEGRATION.md` - Full integration guide
- `SPY_THREAD_GUIDE.md` - Spy thread architecture
- `main_with_spy.cpp` - Workflow renderer with spy thread
- `DEBUGGER_ECOSYSTEM.md` - Complete system overview
