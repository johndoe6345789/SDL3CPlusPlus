# Debugging System Index

## Quick Start

### For Real-Time Game Monitoring (Spy Thread)
1. **Compile demo**: `clang++ -std=c++17 -pthread spy_thread_debugger.cpp spy_thread_demo.cpp -o spy_demo`
2. **Run demo**: `./spy_demo`
3. **In another terminal**: `nc localhost 9999`
4. **Query**: `get fps`, `status`, `pause`, `resume`

**Read First**: `SPY_THREAD_GUIDE.md`

### For Code Debugging (GDB)
1. **Compile test**: `clang++ -g -O0 -fno-split-dwarf-inlining debug_test.cpp -o debug_test_dbg`
2. **Run debugger**: `python3 gdb_debugger.py ./debug_test_dbg`
3. **In Python**: Set breakpoints, step through code, inspect variables

**Read First**: `GDB_DEBUGGER_GUIDE.md`

### For Game Logic (Workflows)
1. **Edit workflow**: `gameengine/workflows/demo_gameplay.json`
2. **Use structures**: for loops, while loops, if conditions, scopes
3. **Configure in JSON**: No C++ recompilation needed

**Read First**: `WORKFLOW_CONTROL_GUIDE.md`

---

## Complete File Structure

### Spy Thread Debugger (Real-Time Monitoring)
```
spy_thread_debugger.cpp      - Core implementation (350 lines)
  - SpyThreadDebugger class
  - 9 atomic variables for state
  - Socket server on localhost:9999
  - Command parser (get, status, pause, resume, help)

spy_thread_demo.cpp          - Example program (100 lines)
  - Simulates game loop
  - Shows integration pattern
  - Updates atomics every frame

SPY_THREAD_GUIDE.md          - Complete reference (300+ lines)
  - Architecture and lock-free design
  - Command reference with examples
  - Integration with cube renderer
  - Advanced use cases (profiling, hang detection)
  - Memory model and performance analysis

test_spy_thread.sh           - Automated test script
  - Tests all commands
  - Demonstrates pause/resume
```

### GDB Machine Interface Debugger (Code Inspection)
```
gdb_debugger.py              - Full GDB-MI wrapper (400+ lines)
  - GDBMIParser class
  - GDBDebugger class with all debugging methods
  - Condition evaluator with operators
  - Structured output parsing

debugger_interface.py        - Simplified interface (250 lines)
  - GDBInterface class
  - Batch and interactive modes
  - Direct GDB command execution

GDB_DEBUGGER_GUIDE.md        - Complete reference (600+ lines)
  - GDB-MI format explanation
  - Python wrapper implementation
  - Known issues and solutions
  - Usage examples

debug_test.cpp               - Test program (75 lines)
  - Functions, classes, recursion
  - Demonstrates debugging scenarios

debug_test_dbg               - Compiled binary (with debug symbols)
```

### Workflow Control Structures (Data-Driven Logic)
```
../experiment/standalone_workflow_cubes/
  workflow_control.hpp       - Interface definitions
  workflow_control.cpp       - 8 control structures
    - ForLoopControl
    - WhileLoopControl
    - IfControl
    - SwitchControl
    - ScopeControl
    - SequenceControl
    - ParallelControl
    - VariableOpControl

WORKFLOW_CONTROL_GUIDE.md    - JSON syntax reference
  - Complete examples for each structure
  - Supported operators
  - Performance considerations

workflow_cubes_advanced.json - Real usage example
  - 11×11 cube grid rendering
  - Nested for loops
  - Conditional feature loading
  - Scoped variables
```

### Documentation & Overview
```
DEBUGGER_ECOSYSTEM.md        - Integration guide (400+ lines)
  - How all three systems work together
  - Complete debugging workflow
  - Comparison table
  - Integration points
  - Future extensions

DEBUGGING_INDEX.md           - This file
  - Quick start for each system
  - File structure overview
  - Selection guide
  - Key concepts
```

---

## Selection Guide: Which Tool to Use?

### Use **Spy Thread** When:
- Monitoring real-time game performance
- Profiling FPS and frame time
- Detecting hangs or freezes
- Testing in automated scripts
- Checking memory usage
- Avoiding breakpoints (game must keep running)
- ✅ **Never blocks main thread**

### Use **GDB Debugger** When:
- Debugging C++ code at function level
- Need to set breakpoints
- Stepping through function calls
- Inspecting variable values at specific points
- Analyzing stack traces
- ⚠️ **Pauses entire program at breakpoint**

### Use **Workflows** When:
- Configuring game logic without rebuilds
- Defining multi-frame animations
- Parameterizing rendering pipelines
- Conditional asset loading
- Data-driven game flow
- ✅ **No code changes needed**

---

## Integration Patterns

### Pattern 1: Monitor While Debugging
```
GDB pauses at breakpoint
→ Spy thread still running (reads stale atomic values)
→ Resume GDB → Spy thread shows live updates again
```

