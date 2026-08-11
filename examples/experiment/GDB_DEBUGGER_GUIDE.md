# GDB Machine Interface Debugger Guide

## Overview

This guide demonstrates programmatic control of GDB for debugging C++ programs. The debugger interface provides methods to set breakpoints, run code, inspect variables, and trace execution.

---

## Test Program

**File**: `debug_test.cpp`
- Simple functions: `add()`, `fibonacci()`
- Class with state: `DataProcessor` with vector
- Demonstrates: arithmetic, recursion, classes, vectors, loops

**Compilation**:
```bash
# Standard compilation with debug symbols
clang++ -std=c++11 -g3 -O0 debug_test.cpp -o debug_test_dbg

# With frame pointers and full debug info
clang++ -std=c++11 -g -O0 -fno-omit-frame-pointer debug_test.cpp -o debug_test_dbg
```

---

## GDB Machine Interface (MI)

GDB's Machine Interface provides structured, machine-readable output format.

### Command Format
```
token^result-class result
Example: 1^done,reason="breakpoint-hit",thread-id="1"
```

### Starting GDB-MI
```bash
gdb -i=mi ./program
```

### Key MI Commands
```
-break-insert file:line           # Set breakpoint
-exec-run [args]                  # Run program
-exec-next                        # Step over
-exec-step                        # Step into
-exec-continue                    # Continue execution
-stack-list-frames [low] [high]   # Get stack trace
-stack-list-locals [values]       # List local variables
-data-evaluate-expression <expr>  # Evaluate expression
```

---

## Python Wrapper Implementation

### GDB Parser Class
```python
class GDBMIParser:
    """Parse GDB-MI output"""

    @staticmethod
    def parse_output(line: str) -> Tuple[Optional[str], Dict[str, Any]]:
        """
        Parse: token^result-class result
        Returns: (result_class, data_dict)
        """
        match = re.match(r'^(\d+)\^(\w+)(.*)', line)
        if not match:
            return None, None

        token = match.group(1)
        result_class = match.group(2)  # done, running, stopped, error
        result_text = match.group(3)

        # Parse key=value pairs
        data = GDBMIParser._parse_result(result_text)
        return result_class, data
```

### GDB Debugger Class
```python
class GDBDebugger:
    """Interface to GDB via Machine Interface"""

    def start(self) -> bool:
        """Start GDB with MI interface"""
        self.process = subprocess.Popen(
            ['gdb', '-i=mi', self.program],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            bufsize=1
        )

    def send_command(self, cmd: str) -> str:
        """Send command and collect response"""
        self.command_counter += 1
        full_cmd = f"{self.command_counter}{cmd}\n"

        self.process.stdin.write(full_cmd)
        self.process.stdin.flush()

        # Read until result line
        response = ""
        while True:
            line = self.process.stdout.readline()
            response += line

            # Stop at (gdb) prompt
            if line.strip() == "(gdb)":
                break

            # Stop at result line (token^result)
            if line.startswith(str(self.command_counter)) and '^' in line:
                self.process.stdout.readline()  # Read prompt
                break

        return response
```

---

## Usage Examples

### Setting Breakpoints
```python
debugger = GDBDebugger("./debug_test_dbg")
debugger.start()

# Set breakpoint at line
bp_num = debugger.set_breakpoint("debug_test.cpp:47")

# Set breakpoint at function
bp_num = debugger.set_breakpoint("main")

# List breakpoints
bps = debugger.list_breakpoints()
```

### Running and Stepping
```python
# Run program
result = debugger.run()

# Step operations
debugger.next()      # Step over
debugger.step()      # Step into
debugger.finish()    # Run until return
debugger.continue_execution()  # Continue to next breakpoint
```

### Inspecting State
```python
# Stack trace
frames = debugger.backtrace(depth=10)
for frame in frames:
    print(f"#{frame.level}: {frame.function} at {frame.file}:{frame.line}")

# Local variables
locals_list = debugger.list_locals()
for var in locals_list:
    print(f"{var.name} ({var.type}) = {var.value}")

# Single variable
value = debugger.print_variable("x")
```

