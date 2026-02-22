"""
CDB Debug Bridge for Claude Code
Manages a persistent CDB debugging session via file-based IPC.

Usage:
    python debug_bridge.py --name core_dev.exe
    python debug_bridge.py --pid 12345
    python debug_bridge.py --launch "c:\\path\\to\\exe" --args "-c"

Session directory: %TEMP%/claude_debug/
    command.json  - Claude writes commands here
    output.json   - Bridge writes results here
    status.json   - Current debugger state
"""

# Force unbuffered stdout so the console window shows output in real-time
import sys
import io
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding="utf-8", line_buffering=True)
sys.stderr = io.TextIOWrapper(sys.stderr.buffer, encoding="utf-8", line_buffering=True)

import subprocess
import os
import sys
import time
import threading
import re
import argparse
import json
from pathlib import Path


CDB_PATH = r"C:\Program Files\WindowsApps\Microsoft.WinDbg_1.2601.12001.0_x64__8wekyb3d8bbwe\amd64\cdb.exe"
SESSION_DIR = Path(os.environ.get("TEMP", ".")) / "claude_debug"
POLL_INTERVAL = 0.2


def find_cdb():
    """Locate cdb.exe on the system."""
    if os.path.isfile(CDB_PATH):
        return CDB_PATH
    # Fallback: search common locations
    candidates = [
        r"C:\Program Files (x86)\Windows Kits\10\Debuggers\x64\cdb.exe",
        r"C:\Program Files\Windows Kits\10\Debuggers\x64\cdb.exe",
    ]
    for path in candidates:
        if os.path.isfile(path):
            return path
    # Try PATH
    import shutil
    found = shutil.which("cdb.exe")
    if found:
        return found
    return None


