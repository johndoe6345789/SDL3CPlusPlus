#!/usr/bin/env python3
"""
Spy Thread Client - Real-time Game State Inspector

Monitor workflow execution and game state via spy thread socket interface.

Usage:
    # Get current FPS
    python3 spy_client.py get fps

    # Get all stats
    python3 spy_client.py status

    # Pause/resume execution
    python3 spy_client.py pause
    python3 spy_client.py resume

    # Watch mode (continuous monitoring)
    python3 spy_client.py watch --interval 1

    # Monitor specific stat
    python3 spy_client.py watch fps --interval 0.5

    # Record stats to CSV
    python3 spy_client.py record stats.csv --duration 30

    # Help
    python3 spy_client.py --help
"""

import argparse
import socket
import sys
import time
import csv
from datetime import datetime
from typing import Dict, Optional, List
import signal


class SpyClient:
    """Client for spy thread debugger"""

    def __init__(self, host: str = "127.0.0.1", port: int = 9999, timeout: float = 5.0):
        self.host = host
        self.port = port
        self.timeout = timeout

    def send_command(self, command: str) -> Optional[str]:
        """Send command to spy thread and get response"""
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.settimeout(self.timeout)
            s.connect((self.host, self.port))
            s.send(f"{command}\n".encode())
            response = s.recv(4096).decode().strip()
            s.close()
            return response
        except ConnectionRefusedError:
            print(f"✗ Connection refused: {self.host}:{self.port}")
            print("  Is the game running with spy thread enabled?")
            return None
        except socket.timeout:
            print(f"✗ Timeout: Could not reach {self.host}:{self.port}")
            return None
        except Exception as e:
            print(f"✗ Error: {e}")
            return None

    def parse_response(self, response: str) -> Dict[str, str]:
        """Parse response from spy thread"""
        result = {}
        for line in response.split('\n'):
            if '=' in line:
                key, value = line.split('=', 1)
                result[key] = value
        return result

    def get_stat(self, stat_name: str) -> Optional[str]:
        """Get single stat value"""
        response = self.send_command(f"get {stat_name}")
        if response:
            parsed = self.parse_response(response)
            return parsed.get(stat_name)
        return None

    def get_all_stats(self) -> Optional[Dict[str, str]]:
        """Get all stats"""
        response = self.send_command("status")
        if response:
            return self.parse_response(response)
        return None

    def pause(self) -> bool:
        """Pause execution"""
        response = self.send_command("pause")
        return response is not None and "true" in response

    def resume(self) -> bool:
        """Resume execution"""
        response = self.send_command("resume")
        return response is not None and "false" in response

    def get_help(self) -> Optional[str]:
        """Get help from spy thread"""
        return self.send_command("help")


def format_stat(key: str, value: str) -> str:
    """Format stat for display"""
    # Try to convert to float for better formatting
    try:
        fval = float(value)
        if key.endswith('_time') or key == 'elapsed_time':
            return f"{fval:.2f}ms" if fval < 1000 else f"{fval/1000:.2f}s"
        elif key == 'fps':
            return f"{fval:.1f} FPS"
        elif key.endswith('_used') or key.endswith('memory'):
            mb = fval / (1024 * 1024)
            return f"{mb:.1f} MB"
        else:
            return str(int(fval))
    except ValueError:
        return value


def cmd_get(args, client: SpyClient):
    """Handle 'get' command"""
    value = client.get_stat(args.stat)
    if value:
        print(f"{args.stat}={format_stat(args.stat, value)}")
    else:
        sys.exit(1)


def cmd_status(args, client: SpyClient):
    """Handle 'status' command"""
    stats = client.get_all_stats()
    if stats:
        print("\n╔════════════════════════════════════════╗")
        print("║        WORKFLOW EXECUTION STATUS       ║")
        print("╚════════════════════════════════════════╝\n")

        for key, value in sorted(stats.items()):
            formatted = format_stat(key, value)
            print(f"  {key:.<30} {formatted:>10}")

        print()
    else:
        sys.exit(1)


def cmd_pause(args, client: SpyClient):
    """Handle 'pause' command"""
    if client.pause():
        print("✓ Execution paused")
    else:
        print("✗ Failed to pause")
        sys.exit(1)


def cmd_resume(args, client: SpyClient):
    """Handle 'resume' command"""
    if client.resume():
        print("✓ Execution resumed")
    else:
        print("✗ Failed to resume")
        sys.exit(1)


