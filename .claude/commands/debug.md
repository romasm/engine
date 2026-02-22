Debug a running C++ application using CDB (Console Debugger). You can set breakpoints, step through code, inspect call stacks, and examine variable values.

## Session directory

All IPC files live in `%TEMP%/claude_debug/` (typically `C:\Users\roman\AppData\Local\Temp\claude_debug\`).

- `command.json` — you write commands here
- `output.json` — bridge writes results here
- `status.json` — current debugger state (`starting`, `stopped`, `running`, `busy`, `detached`, `exited`)

## Starting a debug session

The bridge runs in a **visible console window** so the user can see CDB output.

**Attach to the engine (default):**
```bash
powershell -Command "Start-Process python -ArgumentList 'c:\Users\roman\Dropbox\Engine\.claude\scripts\debug_bridge.py --name core_dev.exe' -WorkingDirectory 'c:\Users\roman\Dropbox\Engine'"
```

**Attach by PID:**
```bash
powershell -Command "Start-Process python -ArgumentList 'c:\Users\roman\Dropbox\Engine\.claude\scripts\debug_bridge.py --pid <PID>' -WorkingDirectory 'c:\Users\roman\Dropbox\Engine'"
```

**Launch under debugger:**
```bash
powershell -Command "Start-Process python -ArgumentList 'c:\Users\roman\Dropbox\Engine\.claude\scripts\debug_bridge.py --launch c:\Users\roman\Dropbox\Engine\build\bin\core_dev.exe --args \"-c\"' -WorkingDirectory 'c:\Users\roman\Dropbox\Engine\build\bin'"
```

After launching, wait 3-5 seconds for CDB to attach, then check status:
```bash
python -c "import json; print(json.dumps(json.load(open('C:/Users/roman/AppData/Local/Temp/claude_debug/status.json')), indent=2))"
```

## Sending commands

Write a JSON command to `command.json` with an incrementing `seq` number. Then read the result from `output.json` when it has the matching `seq`.

```bash
# Send a command (increment seq each time!)
python -c "import json; json.dump({'seq': 1, 'cmd': 'stack'}, open('C:/Users/roman/AppData/Local/Temp/claude_debug/command.json', 'w'))"

# Wait briefly, then read result
sleep 1
python -c "import json; d=json.load(open('C:/Users/roman/AppData/Local/Temp/claude_debug/output.json')); print(d.get('output',''))"
```

**CRITICAL**: Always increment `seq` for each new command. The bridge ignores commands with `seq <= last_seq`.

## Available commands

### Breakpoints
| Command | Description |
|---|---|
| `break FunctionName` | Break on function (deferred, resolves when module loads) |
| `break file.cpp:42` | Break on source file + line number |
| `break` | List all breakpoints |
| `delete 0` | Delete breakpoint #0 |
| `delete` | Delete all breakpoints |
| `disable 0` / `enable 0` | Disable/enable breakpoint |

### Execution control
| Command | Description |
|---|---|
| `go` / `continue` | Resume execution (async — returns immediately) |
| `wait` | Wait for target to stop (breakpoint, exception, etc.) |
| `interrupt` | Break into a running target (like Ctrl+C) |
| `step` / `stepin` | Step into (one source line) |
| `next` / `stepover` | Step over (one source line) |
| `finish` / `stepout` | Step out (run until current function returns) |

### Inspection
| Command | Description |
|---|---|
| `stack` | Call stack of current thread with parameters |
| `stackall` | Call stacks of ALL threads |
| `locals` | Local variables with types |
| `print <expr>` | Evaluate a C++ expression (e.g. `print this->m_width`) |
| `eval <expr>` | Same as print |
| `display <type> <addr>` | Display a typed structure at address |
| `memory <addr>` | Hex dump memory at address |
| `threads` | List all threads with stacks |
| `modules` | List loaded modules |
| `registers` | CPU registers |
| `where` | Current frame + top of stack |

### Session control
| Command | Description |
|---|---|
| `detach` | Detach from target and close CDB |
| `raw <cdb command>` | Send any raw CDB command directly |

## Typical workflow

1. **Start the bridge** (launch in visible window)
2. **Wait for attach** — check `status.json` shows `"state": "stopped"`
3. **NOTE**: When CDB attaches, the target process is FROZEN. The engine will be paused.
4. **Set breakpoints** — e.g. `break Render::Draw`, `break ScenePipeline.cpp:100`
5. **Resume** — send `go` (target runs, bridge shows `"state": "running"`)
6. **Wait for hit** — send `wait` (blocks until breakpoint hit or timeout)
7. **Inspect** — `stack`, `locals`, `print varName`
8. **Step** — `next`, `step`, `finish`
9. **Continue or detach** — `go` again, or `detach` when done

## Helper: Send command and read result

Use this pattern to send a command and get the result in one shot:

```bash
python -c "
import json, time, pathlib
d = pathlib.Path('C:/Users/roman/AppData/Local/Temp/claude_debug')
seq = json.load(open(d/'output.json')).get('seq', 0) + 1 if (d/'output.json').exists() else 1
json.dump({'seq': seq, 'cmd': '<COMMAND_HERE>'}, open(d/'command.json', 'w'))
time.sleep(1.5)
r = json.load(open(d/'output.json'))
if r.get('seq') == seq: print(r.get('output', ''))
else: print('Waiting...')
"
```

Replace `<COMMAND_HERE>` with the actual command. For `go`/`continue`, don't wait — just send and move on.

## Important notes

- **Attaching freezes the target** — always `go` after setting breakpoints to resume the engine
- **Symbol loading** — PDB files must be next to the exe. The bridge uses `build/bin/` as symbol path
- **Source-level debugging** requires PDB files from a Debug or Development build (not Release)
- After `go`, the engine resumes normally. Send `wait` to block until next breakpoint hit
- If `wait` times out, the breakpoint hasn't been hit yet — target is still running
- The bridge runs in a separate console window — the user can see all CDB output there

## User's request

$ARGUMENTS