class CDBSession:
    """Manages a CDB debugging session with stdin/stdout communication."""

    PROMPT_RE = re.compile(r"\d+:\d+(?::\w+)?> $")

    def __init__(self, cdb_path, symbol_path=None):
        self.cdb_path = cdb_path
        self.symbol_path = symbol_path or r"c:\Users\roman\Dropbox\Engine\build\bin"
        self.process = None
        self.output_lock = threading.Lock()
        self.output_chunks = []
        self.prompt_event = threading.Event()
        self.reader_thread = None
        self.target_running = False
        self.alive = True

    def attach_by_name(self, name):
        """Attach to a process by name."""
        return self._start([self.cdb_path, "-pn", name,
                           "-y", self.symbol_path,
                           "-lines"])

    def attach_by_pid(self, pid):
        """Attach to a process by PID."""
        return self._start([self.cdb_path, "-p", str(pid),
                           "-y", self.symbol_path,
                           "-lines"])

    def launch(self, exe_path, args=""):
        """Launch an executable under the debugger."""
        cmd = [self.cdb_path, "-y", self.symbol_path, "-lines", exe_path]
        if args:
            cmd.extend(args.split())
        return self._start(cmd)

    def _start(self, cmd):
        """Start CDB with given arguments."""
        try:
            self.process = subprocess.Popen(
                cmd,
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                bufsize=0,
                creationflags=0,
            )
        except Exception as exc:
            return f"ERROR: Failed to start CDB: {exc}"

        self.reader_thread = threading.Thread(target=self._reader, daemon=True)
        self.reader_thread.start()

        result = self._wait_for_prompt(timeout=15)
        if result is None:
            return "ERROR: Timed out waiting for CDB prompt"

        # Extract target PID from CDB output for interrupt support
        pid_match = re.search(r"id:\s*(\w+)\s", result, re.IGNORECASE)
        if pid_match:
            try:
                self._target_pid = int(pid_match.group(1), 16)
            except ValueError:
                pass

        return result

    def _reader(self):
        """Background thread: continuously reads CDB stdout."""
        try:
            buf = b""
            while self.alive:
                byte = self.process.stdout.read(1)
                if not byte:
                    break
                buf += byte
                # Try to decode what we have
                try:
                    char = buf.decode("utf-8")
                    buf = b""
                except UnicodeDecodeError:
                    if len(buf) > 4:
                        char = buf.decode("utf-8", errors="replace")
                        buf = b""
                    else:
                        continue

                with self.output_lock:
                    self.output_chunks.append(char)
                    current = "".join(self.output_chunks)
                    # Check for prompt at end of output
                    lines = current.split("\n")
                    last_line = lines[-1] if lines else ""
                    if self.PROMPT_RE.search(last_line):
                        self.target_running = False
                        self.prompt_event.set()
        except Exception:
            pass
        finally:
            self.alive = False
            self.prompt_event.set()

    def _wait_for_prompt(self, timeout=30):
        """Wait for CDB prompt, return accumulated output."""
        if self.prompt_event.wait(timeout):
            self.prompt_event.clear()
            with self.output_lock:
                result = "".join(self.output_chunks)
                self.output_chunks.clear()
            return result
        return None

    def command(self, cmd, timeout=30):
        """Send a command to CDB and wait for the prompt."""
        if not self.alive or not self.process:
            return "ERROR: CDB session is not active"

        with self.output_lock:
            self.output_chunks.clear()
            self.prompt_event.clear()

        try:
            self.process.stdin.write((cmd + "\n").encode())
            self.process.stdin.flush()
        except Exception as exc:
            return f"ERROR: Failed to send command: {exc}"

        result = self._wait_for_prompt(timeout)
        if result is None:
            return "TIMEOUT: Command did not complete within {}s. Target may be running.".format(timeout)
        return result

    def command_async(self, cmd):
        """Send a command without waiting (for 'g' / continue)."""
        if not self.alive or not self.process:
            return "ERROR: CDB session is not active"

        with self.output_lock:
            self.output_chunks.clear()
            self.prompt_event.clear()

        self.target_running = True
        try:
            self.process.stdin.write((cmd + "\n").encode())
            self.process.stdin.flush()
        except Exception as exc:
            self.target_running = False
            return f"ERROR: Failed to send command: {exc}"
        return "OK: Target is running. Use 'wait' or 'break' to check/stop."

    def wait_for_stop(self, timeout=60):
        """Wait for the target to stop (breakpoint hit, exception, etc.)."""
        result = self._wait_for_prompt(timeout)
        if result is None:
            return "TIMEOUT: Target still running after {}s".format(timeout)
        self.target_running = False
        return result

    def _break_in(self):
        """Send Debug Break to the target process via CDB."""
        try:
            import ctypes
            kernel32 = ctypes.windll.kernel32
            # Open the target process and inject a debug break
            pid = self._get_target_pid()
            if pid:
                handle = kernel32.OpenProcess(0x1F0FFF, False, pid)  # PROCESS_ALL_ACCESS
                if handle:
                    kernel32.DebugBreakProcess(handle)
                    kernel32.CloseHandle(handle)
                    return True
        except Exception:
            pass
        return False

    def _get_target_pid(self):
        """Get the target process PID from CDB's initial output or cache."""
        if hasattr(self, '_target_pid'):
            return self._target_pid
        return None

    def interrupt(self):
        """Break into the target (Ctrl+C equivalent)."""
        if not self.alive or not self.process:
            return "ERROR: CDB session is not active"
        self._break_in()
        time.sleep(0.5)
        return self.wait_for_stop(timeout=5)

    def detach(self):
        """Detach from the target and quit CDB."""
        if not self.alive or not self.process:
            return "Already detached"

        try:
            # If target is stopped (at a prompt), just send .detach + q
            if not self.target_running:
                with self.output_lock:
                    self.output_chunks.clear()
                    self.prompt_event.clear()
                self.process.stdin.write(b".detach\n")
                self.process.stdin.flush()
                self._wait_for_prompt(timeout=5)
                self.process.stdin.write(b"q\n")
                self.process.stdin.flush()
            else:
                # Target is running — break in, detach, quit
                self._break_in()
                time.sleep(0.5)
                self._wait_for_prompt(timeout=5)
                with self.output_lock:
                    self.output_chunks.clear()
                    self.prompt_event.clear()
                self.process.stdin.write(b".detach\n")
                self.process.stdin.flush()
                self._wait_for_prompt(timeout=5)
                self.process.stdin.write(b"q\n")
                self.process.stdin.flush()
        except Exception:
            pass

        self.alive = False
        try:
            self.process.wait(timeout=5)
        except Exception:
            try:
                self.process.kill()
            except Exception:
                pass
        return "Detached and closed CDB session"


