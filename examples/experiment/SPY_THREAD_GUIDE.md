# Spy Thread Debugger Guide

## Architecture Overview

**Lock-Free Multi-Threaded Inspection**:
- **Main Thread**: Renders game, updates atomic variables
- **Spy Thread**: Listens on socket, reads atomics, responds to commands
- **Communication**: `std::atomic<>` for state (no mutexes), sockets for commands

**Key Insight**: The spy thread never blocks the main thread. All state updates are through atomic variables with relaxed memory ordering (extremely fast).

---

## How It Works

### Main Thread (Game Loop)
```cpp
SpyThreadDebugger spy("localhost", 9999);
spy.start();  // Start listening on port 9999

while (running) {
    frame_num++;
    elapsed += delta_time;

    // Update spy thread atomics (lock-free, ~nanoseconds)
    spy.update_frame_count(frame_num);
    spy.update_elapsed_time(elapsed);
    spy.update_fps(current_fps);
    spy.update_memory(get_memory_usage());
    spy.update_draw_calls(draw_call_count);

    // Render normally
    render_frame();

    // Check if external command paused us
    if (spy.paused.load()) {
        wait_until_resumed();
    }
}

spy.stop();
```

### Client (Debugger)
```bash
# Terminal 1: Run game
./game --bootstrap bootstrap_mac --game standalone_cubes

# Terminal 2: Connect to spy thread
nc localhost 9999

# Now you can query live stats:
> get fps
< fps=60.2

> get frame_count
< frame_count=1200

> status
< frame_count=1200
< elapsed_time=20.0
< fps=60.2
< gpu_time=16.5
< cpu_time=14.2
< memory_used=512000000
< draw_calls=121
< triangles_rendered=726

> pause
< paused=true

# Game pauses (main thread checks paused flag)

> resume
< paused=false

# Game continues
```

---

## Command Reference

### Query Commands
```
get frame_count      - Current frame number
get elapsed_time     - Seconds since start
get fps              - Frames per second
get gpu_time         - GPU time in milliseconds
get cpu_time         - CPU time in milliseconds
get memory           - Memory usage in bytes
get draw_calls       - Number of draw calls this frame
get triangles        - Triangles rendered this frame
get all              - Get all stats
status               - Alias for 'get all'
```

### Control Commands
```
pause                - Set paused flag (main thread observes)
resume               - Clear paused flag
list_commands        - Show available commands
help                 - Show full help
```

---

## Integration with Cube Renderer

### Step 1: Include Header
```cpp
#include "spy_thread_debugger.cpp"  // Contains full implementation
```

### Step 2: Create Spy Thread
```cpp
int main() {
    SpyThreadDebugger spy("localhost", 9999);

    if (!spy.start()) {
        std::cerr << "Failed to start spy thread\n";
        return 1;
    }

    // ... initialize graphics ...

    uint64_t frame_count = 0;
    auto start_time = std::chrono::high_resolution_clock::now();

    while (running) {
        // Update spy stats
        frame_count++;
        auto now = std::chrono::high_resolution_clock::now();
        double elapsed = std::chrono::duration<double>(now - start_time).count();

        spy.update_frame_count(frame_count);
        spy.update_elapsed_time(elapsed);
        spy.update_fps(frame_count / elapsed);
        spy.update_draw_calls(121);  // 11x11 grid
        spy.update_triangles(121 * 6);  // 2 triangles per cube face, 6 faces

        // Check if external code requested pause
        if (spy.paused.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        // Render frame...
    }

    spy.stop();
    return 0;
}
```

### Step 3: Query from Terminal
```bash
# While game is running in another terminal:
$ nc localhost 9999
> status
frame_count=1200
elapsed_time=20.1
fps=59.7
gpu_time=16.8
cpu_time=14.5
memory_used=512000000
draw_calls=121
triangles_rendered=726
paused=false

> get frame_count
frame_count=1234
```

---

## Advanced Use Cases

### Real-Time Performance Profiling
```bash
#!/bin/bash
# Profile game every second
for i in {1..60}; do
    echo "get fps" | nc localhost 9999
    sleep 1
done
```

### Automated Testing
```python
#!/usr/bin/env python3
import socket
import time

def query_stat(stat_name):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect(('localhost', 9999))
    s.send(f"get {stat_name}\n".encode())
    response = s.recv(1024).decode()
    s.close()
    return response.strip()

# Monitor for 60 frames
for i in range(60):
    fps = query_stat("fps")
    print(f"Frame {i}: {fps}")
    time.sleep(0.016)  # ~60 FPS
```

