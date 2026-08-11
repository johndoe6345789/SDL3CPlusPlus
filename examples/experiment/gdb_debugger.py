#!/usr/bin/env python3
"""
GDB-MI (Machine Interface) Debugger Wrapper
Programmatic control of GDB for debugging C++ code
"""

import subprocess
import json
import sys
import time
import re
from typing import Dict, List, Optional, Any, Tuple
from dataclasses import dataclass, asdict

# ============================================================================
# DATA STRUCTURES
# ============================================================================

@dataclass
class BreakpointInfo:
    """Information about a breakpoint"""
    number: int
    file: str
    line: int
    enabled: bool
    hit_count: int = 0

@dataclass
class StackFrame:
    """Information about a stack frame"""
    level: int
    function: str
    file: str
    line: int
    pc: str  # Program counter

@dataclass
class Variable:
    """Information about a variable"""
    name: str
    value: str
    type: str

@dataclass
class DebugSession:
    """Current debugging session state"""
    program: str
    running: bool = False
    breakpoints: Dict[int, BreakpointInfo] = None
    current_frame: Optional[StackFrame] = None
    variables: Dict[str, Variable] = None

    def __post_init__(self):
        if self.breakpoints is None:
            self.breakpoints = {}
        if self.variables is None:
            self.variables = {}


# ============================================================================
# GDB-MI PARSER
# ============================================================================

class GDBMIParser:
    """Parse GDB Machine Interface output"""

    @staticmethod
    def parse_output(line: str) -> Tuple[Optional[str], Optional[Dict[str, Any]]]:
        """
        Parse a GDB-MI output line
        Format: token^ result-class result
        Example: 1^done,reason="breakpoint-hit"
        """
        if not line.strip():
            return None, None

        # Extract token
        match = re.match(r'^(\d+)\^(\w+)(.*)', line)
        if not match:
            return None, None

        token = match.group(1)
        result_class = match.group(2)  # done, running, stopped, error
        result_text = match.group(3)

        # Parse result data
        data = GDBMIParser._parse_result(result_text) if result_text else {}

        return result_class, data

    @staticmethod
    def _parse_result(text: str) -> Dict[str, Any]:
        """Parse GDB-MI result data"""
        result = {}

        # Simple key=value parser (not full GDB-MI, but covers common cases)
        # Handle: reason="breakpoint-hit",thread-id="1"
        parts = re.findall(r'(\w+)=("(?:[^"\\]|\\.)*"|\[.*?\]|\{.*?\}|[^,]*)', text)

        for key, value in parts:
            # Clean up value
            if value.startswith('"') and value.endswith('"'):
                result[key] = value[1:-1]  # Remove quotes
            elif value.startswith('[') or value.startswith('{'):
                try:
                    result[key] = json.loads(value)
                except:
                    result[key] = value
            else:
                try:
                    result[key] = int(value)
                except:
                    result[key] = value

        return result


# ============================================================================
# GDB DEBUGGER INTERFACE
# ============================================================================

