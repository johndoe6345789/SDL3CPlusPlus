# Game Engine Debugger Ecosystem

## Overview

We've built a **complete debugging and inspection system** for the game engine with three complementary approaches:

1. **GDB-MI Debugger** - Low-level code debugging (breakpoints, stepping, stack traces)
2. **Spy Thread Debugger** - Real-time game state inspection (lock-free, high-performance)
3. **Workflow Control Structures** - Data-driven logic (for, while, if, scope, variables)

Together, these provide complete visibility into game execution at all levels.

---

## 1. GDB Machine Interface Debugger

**Purpose**: Debug C++ code - set breakpoints, step through functions, inspect variables

**Architecture**:
- `gdb_debugger.py` - Full GDB-MI wrapper (Python)
- Communicates with GDB via structured Machine Interface output
- Parses breakpoint info, stack frames, local variables
- Supports condition evaluation with custom operators

**Capabilities**:
```
- Set breakpoints at file:line or function name
- Run programs with arguments
- Step through code (next, step, finish)
- Inspect stack traces (backtrace)
- List local variables
- Evaluate expressions
- Parse and return structured data
```

**Example Usage**:
```python
from gdb_debugger import GDBDebugger

debugger = GDBDebugger("./debug_test_dbg")
debugger.start()

debugger.set_breakpoint("debug_test.cpp:47")
debugger.run()

frames = debugger.backtrace(depth=5)
for frame in frames:
    print(f"#{frame.level}: {frame.function} at {frame.file}:{frame.line}")

locals_list = debugger.list_locals()
for var in locals_list:
    print(f"{var.name} = {var.value}")

debugger.quit()
```

**Known Issues**:
- macOS debug symbols: Use `-fno-split-dwarf-inlining` flag
- DWARF format handling varies by clang/GDB version

**When to Use**:
- Debugging shader compilation issues
- Tracing workflow step execution
- Investigating crashes or hangs in engine code
- Variable inspection during game setup

---

## 2. Spy Thread Debugger

**Purpose**: Monitor game state in real-time without blocking rendering

**Architecture**:
- `spy_thread_debugger.cpp` - Lock-free multi-threaded inspector (C++)
- Main thread: Updates atomic variables (negligible overhead)
- Spy thread: Listens on socket, responds to queries
- Communication: `std::atomic<>` for state, TCP for commands

**Key Innovation**:
- **Zero-blocking design**: Main thread never stalls
- **Lock-free updates**: ~3-5 nanoseconds per state update
- **Socket-based commands**: Query from any connected client

**Monitored Stats**:
```cpp
std::atomic<uint64_t> frame_count;        // Current frame number
std::atomic<double> elapsed_time;         // Seconds since start
std::atomic<float> fps;                   // Frames per second
std::atomic<double> gpu_time;             // GPU frame time in ms
std::atomic<double> cpu_time;             // CPU frame time in ms
std::atomic<size_t> memory_used;          // Memory in bytes
std::atomic<uint32_t> draw_calls;         // Draw calls this frame
std::atomic<uint32_t> triangles_rendered; // Triangle count
std::atomic<bool> paused;                 // External pause flag
```

**Commands**:
```
Query:
  get frame_count      - Current frame number
  get fps              - Frames per second
  get memory           - Memory usage
  get all / status     - All stats

Control:
  pause                - Set paused flag (main thread observes)
  resume               - Clear paused flag
  help                 - Show commands
```

**Example Usage**:
```bash
# Terminal 1: Run game
./spy_demo &

# Terminal 2: Connect and inspect
nc localhost 9999
> get fps
< fps=60.2

> status
< frame_count=600
< elapsed_time=10.0
< fps=60.2
< gpu_time=16.5
< cpu_time=14.2
< memory_used=512000000
< draw_calls=121
< triangles_rendered=726
< paused=false

> pause
# Game pauses (main thread checks paused flag)

> resume
# Game continues
```

**Performance**:
- Atomic write: ~3-5 nanoseconds
- Atomic read: ~3-5 nanoseconds
- Socket accept: ~1ms every second (1-second timeout)
- **Total overhead**: <100ns per frame (negligible on 16ms budget)

**Use Cases**:
1. **Real-time profiling** - Watch FPS, GPU time, memory
2. **Hang detection** - Monitor frame counter for freezes
3. **Automated testing** - Query stats to verify behavior
4. **Performance investigation** - Correlate FPS with memory usage
5. **Remote debugging** - Inspect game state without pause/breakpoint

**When to Use**:
- Monitor game performance during playtest
- Verify rendering pipeline is updating
- Detect memory leaks or unexpected spikes
- Automate performance testing
- Investigate frame rate issues without stopping game

---

## 3. Workflow Control Structures

**Purpose**: Express complex game logic entirely in JSON (no C++ code changes)

**Architecture**:
- `workflow_control.hpp/.cpp` - C++ implementation of 8 control structures
- `WORKFLOW_CONTROL_GUIDE.md` - Complete JSON syntax reference
- `workflow_cubes_advanced.json` - Real example with nested loops

**Supported Structures**:
```json
// For loop
{"type": "for", "variable": "i", "start": 0, "end": 10, "nodes": [...]}

// While loop
{"type": "while", "condition": {...}, "nodes": [...]}

// If/else
{"type": "if", "condition": {...}, "then": [...], "else": [...]}

// Variable operations
{"type": "var", "op": "let|set|increment|decrement|push", "name": "x", "value": 42}

// Scope (local variables)
{"type": "scope", "locals": {"x": 10, "y": 20}, "nodes": [...]}

// Switch statement
{"type": "switch", "expr": "${var}", "cases": [{"value": "case1", "nodes": [...]}]}

// Sequence (explicit ordering)
{"type": "sequence", "nodes": [...]}

// Parallel (mark for concurrent execution)
{"type": "parallel", "nodes": [...]}
```