def cmd_watch(args, client: SpyClient):
    """Handle 'watch' command - continuous monitoring"""
    stat_name = args.stat if args.stat else None
    interval = args.interval

    print(f"Watching {'all stats' if not stat_name else stat_name} (interval: {interval}s)")
    print("Press Ctrl+C to stop\n")

    try:
        while True:
            if stat_name:
                # Watch single stat
                value = client.get_stat(stat_name)
                if value:
                    formatted = format_stat(stat_name, value)
                    timestamp = datetime.now().strftime("%H:%M:%S")
                    print(f"[{timestamp}] {stat_name}={formatted}")
            else:
                # Watch all stats
                stats = client.get_all_stats()
                if stats:
                    timestamp = datetime.now().strftime("%H:%M:%S")
                    print(f"\n[{timestamp}] Status:")
                    for key, value in sorted(stats.items()):
                        formatted = format_stat(key, value)
                        print(f"  {key:.<30} {formatted:>10}")

            time.sleep(interval)
    except KeyboardInterrupt:
        print("\n\nWatch stopped")


def cmd_record(args, client: SpyClient):
    """Handle 'record' command - log stats to CSV"""
    filename = args.filename
    duration = args.duration
    interval = args.interval

    print(f"Recording to {filename} for {duration}s (interval: {interval}s)")

    start_time = time.time()
    elapsed = 0.0
    records = []

    try:
        while elapsed < duration:
            stats = client.get_all_stats()
            if stats:
                record = {'timestamp': datetime.now().isoformat(), 'elapsed': elapsed}
                record.update(stats)
                records.append(record)

            elapsed = time.time() - start_time
            print(f"  {elapsed:.1f}s / {duration}s - {len(records)} records")

            time.sleep(interval)
    except KeyboardInterrupt:
        print("\nRecording stopped early")

    # Write to CSV
    if records:
        try:
            with open(filename, 'w', newline='') as f:
                fieldnames = records[0].keys()
                writer = csv.DictWriter(f, fieldnames=fieldnames)
                writer.writeheader()
                writer.writerows(records)
            print(f"✓ Recorded {len(records)} entries to {filename}")
        except Exception as e:
            print(f"✗ Failed to write CSV: {e}")
            sys.exit(1)
    else:
        print("✗ No records collected")
        sys.exit(1)


def main():
    parser = argparse.ArgumentParser(
        description="Spy Thread Client - Monitor workflow execution and game state",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  Get current FPS:
    python3 spy_client.py get fps

  Get all statistics:
    python3 spy_client.py status

  Watch FPS in real-time (update every 0.5s):
    python3 spy_client.py watch fps --interval 0.5

  Watch all stats (update every 1s):
    python3 spy_client.py watch --interval 1

  Record all stats to CSV for 30 seconds:
    python3 spy_client.py record stats.csv --duration 30

  Pause and resume execution:
    python3 spy_client.py pause
    python3 spy_client.py resume
        """
    )

    parser.add_argument('--host', default='127.0.0.1', help='Spy thread host (default: 127.0.0.1)')
    parser.add_argument('--port', type=int, default=9999, help='Spy thread port (default: 9999)')
    parser.add_argument('--timeout', type=float, default=5.0, help='Socket timeout in seconds (default: 5.0)')

    subparsers = parser.add_subparsers(dest='command', help='Command to execute')

    # 'get' command
    get_parser = subparsers.add_parser('get', help='Get single stat')
    get_parser.add_argument('stat', help='Stat name (fps, frame_count, elapsed_time, etc.)')
    get_parser.set_defaults(func=cmd_get)

    # 'status' command
    status_parser = subparsers.add_parser('status', help='Get all statistics')
    status_parser.set_defaults(func=cmd_status)

    # 'pause' command
    pause_parser = subparsers.add_parser('pause', help='Pause execution')
    pause_parser.set_defaults(func=cmd_pause)

    # 'resume' command
    resume_parser = subparsers.add_parser('resume', help='Resume execution')
    resume_parser.set_defaults(func=cmd_resume)

    # 'watch' command
    watch_parser = subparsers.add_parser('watch', help='Watch stats in real-time')
    watch_parser.add_argument('stat', nargs='?', default=None, help='Specific stat to watch (optional)')
    watch_parser.add_argument('--interval', type=float, default=1.0, help='Update interval in seconds (default: 1.0)')
    watch_parser.set_defaults(func=cmd_watch)

    # 'record' command
    record_parser = subparsers.add_parser('record', help='Record stats to CSV file')
    record_parser.add_argument('filename', help='Output CSV filename')
    record_parser.add_argument('--duration', type=float, default=30.0, help='Record duration in seconds (default: 30.0)')
    record_parser.add_argument('--interval', type=float, default=0.5, help='Sample interval in seconds (default: 0.5)')
    record_parser.set_defaults(func=cmd_record)

    args = parser.parse_args()

    # Show help if no command
    if not args.command:
        parser.print_help()
        sys.exit(0)

    # Create client
    client = SpyClient(host=args.host, port=args.port, timeout=args.timeout)

    # Execute command
    try:
        args.func(args, client)
    except AttributeError:
        parser.print_help()
        sys.exit(1)


if __name__ == '__main__':
    main()