# High-level command translation
COMMAND_MAP = {
    "stack":      "kp",
    "stackall":   "~*kp",
    "locals":     "dv /t",
    "threads":    "~*k",
    "modules":    "lm",
    "registers":  "r",
    "breakpoints":"bl",
    "where":      "ln @rip; kp 3",
    "step":       "t",
    "stepin":     "t",
    "next":       "p",
    "stepover":   "p",
    "finish":     "gu",
    "stepout":    "gu",
}


def translate_command(user_cmd):
    """
    Translate a high-level debug command to CDB command(s).
    Returns (cdb_cmd, is_async) tuple.
    """
    parts = user_cmd.strip().split(None, 1)
    verb = parts[0].lower() if parts else ""
    arg = parts[1] if len(parts) > 1 else ""

    # Direct CDB pass-through
    if verb == "raw":
        return (arg, False)

    # Simple translations
    if verb in COMMAND_MAP:
        return (COMMAND_MAP[verb], False)

    # Breakpoint commands
    if verb in ("break", "bp"):
        if not arg:
            return ("bl", False)  # list breakpoints
        # File:line format
        if ":" in arg and any(c.isdigit() for c in arg.split(":")[-1]):
            file_part, line_part = arg.rsplit(":", 1)
            return (f"bu `{file_part}:{line_part}`", False)
        # Symbol name
        return (f"bu {arg}", False)

    if verb in ("delete", "bc"):
        return (f"bc {arg}" if arg else "bc *", False)

    if verb in ("disable", "bd"):
        return (f"bd {arg}" if arg else "bd *", False)

    if verb in ("enable", "be"):
        return (f"be {arg}" if arg else "be *", False)

    # Continue / Go (async - target runs freely)
    if verb in ("go", "continue", "run", "g"):
        return ("g", True)

    # Print / evaluate
    if verb in ("print", "p") and arg:
        return (f"?? {arg}", False)

    if verb in ("eval", "evaluate", "?") and arg:
        return (f"?? {arg}", False)

    if verb in ("display", "dt") and arg:
        return (f"dt {arg}", False)

    if verb in ("memory", "mem", "db") and arg:
        return (f"db {arg}", False)

    # Source listing
    if verb in ("list", "source", "ls"):
        if arg:
            return (f"lsf {arg}", False)
        return ("lsp -a", False)

    # Wait for breakpoint
    if verb == "wait":
        return ("__WAIT__", False)

    # Interrupt (break into running target)
    if verb in ("interrupt", "halt", "stop", "ctrl+c"):
        return ("__INTERRUPT__", False)

    # Detach
    if verb == "detach":
        return ("__DETACH__", False)

    # Unknown - pass through as raw CDB command
    return (user_cmd, False)


def write_json(path, data):
    """Atomically write JSON to a file."""
    tmp = str(path) + ".tmp"
    with open(tmp, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2)
    os.replace(tmp, str(path))