class GDBDebugger:
    """Interface to GDB via Machine Interface"""

    def __init__(self, program: str):
        self.program = program
        self.process: Optional[subprocess.Popen] = None
        self.session = DebugSession(program=program)
        self.command_counter = 0
        self.output_lines: List[str] = []

    def start(self) -> bool:
        """Start GDB with MI interface"""
        try:
            self.process = subprocess.Popen(
                ['gdb', '-i=mi', self.program],
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                bufsize=1
            )
            print(f"[GDB] Started debugging: {self.program}")
            return True
        except FileNotFoundError:
            print("[ERROR] GDB not found. Install with: brew install gdb")
            return False

    def send_command(self, cmd: str) -> str:
        """Send a command to GDB and get response"""
        if not self.process:
            return ""

        self.command_counter += 1
        full_cmd = f"{self.command_counter}{cmd}\n"

        try:
            self.process.stdin.write(full_cmd)
            self.process.stdin.flush()

            # Read response lines until we get the result
            response = ""
            while True:
                line = self.process.stdout.readline()
                if not line:
                    break

                response += line

                # MI output ends with "(gdb)" prompt
                if line.strip() == "(gdb)":
                    break

                # Stop at result line (^done, ^error, etc.)
                if line.startswith(str(self.command_counter)) and '^' in line:
                    # Read one more line to get the prompt
                    self.process.stdout.readline()
                    break

            return response
        except Exception as e:
            print(f"[ERROR] Command failed: {e}")
            return ""

    def set_breakpoint(self, file: str, line: int) -> Optional[int]:
        """Set a breakpoint at file:line"""
        cmd = f"-break-insert {file}:{line}"
        response = self.send_command(cmd)

        # Parse response for breakpoint number
        match = re.search(r'number="(\d+)"', response)
        if match:
            bp_num = int(match.group(1))
            self.session.breakpoints[bp_num] = BreakpointInfo(
                number=bp_num,
                file=file,
                line=line,
                enabled=True
            )
            print(f"[GDB] Breakpoint {bp_num} set at {file}:{line}")
            return bp_num

        return None

    def list_breakpoints(self) -> List[BreakpointInfo]:
        """List all breakpoints"""
        response = self.send_command("-break-list")
        breakpoints = []

        for bp_num, bp_info in self.session.breakpoints.items():
            breakpoints.append(bp_info)

        return breakpoints

    def run(self, args: str = "") -> Dict[str, Any]:
        """Run the program"""
        cmd = f"-exec-run{' ' + args if args else ''}"
        response = self.send_command(cmd)

        self.session.running = True
        print(f"[GDB] Program running...")

        # Wait for it to stop (breakpoint or end)
        return self._parse_stop_response(response)

    def continue_execution(self) -> Dict[str, Any]:
        """Continue execution after breakpoint"""
        response = self.send_command("-exec-continue")
        return self._parse_stop_response(response)

    def next(self) -> Dict[str, Any]:
        """Step over next line"""
        response = self.send_command("-exec-next")
        return self._parse_stop_response(response)

    def step(self) -> Dict[str, Any]:
        """Step into next line"""
        response = self.send_command("-exec-step")
        return self._parse_stop_response(response)

    def finish(self) -> Dict[str, Any]:
        """Run until function returns"""
        response = self.send_command("-exec-finish")
        return self._parse_stop_response(response)

    def backtrace(self, depth: int = 10) -> List[StackFrame]:
        """Get stack trace"""
        response = self.send_command(f"-stack-list-frames 0 {depth}")

        frames = []
        # Parse frame information
        for match in re.finditer(
            r'frame=\{level="(\d+)",addr="(0x[0-9a-f]+)",func="([^"]+)",file="([^"]+)",fullname="([^"]+)",line="(\d+)"',
            response
        ):
            level = int(match.group(1))
            pc = match.group(2)
            function = match.group(3)
            file = match.group(4)
            line = int(match.group(6))

            frames.append(StackFrame(
                level=level,
                function=function,
                file=file,
                line=line,
                pc=pc
            ))

        return frames

    def print_variable(self, var_name: str) -> Optional[str]:
        """Evaluate a variable in current context"""
        response = self.send_command(f"-data-evaluate-expression {var_name}")

        match = re.search(r'value="([^"]*)"', response)
        if match:
            value = match.group(1)
            self.session.variables[var_name] = Variable(
                name=var_name,
                value=value,
                type="unknown"
            )
            return value

        return None

    def list_locals(self) -> List[Variable]:
        """List local variables in current frame"""
        response = self.send_command("-stack-list-locals --simple-values")

        variables = []
        for match in re.finditer(
            r'name="([^"]+)",type="([^"]+)",value="([^"]*)"',
            response
        ):
            name = match.group(1)
            var_type = match.group(2)
            value = match.group(3)

            var = Variable(name=name, type=var_type, value=value)
            variables.append(var)
            self.session.variables[name] = var

        return variables

    def get_info(self) -> Dict[str, Any]:
        """Get debugging info"""
        return {
            "program": self.session.program,
            "running": self.session.running,
            "breakpoints": {
                bp_num: asdict(bp_info)
                for bp_num, bp_info in self.session.breakpoints.items()
            },
            "variables": {
                name: asdict(var)
                for name, var in self.session.variables.items()
            }
        }

    def _parse_stop_response(self, response: str) -> Dict[str, Any]:
        """Parse stop/breakpoint response"""
        result = {
            "stopped": "stopped" in response,
            "reason": None,
            "thread_id": None,
            "breakpoint_num": None
        }

        if "reason=" in response:
            match = re.search(r'reason="([^"]+)"', response)
            if match:
                result["reason"] = match.group(1)

        if "thread-id=" in response:
            match = re.search(r'thread-id="(\d+)"', response)
            if match:
                result["thread_id"] = int(match.group(1))

        if "bkptno=" in response:
            match = re.search(r'bkptno="(\d+)"', response)
            if match:
                result["breakpoint_num"] = int(match.group(1))

        return result

    def quit(self):
        """Exit GDB"""
        if self.process:
            self.send_command("-gdb-exit")
            self.process.terminate()
            self.session.running = False
            print("[GDB] Debugger closed")


# ============================================================================
# INTERACTIVE DEBUG SESSION
# ============================================================================

def debug_session_demo(program: str):
    """Demo interactive debugging session"""
    debugger = GDBDebugger(program)

    if not debugger.start():
        return

    print("\n" + "="*60)
    print("GDB-MI INTERACTIVE DEBUG SESSION")
    print("="*60 + "\n")

    # Set breakpoints
    print("[1] Setting breakpoints...")
    debugger.set_breakpoint("main.cpp", 200)  # graphics.init
    debugger.set_breakpoint("main.cpp", 335)  # render loop

    # Run program
    print("\n[2] Running program...")
    result = debugger.run()
    print(f"    Result: {result}\n")

    # Show stack trace
    print("[3] Stack trace:")
    frames = debugger.backtrace(depth=5)
    for frame in frames:
        print(f"    #{frame.level}: {frame.function} at {frame.file}:{frame.line}")

    # Inspect locals
    print("\n[4] Local variables:")
    locals_list = debugger.list_locals()
    for var in locals_list[:5]:  # Show first 5
        print(f"    {var.name} ({var.type}) = {var.value}")

    # Continue to next breakpoint
    print("\n[5] Continuing to next breakpoint...")
    result = debugger.continue_execution()
    print(f"    Result: {result}\n")

    # Step through code
    print("[6] Stepping through code...")
    for i in range(3):
        debugger.next()
        frames = debugger.backtrace(depth=1)
        if frames:
            frame = frames[0]
            print(f"    Step {i+1}: {frame.function} at {frame.file}:{frame.line}")

    # Get session info
    print("\n[7] Debug session info:")
    info = debugger.get_info()
    print(json.dumps(info, indent=2))

    # Cleanup
    print("\n[8] Closing debugger...")
    debugger.quit()

    print("\n" + "="*60)
    print("DEBUG SESSION COMPLETE")
    print("="*60 + "\n")


if __name__ == "__main__":
    if len(sys.argv) > 1:
        program = sys.argv[1]
    else:
        program = "./build/standalone_workflow_cubes"

    debug_session_demo(program)
