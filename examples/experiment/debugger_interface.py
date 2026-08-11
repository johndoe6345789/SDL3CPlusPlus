#!/usr/bin/env python3
"""
GDB Debugger Interface - Command-line wrapper for C++ debugging

This script provides a simple interface to GDB for debugging C++ programs.
It demonstrates programmatic control of the debugger and parsing of output.

Usage:
    python3 debugger_interface.py demo          # Run interactive demo
    python3 debugger_interface.py -i program    # Interactive mode on program
"""

import subprocess
import re
import sys
from typing import List, Dict, Optional


class GDBInterface:
    """Interface to GDB debugger"""

    def __init__(self, program: str):
        self.program = program
        self.process: Optional[subprocess.Popen] = None

    def start(self) -> bool:
        """Start GDB process"""
        try:
            self.process = subprocess.Popen(
                ['gdb', '--quiet', '--args', self.program],
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                bufsize=1
            )
            print(f"✓ Started GDB for: {self.program}")
            return True
        except FileNotFoundError:
            print("✗ GDB not found")
            return False

    def send_cmd(self, cmd: str) -> str:
        """Send GDB command and read response"""
        if not self.process:
            return ""
        try:
            self.process.stdin.write(cmd + "\n")
            self.process.stdin.flush()
            output = ""
            while True:
                line = self.process.stdout.readline()
                if not line:
                    break
                output += line
                if line.startswith("(gdb)"):
                    break
            return output
        except Exception:
            return ""

    def quit(self):
        """Exit GDB"""
        if self.process:
            self.send_cmd("quit")
            self.process.terminate()


def run_demo():
    """Demonstration of GDB usage"""
    print("=" * 60)
    print("GDB Debugger Interface Demo")
    print("=" * 60)

    debugger = GDBInterface("./debug_test_dbg")
    if not debugger.start():
        return

    # Turn off pagination
    debugger.send_cmd("set pagination off")

    print("\n[1] Program Info:")
    output = debugger.send_cmd("file")
    print(output[:200])

    print("\n[2] Available Functions:")
    output = debugger.send_cmd("info functions")
    lines = output.split('\n')
    for line in lines[5:15]:  # Show first 10 functions
        if line.strip() and not line.startswith('(gdb)'):
            print(f"  {line}")

    print("\n[3] Setting Breakpoint:")
    output = debugger.send_cmd("break main")
    if "Breakpoint" in output:
        print("  ✓ Breakpoint set at main")
    else:
        print("  ⚠ Could not set breakpoint (debug info issue)")
        print("  Rebuild with: clang++ -g debug_test.cpp -o debug_test_dbg")

    print("\n[4] Attempting to Run:")
    output = debugger.send_cmd("run")
    if "Exited normally" in output:
        print("  ✓ Program completed")
    elif "Breakpoint" in output:
        print("  ✓ Hit breakpoint at main")
    else:
        print("  ⚠ Could not run (executable or debug issue)")

    debugger.quit()
    print("\n" + "=" * 60)
    print("Demo complete")
    print("=" * 60)


if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "-i":
        # Interactive mode
        program = sys.argv[2] if len(sys.argv) > 2 else "./debug_test_dbg"
        debugger = GDBInterface(program)
        debugger.start()
        print("Enter GDB commands (type 'quit' to exit):\n")
        try:
            while True:
                cmd = input("(gdb) ").strip()
                if cmd == "quit":
                    break
                if cmd:
                    output = debugger.send_cmd(cmd)
                    print(output)
        except KeyboardInterrupt:
            pass
        debugger.quit()
    else:
        # Demo mode
        run_demo()