### Pattern 2: Profile Workflow Execution
```
Workflow executes (JSON-driven)
→ Main thread updates spy thread atomics
→ External script queries spy thread every frame
→ Detects performance bottlenecks
```

### Pattern 3: Automated Testing
```
Workflow runs with test config
→ Spy thread monitors execution
→ Python script checks stats
→ Verifies correct behavior without breakpoints
```

---

## Key Concepts

### Lock-Free Architecture
The spy thread uses `std::atomic<>` instead of mutexes:
- **Zero blocking**: Main thread never waits for locks
- **Ultra-fast**: ~3-5 nanoseconds per state update
- **Safe**: Atomic operations guarantee thread safety

### Memory Model
```cpp
// Main thread (game loop)
spy.update_fps(60.5);                    // ~5ns
spy.update_frame_count(frame_num);       // ~5ns

// Spy thread (socket handler)
float current_fps = spy.fps.load();      // ~5ns read
// Send over socket (milliseconds, but in different thread)
```

### Socket Communication
```
Main thread: Game loop → Update atomics (ns)
Spy thread: Accept connection (ms) → Read atomics (ns) → Send response (ms)
Result: Main thread never blocks on socket operations
```

---

## Compilation Examples

### Spy Thread Only
```bash
clang++ -std=c++17 -pthread \
  spy_thread_debugger.cpp \
  spy_thread_demo.cpp \
  -o spy_demo
```

### GDB Debugger Test Program
```bash
clang++ -std=c++17 -g -O0 -fno-split-dwarf-inlining \
  debug_test.cpp \
  -o debug_test_dbg
```

### Full Game Engine
```bash
cd gameengine
cmake --build build/Release
./build/Release/sdl3_app --bootstrap bootstrap_mac --game standalone_cubes

# In another terminal:
nc localhost 9999
```

---

## Performance Impact

### Spy Thread Updates
- Per-frame cost: <100 nanoseconds (atomic writes)
- On 60 FPS (16.67ms frame budget): <0.0006% overhead

### GDB Breakpoints
- Breakpoint overhead: 1-10 milliseconds
- Program paused completely

### Workflow Execution
- Overhead: Depends on logic complexity
- Loops limited to 10,000 iterations (safety)

---

## Troubleshooting

### GDB: "No debugging symbols found"
```bash
# Use this compilation flag:
clang++ -g -O0 -fno-split-dwarf-inlining debug_test.cpp -o debug_test_dbg
```

### Spy Thread: "Port already in use"
```bash
# Change port in spy_thread_debugger.cpp:
SpyThreadDebugger spy("localhost", 9998);  // Use different port
```

### Spy Thread: No connection response
```bash
# Verify server is listening:
lsof -i :9999

# Or increase accept timeout in spy_thread_debugger.cpp:
setsockopt(server_socket_, SOL_SOCKET, SO_RCVTIMEO, ...);
```

---

## Further Reading

1. **Lock-Free Programming**: Search "std::atomic memory_order_relaxed"
2. **GDB Machine Interface**: https://sourceware.org/gdb/onlinedocs/gdb/GDB_002fMI.html
3. **Socket Programming**: Unix Network Programming (Stevens & Rago)
4. **Game Engine Architecture**: Game Engine Architecture (Jason Gregory)

---

## File Relationships

```
DEBUGGING_INDEX.md
    ↓
    ├─→ SPY_THREAD_GUIDE.md
    │       ↓
    │       └─→ spy_thread_debugger.cpp
    │           spy_thread_demo.cpp
    │
    ├─→ GDB_DEBUGGER_GUIDE.md
    │       ↓
    │       └─→ gdb_debugger.py
    │           debugger_interface.py
    │
    ├─→ WORKFLOW_CONTROL_GUIDE.md
    │       ↓
    │       └─→ workflow_control.hpp/cpp
    │           workflow_cubes_advanced.json
    │
    └─→ DEBUGGER_ECOSYSTEM.md
            ↓
            └─→ All of the above (integration)
```

---

## Quick Reference

### Spy Thread Commands
```
get frame_count              # Query frame number
get fps                      # Query frames per second
get all / status             # Get all statistics
pause / resume               # Control game execution
list_commands / help         # Show available commands
```

### GDB Debugger Methods
```python
debugger.set_breakpoint(location)    # Set breakpoint
debugger.run(args)                   # Run program
debugger.step() / next()             # Step through code
debugger.backtrace(depth)            # Show stack trace
debugger.print_variable(name)        # Inspect variable
debugger.list_locals()               # Show local variables
```

### Workflow Control Structures
```json
{"type": "for", "variable": "i", "start": 0, "end": 10}
{"type": "while", "condition": {...}}
{"type": "if", "condition": {...}, "then": [...], "else": [...]}
{"type": "var", "op": "increment", "name": "counter"}
{"type": "scope", "locals": {"x": 10}, "nodes": [...]}
```

---

**Status**: Production-ready (all systems tested and documented)
**Last Updated**: Feb 11, 2026
**Session**: 29 (Debugger Ecosystem Complete)