def main():
    parser = argparse.ArgumentParser(description="CDB Debug Bridge for Claude Code")
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--name", help="Attach to process by name (e.g. core_dev.exe)")
    group.add_argument("--pid", type=int, help="Attach to process by PID")
    group.add_argument("--launch", help="Launch executable under debugger")
    parser.add_argument("--args", default="", help="Arguments for launched executable")
    parser.add_argument("--symbols", default=r"c:\Users\roman\Dropbox\Engine\build\bin",
                       help="Symbol search path")
    parser.add_argument("--session-dir", default=str(SESSION_DIR),
                       help="Session directory for IPC files")
    args = parser.parse_args()

    cdb_path = find_cdb()
    if not cdb_path:
        print("ERROR: cdb.exe not found. Install Debugging Tools for Windows.", file=sys.stderr)
        sys.exit(1)

    session_dir = Path(args.session_dir)
    session_dir.mkdir(parents=True, exist_ok=True)

    command_file = session_dir / "command.json"
    output_file = session_dir / "output.json"
    status_file = session_dir / "status.json"

    # Clean up old session files
    for f in [command_file, output_file, status_file]:
        if f.exists():
            f.unlink()

    write_json(status_file, {"state": "starting"})

    print(f"CDB path: {cdb_path}")
    print(f"Session directory: {session_dir}")

    session = CDBSession(cdb_path, symbol_path=args.symbols)

    # Attach or launch
    if args.name:
        print(f"Attaching to process: {args.name}")
        result = session.attach_by_name(args.name)
    elif args.pid:
        print(f"Attaching to PID: {args.pid}")
        result = session.attach_by_pid(args.pid)
    else:
        print(f"Launching: {args.launch} {args.args}")
        result = session.launch(args.launch, args.args)

    if result and result.startswith("ERROR"):
        print(result, file=sys.stderr)
        write_json(status_file, {"state": "error", "message": result})
        sys.exit(1)

    print("CDB attached successfully.")
    print("---CDB INIT OUTPUT---")
    print(result)
    print("---END CDB INIT OUTPUT---")

    # Load source lines support
    session.command(".lines -e")

    write_json(status_file, {"state": "stopped", "reason": "attached"})
    write_json(output_file, {"seq": 0, "status": "ok", "output": result})

    # Main loop: poll for commands
    last_seq = 0
    print("\nBridge ready. Polling for commands...")

    try:
        while session.alive:
            # Check for new command
            if command_file.exists():
                try:
                    with open(command_file, "r", encoding="utf-8") as f:
                        cmd_data = json.load(f)
                except (json.JSONDecodeError, IOError):
                    time.sleep(POLL_INTERVAL)
                    continue

                seq = cmd_data.get("seq", 0)
                if seq <= last_seq:
                    time.sleep(POLL_INTERVAL)
                    continue

                last_seq = seq
                user_cmd = cmd_data.get("cmd", "")
                timeout = cmd_data.get("timeout", 30)

                print(f"\n[seq={seq}] Command: {user_cmd}")

                cdb_cmd, is_async = translate_command(user_cmd)

                # Special commands
                if cdb_cmd == "__WAIT__":
                    write_json(status_file, {"state": "waiting"})
                    result = session.wait_for_stop(timeout=timeout)
                    state = "stopped" if not result.startswith("TIMEOUT") else "running"
                    write_json(status_file, {"state": state})
                    write_json(output_file, {"seq": seq, "status": "ok", "output": result})

                elif cdb_cmd == "__INTERRUPT__":
                    result = session.interrupt()
                    write_json(status_file, {"state": "stopped", "reason": "interrupted"})
                    write_json(output_file, {"seq": seq, "status": "ok", "output": result})

                elif cdb_cmd == "__DETACH__":
                    result = session.detach()
                    write_json(status_file, {"state": "detached"})
                    write_json(output_file, {"seq": seq, "status": "ok", "output": result})
                    print("Detached. Exiting bridge.")
                    break

                elif is_async:
                    result = session.command_async(cdb_cmd)
                    write_json(status_file, {"state": "running"})
                    write_json(output_file, {"seq": seq, "status": "ok", "output": result})

                else:
                    write_json(status_file, {"state": "busy"})
                    result = session.command(cdb_cmd, timeout=timeout)
                    if result is None:
                        result = "TIMEOUT"
                    write_json(status_file, {"state": "stopped"})
                    write_json(output_file, {"seq": seq, "status": "ok", "output": result})

                # Echo CDB output to console so user can see it
                if result:
                    output_lines = result.strip()
                    if len(output_lines) > 3000:
                        output_lines = output_lines[:3000] + "\n... (truncated)"
                    print(f"[seq={seq}] CDB output:\n{output_lines}")
                else:
                    print(f"[seq={seq}] (no output)")

            time.sleep(POLL_INTERVAL)

    except KeyboardInterrupt:
        print("\nInterrupted. Detaching...")
        session.detach()
    finally:
        write_json(status_file, {"state": "exited"})
        print("Bridge exited.")


if __name__ == "__main__":
    main()