### Hang Detection
```cpp
// Background thread monitoring
void hang_detector_thread(SpyThreadDebugger& spy) {
    uint64_t last_frame = 0;
    auto last_check = std::chrono::high_resolution_clock::now();

    while (true) {
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration<double>(now - last_check).count();

        uint64_t current_frame = spy.frame_count.load();

        if (current_frame == last_frame && elapsed > 1.0) {
            // Frame counter hasn't changed in 1+ second
            std::cerr << "HANG DETECTED: Game thread is stuck!\n";
            // Can send alert, dump state, etc.
        }

        last_frame = current_frame;
        last_check = now;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}
```

### Frame Capture Triggering
```bash
# Monitor FPS and capture screenshot when it dips
$ while true; do
>   fps=$(echo "get fps" | nc localhost 9999 | grep fps | cut -d= -f2)
>   if (( $(echo "$fps < 30" | bc -l) )); then
>     echo "FPS dropped to $fps, taking screenshot..."
>     # Trigger screenshot logic
>   fi
>   sleep 1
> done
```

---

## Memory Model Details

### Why Lock-Free Works Here

**Main Thread → Spy Thread (One-way data flow)**:
```cpp
// Main thread writes
spy.update_fps(60.5);
// Internally:
fps.store(60.5, std::memory_order_relaxed);

// Spy thread reads
float current_fps = fps.load(std::memory_order_relaxed);
```

**Memory Ordering**:
- `memory_order_relaxed`: No synchronization overhead, just atomic operation
- No locks = no contention = main thread never stalls
- Spy thread always reads latest value written by main thread

**Performance Impact**:
- Atomic write: ~3-5 nanoseconds
- Atomic read: ~3-5 nanoseconds
- Total overhead per frame: <100ns (negligible on 16ms frame budget)

### Safety Guarantees

1. **No data races**: All shared variables are `std::atomic<>`
2. **Main thread never blocks**: Spy thread only does socket I/O
3. **One-way communication**: Main → Spy (atomic reads), Spy → Main (paused flag)
4. **Graceful shutdown**: `running_.store(false)` signals both threads to exit

---

## Compilation

### Standalone Test
```bash
# Simple test program
g++ -std=c++17 -pthread spy_thread_debugger.cpp -o spy_test
./spy_test &
nc localhost 9999
```

### With Game Engine
```bash
# Already includes in main.cpp
cmake --build build/Release
./build/Release/sdl3_app --bootstrap bootstrap_mac --game standalone_cubes

# In another terminal:
nc localhost 9999
```

---

## Known Limitations

1. **Single client**: Current implementation accepts one client at a time
   - To support multiple: Use `std::vector<int>` for client sockets, fork/thread per client

2. **No encryption**: Commands sent in plaintext
   - For production: Add TLS/SSL layer

3. **Local only**: Default binds to `localhost` only
   - To expose externally: Change to `0.0.0.0`, add authentication

4. **Socket blocking**: Accept has 1-second timeout
   - If no client: 1ms latency every second
   - negligible for game loop

---

## Comparison with Other Approaches

| Approach | Thread Safety | Performance | Flexibility |
|----------|---------------|-------------|-------------|
| **Spy Thread (This)** | ✅ Lock-free | ⭐⭐⭐ Excellent | ⭐⭐⭐ Full |
| Mutex-protected queue | ✅ Guaranteed | ⭐⭐ Good | ⭐⭐ Limited |
| Pause/Resume with locks | ✅ Guaranteed | ⭐ Poor | ⭐⭐⭐ Full |
| Shared memory files | ⚠️ Race condition | ⭐⭐ Good | ⭐⭐ Limited |
| Named pipes | ✅ Guaranteed | ⭐⭐ Good | ⭐⭐ Limited |

---

## See Also

- `spy_thread_debugger.cpp` - Full implementation (300 lines)
- `GDB_DEBUGGER_GUIDE.md` - GDB-MI debugger for code debugging
- `WORKFLOW_CONTROL_GUIDE.md` - JSON control structures
- Game engine main loop: `gameengine/experiment/standalone_workflow_cubes/main.cpp`