### Session Info
```python
info = debugger.get_info()
print(json.dumps(info, indent=2))
# {
#   "program": "./debug_test_dbg",
#   "running": false,
#   "breakpoints": {
#     "1": {"file": "debug_test.cpp", "line": 47, "enabled": true}
#   },
#   "variables": {
#     "x": {"name": "x", "type": "int", "value": "10"}
#   }
# }
```

---

## Known Issues on macOS

### Issue: "No debugging symbols found"
**Cause**: Mach-O format debug symbol handling in newer clang/gdb

**Solutions**:
1. Disable dSYM splitting:
   ```bash
   clang++ -g -O0 -fno-split-dwarf-inlining debug_test.cpp -o debug_test_dbg
   ```

2. Use system GDB instead of homebrew GDB:
   ```bash
   /usr/bin/gdb ./debug_test_dbg
   ```

3. Recompile with GCC (if available):
   ```bash
   g++ -g -O0 debug_test.cpp -o debug_test_dbg
   ```

### Issue: "Can't open to read symbols"
**Cause**: Temporary object file path in debug info

**Solution**: Remove dSYM:
```bash
rm -rf debug_test_dbg.dSYM
clang++ -g debug_test.cpp -o debug_test_dbg
```

---

## Control Structures in Workflows

The workflow system supports C++-like control structures in JSON:

### For Loop
```json
{
  "type": "for",
  "variable": "i",
  "start": 0,
  "end": 10,
  "nodes": [
    {"type": "execute_node", "parameters": {"index": "${i}"}}
  ]
}
```

### While Loop
```json
{
  "type": "while",
  "condition": {"op": "<", "left": "${frame_count}", "right": 120},
  "nodes": [...]
}
```

### Conditional
```json
{
  "type": "if",
  "condition": {"op": "==", "left": "${quality}", "right": "high"},
  "then": [...],
  "else": [...]
}
```

### Scope (Local Variables)
```json
{
  "type": "scope",
  "locals": {"temp_x": 42, "temp_y": 100},
  "nodes": [...]
}
```

### Variable Operations
```json
{
  "type": "var",
  "op": "let",
  "name": "counter",
  "value": 0
},
{
  "type": "var",
  "op": "increment",
  "name": "counter"
}
```

---

## Integration with Workflow Engine

The workflow engine's condition evaluator supports:

**Operators**:
- Comparison: `==`, `!=`, `<`, `<=`, `>`, `>=`
- Logical: `&&`, `||`, `!`
- Arithmetic: `+`, `-`, `*`, `/`, `%`

**Variables**: Reference workflow variables with `${variables.name}`

**Example**:
```json
{
  "type": "while",
  "condition": {
    "op": "<",
    "left": "${variables.frame_count}",
    "right": "${variables.max_frames}"
  },
  "nodes": [...]
}
```

---

## Testing the Debugger

### Quick Test
```python
python3 gdb_debugger.py ./debug_test_dbg
```

### Interactive Mode
```python
python3 simple_gdb_debugger.py -i ./debug_test_dbg
```

### Programmatic Test
```python
from gdb_debugger import GDBDebugger

debugger = GDBDebugger("./debug_test_dbg")
debugger.start()
debugger.set_breakpoint("debug_test.cpp:47")
debugger.run()
frames = debugger.backtrace(5)
debugger.quit()
```

---

## Performance Considerations

- **Loop Limits**: Workflows support up to 10,000 iterations (safety limit)
- **Condition Evaluation**: Evaluated once per iteration (no lazy evaluation)
- **Variable Scoping**: Local variables freed when scope exits
- **Breakpoint Overhead**: Each breakpoint in debugger ~1-10ms overhead

---

## See Also

- `gdb_debugger.py` - Full GDB-MI wrapper with parser
- `debugger_interface.py` - Simplified GDB command interface
- `workflow_control.hpp/.cpp` - C++ control structure implementations
- `WORKFLOW_CONTROL_GUIDE.md` - Complete control structure reference
- `workflow_cubes_advanced.json` - Advanced workflow with control structures