**Condition Operators**:
- Comparison: `==`, `!=`, `<`, `<=`, `>`, `>=`
- Logical: `&&`, `||`, `!`
- Arithmetic: `+`, `-`, `*`, `/`, `%`

**Example**:
```json
{
  "type": "for",
  "variable": "row",
  "start": 0,
  "end": 11,
  "nodes": [
    {
      "type": "for",
      "variable": "col",
      "start": 0,
      "end": 11,
      "nodes": [
        {
          "type": "render.cube",
          "parameters": {
            "x": "${variables.grid_start_x} + ${col} * 3.0",
            "y": "${variables.grid_start_y} + ${row} * 3.0"
          }
        }
      ]
    }
  ]
}
```

**When to Use**:
- Implement game logic without C++ recompilation
- Parameterize rendering pipelines
- Define multi-frame animations
- Control asset loading sequences
- Data-driven game flow

---

## Integration Points

### GDB → Spy Thread
```
Problem: "Why is FPS dropping?"
Solution:
1. Use GDB to debug graphics code
2. Use Spy Thread to monitor FPS in real-time
3. Correlate GDB breakpoints with FPS dips
```

### Spy Thread → Workflows
```
Problem: "Test if loop iterations are correct"
Solution:
1. Workflow defines for loop (JSON)
2. C++ execution increments counter
3. Spy Thread queries counter value
4. Automated test verifies count matches expectation
```

### Workflows → GDB
```
Problem: "Workflow step isn't executing"
Solution:
1. Workflow JSON defines conditional execution
2. GDB step through step handler code
3. Inspect condition evaluation
4. Fix JSON or code as needed
```

---

## Complete Debugging Workflow

### Scenario: "Orange cube isn't rendering"

**Step 1: Check with Spy Thread**
```bash
./game &
nc localhost 9999
> get frame_count    # Verify frames updating
> get draw_calls     # Verify 121 draw calls happening
> status             # Check all metrics
```
→ **Result**: Frame counter stuck at 0? Spy Thread shows game isn't updating → GDB needed
→ **Result**: Frame counter updating but triangles=0? Rendering issue → Check workflow

**Step 2: Use GDB to Debug**
```python
debugger.set_breakpoint("main.cpp:335")  # Render loop
debugger.run()
# Inspect variables, step through rendering
```
→ **Result**: Find shader loading failed → Check workflow
→ **Result**: Find geometry not bound → Check workflow step order

**Step 3: Verify with Workflow JSON**
```json
// Workflow: Check step order
[
  {"type": "geometry.create_cube"},      // Must come first
  {"type": "shader.load_binary"},        // Then shaders
  {"type": "render.cube_grid"}           // Then render
]
```
→ **Result**: Wrong order → Reorder JSON steps
→ **Result**: Missing step → Add step to workflow

**Step 4: Confirm with Spy Thread**
```bash
# After JSON fix, restart game
./game &
nc localhost 9999
> get triangles      # Should see 726 (121 cubes × 6 faces × 2 triangles)
```
→ **Result**: Triangles non-zero → Rendering working!

---

## Comparison Table

| Feature | GDB-MI | Spy Thread | Workflows |
|---------|--------|-----------|-----------|
| **Code Debugging** | ✅ Full | ❌ No | ❌ No |
| **Breakpoints** | ✅ Yes | ❌ No | ❌ No |
| **State Inspection** | ✅ At breakpoint | ✅ Real-time | ❌ No |
| **Blocks Main Thread** | ✅ Yes (at break) | ❌ No | ❌ No |
| **Game Logic** | ❌ No | ❌ No | ✅ Full |
| **Performance Impact** | Variable | Negligible | Depends on logic |
| **Setup Complexity** | Medium | Low | Low |
| **Python API** | ✅ Yes | ✅ Possible | ✅ Native JSON |

---

## Files Summary

### GDB-MI Debugger
- `gdb_debugger.py` - Full implementation with parser
- `debugger_interface.py` - Simplified command interface
- `GDB_DEBUGGER_GUIDE.md` - Complete reference
- `debug_test.cpp` / `debug_test_dbg` - Test program and binary

### Spy Thread Debugger
- `spy_thread_debugger.cpp` - Core implementation (350 lines)
- `spy_thread_demo.cpp` - Demonstration program
- `SPY_THREAD_GUIDE.md` - Complete reference
- `test_spy_thread.sh` - Automated test script

### Workflow Control Structures
- `workflow_control.hpp` - C++ interface and parser
- `workflow_control.cpp` - Implementation (8 control types)
- `WORKFLOW_CONTROL_GUIDE.md` - JSON syntax reference
- `workflow_cubes_advanced.json` - Real usage example

---

## Future Extensions

### Spy Thread: Multi-Client Support
```cpp
// Current: Single client
// Future: std::vector<int> client_sockets
//         Fork/thread per client connection
```

### Spy Thread: Command Injection Hooks
```cpp
// Add ability to execute commands from spy thread
spy.register_command("screenshot", []() { ... });
spy.register_command("save_state", []() { ... });
```

### GDB: Python Debugger API
```python
# Extend to support async breakpoints
with debugger.watch("frame_count") as watcher:
    for value in watcher:
        print(f"Frame: {value}")
```

### Workflows: Performance Profiling
```json
{
  "type": "profile",
  "nodes": [...],
  "output_stats": "profile.json"
}
```

---

## See Also

- Main game engine: `gameengine/experiment/standalone_workflow_cubes/main.cpp`
- Workflow system: `gameengine/workflows/demo_gameplay.json`
- Package shaders: `gameengine/packages/standalone_cubes/shaders/`
- CLAUDE.md: Project standards and patterns
